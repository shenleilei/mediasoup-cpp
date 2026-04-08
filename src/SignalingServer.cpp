#include "SignalingServer.h"
#include "Constants.h"
#include <App.h>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <dirent.h>
#include <sys/stat.h>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {

struct PerSocketData {
	std::string peerId;
	std::string roomId;
	uint64_t sessionId = 0;  // Monotonic session token to detect stale requests
	// Shared token: set to false when .close fires, checked in deferred callbacks
	std::shared_ptr<std::atomic<bool>> alive = std::make_shared<std::atomic<bool>>(true);
};

static std::atomic<uint64_t> g_nextSessionId{1};

SignalingServer::SignalingServer(int port, RoomService& roomService, const std::string& recordDir)
	: port_(port), roomService_(roomService), recordDir_(recordDir) {}

SignalingServer::~SignalingServer() {
	stop();
}

void SignalingServer::postWork(std::function<void()> fn) {
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		taskQueue_.push(std::move(fn));
	}
	queueCv_.notify_one();
}

void SignalingServer::workerLoop() {
	while (true) {
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(queueMutex_);
			queueCv_.wait(lock, [this]{ return workerStop_ || !taskQueue_.empty(); });
			if (workerStop_ && taskQueue_.empty()) return;
			task = std::move(taskQueue_.front());
			taskQueue_.pop();
		}
		try {
			task();
		} catch (const std::exception& e) {
			spdlog::error("worker task exception: {}", e.what());
		} catch (...) {
			spdlog::error("worker task unknown exception");
		}
	}
}

void SignalingServer::run() {
	running_ = true;

	// Start the single worker thread
	workerThread_ = std::thread(&SignalingServer::workerLoop, this);

	// peerId → ws* mapping for notify/broadcast
	struct WsMap {
		std::mutex mutex;
		// Key: "roomId/peerId" — isolates peers across rooms
		std::unordered_map<std::string, uWS::WebSocket<false, true, PerSocketData>*> peers;
		static std::string key(const std::string& roomId, const std::string& peerId) {
			return roomId + "/" + peerId;
		}
	};
	auto wsMap = std::make_shared<WsMap>();

	// Capture the uWS event loop for defer() calls from worker thread
	uwsLoop_ = (void*)uWS::Loop::get();

	// Wire up RoomService callbacks — these may be called from the worker thread,
	// so they must defer ws->send() back to the uWS thread.
	uWS::Loop* loop = uWS::Loop::get();
	roomService_.setNotify([wsMap, loop](const std::string& roomId, const std::string& peerId, const json& msg) {
		std::string data = msg.dump();
		std::string mapKey = WsMap::key(roomId, peerId);
		loop->defer([wsMap, mapKey, data = std::move(data)] {
			std::lock_guard<std::mutex> lock(wsMap->mutex);
			auto it = wsMap->peers.find(mapKey);
			if (it != wsMap->peers.end())
				it->second->send(data, uWS::OpCode::TEXT);
		});
	});

	roomService_.setBroadcast([wsMap, loop](const std::string& roomId,
		const std::string& excludePeerId, const json& msg)
	{
		std::string data = msg.dump();
		std::string excludeKey = excludePeerId.empty() ? "" : WsMap::key(roomId, excludePeerId);
		loop->defer([wsMap, roomId, excludeKey, data = std::move(data)] {
			std::lock_guard<std::mutex> lock(wsMap->mutex);
			for (auto& [mapKey, ws] : wsMap->peers) {
				if (mapKey == excludeKey) continue;
				auto* sd = ws->getUserData();
				if (sd->roomId == roomId)
					ws->send(data, uWS::OpCode::TEXT);
			}
		});
	});

	// GC timer
	struct us_timer_t* gcTimer = nullptr;

	uWS::App().ws<PerSocketData>("/ws", {
		.compression = uWS::DISABLED,
		.maxPayloadLength = kWsMaxPayloadBytes,
		.idleTimeout = kWsIdleTimeoutSec,

		.open = [wsMap](auto* ws) {
			auto* d = ws->getUserData();
			d->peerId = "";
			d->roomId = "";
		},

		.message = [this, wsMap, loop](auto* ws, std::string_view message, uWS::OpCode) {
			// Parse on the main thread (cheap) to extract parameters
			json req;
			try {
				req = json::parse(message);
			} catch (...) { return; }
			if (!req.contains("request") || !req["request"].get<bool>()) return;

			std::string method = req.value("method", "");
			uint64_t id = req.value("id", 0);
			json data = req.value("data", json::object());
			auto* sd = ws->getUserData();
			std::string roomId = sd->roomId;
			std::string peerId = sd->peerId;
			uint64_t sessionId = sd->sessionId;

			if (method != "clientStats") {
				spdlog::debug("[{} {}] {} id={}", roomId, peerId, method, id);
			}

			// clientStats is lightweight — handle inline, no IPC
			if (method == "clientStats") {
				roomService_.setClientStats(roomId, peerId, data);
				json resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", json::object()}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
				return;
			}

			// For join, extract params now (before ws might change)
			std::string joinRoomId, joinPeerId, joinDisplayName, joinClientIp;
			json joinRtpCaps;
			if (method == "join") {
				joinRoomId = data.value("roomId", "default");
				joinPeerId = data.value("peerId", "");
				joinDisplayName = data.value("displayName", joinPeerId);
				joinRtpCaps = data.value("rtpCapabilities", json::object());
				// Client IP: prefer explicit field (for proxy scenarios), fallback to remote address
				joinClientIp = data.value("clientIp", "");
				if (joinClientIp.empty())
					joinClientIp = std::string(ws->getRemoteAddressAsText());
			}

			// Capture alive token to detect if ws was closed before defer runs
			auto alive = sd->alive;

			// Dispatch to worker thread — all RoomService calls happen there
			postWork([this, wsMap, ws, alive, loop, method, id, data,
				roomId, peerId, sessionId, joinRoomId, joinPeerId, joinDisplayName, joinRtpCaps, joinClientIp]
			{
				// For non-join requests, verify this socket's session is still current.
				// Prevents stale requests from a replaced connection from affecting the new session.
				if (method != "join" && !roomId.empty()) {
					std::lock_guard<std::mutex> lock(wsMap->mutex);
					auto mapKey = WsMap::key(roomId, peerId);
					auto it = wsMap->peers.find(mapKey);
					if (it == wsMap->peers.end() || it->second->getUserData()->sessionId != sessionId)
						return; // stale request from replaced connection
				}

				RoomService::Result result;
				try {
					if (method == "join") {
						result = roomService_.join(joinRoomId, joinPeerId, joinDisplayName, joinRtpCaps, joinClientIp);
					} else if (method == "createWebRtcTransport") {
						result = roomService_.createTransport(roomId, peerId,
							data.value("producing", false), data.value("consuming", false));
					} else if (method == "connectWebRtcTransport") {
						result = roomService_.connectTransport(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("dtlsParameters").get<DtlsParameters>());
					} else if (method == "produce") {
						result = roomService_.produce(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("kind").get<std::string>(),
							data.at("rtpParameters"));
					} else if (method == "consume") {
						result = roomService_.consume(roomId, peerId,
							data.at("transportId").get<std::string>(),
							data.at("producerId").get<std::string>(),
							data.at("rtpCapabilities"));
					} else if (method == "pauseProducer") {
						result = roomService_.pauseProducer(roomId,
							data.at("producerId").get<std::string>());
					} else if (method == "resumeProducer") {
						result = roomService_.resumeProducer(roomId,
							data.at("producerId").get<std::string>());
					} else if (method == "restartIce") {
						result = roomService_.restartIce(roomId, peerId,
							data.at("transportId").get<std::string>());
					} else if (method == "getStats") {
						json stats = roomService_.collectPeerStats(roomId,
							data.value("peerId", peerId));
						result = {true, stats};
					} else {
						result = {false, {}, "", "unknown method: " + method};
					}
				} catch (const std::exception& e) {
					spdlog::error("[{} {}] {} error: {}", roomId, peerId, method, e.what());
					result = {false, {}, "", e.what()};
				}

				// Build response
				json resp;
				if (!result.redirect.empty()) {
					resp = {{"response", true}, {"id", id}, {"ok", false},
						{"redirect", result.redirect}};
				} else if (result.ok) {
					resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", result.data}};
				} else {
					resp = {{"response", true}, {"id", id}, {"ok", false}, {"error", result.error}};
				}
				std::string respStr = resp.dump();

				// For join success, we need to update PerSocketData on the main thread
				bool joinOk = (method == "join" && result.ok && result.redirect.empty());
				std::string jRoomId = joinRoomId, jPeerId = joinPeerId;

				// Defer ws->send() back to the uWS event loop thread
				loop->defer([wsMap, ws, alive, respStr = std::move(respStr),
					joinOk, jRoomId = std::move(jRoomId), jPeerId = std::move(jPeerId)]
				{
					if (!alive->load()) return;
					if (joinOk) {
						auto* sd = ws->getUserData();
						sd->roomId = jRoomId;
						sd->peerId = jPeerId;
						std::string mapKey = WsMap::key(jRoomId, jPeerId);
						decltype(ws) oldWs = nullptr;
						{
							std::lock_guard<std::mutex> lock(wsMap->mutex);
							sd->sessionId = g_nextSessionId++;  // inside mutex for happens-before
							auto it = wsMap->peers.find(mapKey);
							if (it != wsMap->peers.end() && it->second != ws)
								oldWs = it->second;
							wsMap->peers[mapKey] = ws;
						}
						spdlog::info("[{} {}] joined (session:{})", jRoomId, jPeerId, sd->sessionId);
						if (oldWs) oldWs->end(4000, "replaced");
					}
					ws->send(respStr, uWS::OpCode::TEXT);
				});
			});
		},

		.close = [this, wsMap](auto* ws, int, std::string_view) {
			auto* sd = ws->getUserData();
			sd->alive->store(false);
			if (!sd->peerId.empty()) {
				spdlog::info("[{} {}] disconnected", sd->roomId, sd->peerId);
				std::string roomId = sd->roomId;
				std::string peerId = sd->peerId;
				std::string mapKey = WsMap::key(roomId, peerId);
				bool erased = false;
				{
					std::lock_guard<std::mutex> lock(wsMap->mutex);
					auto it = wsMap->peers.find(mapKey);
					if (it != wsMap->peers.end() && it->second == ws) {
						wsMap->peers.erase(it);
						erased = true;
					}
				}
				if (erased && !roomId.empty()) {
					postWork([this, roomId, peerId] {
						try {
							roomService_.leave(roomId, peerId);
						} catch (const std::exception& e) {
							spdlog::error("[{} {}] leave failed: {}", roomId, peerId, e.what());
						} catch (...) {
							spdlog::error("[{} {}] leave failed: unknown error", roomId, peerId);
						}
					});
				}
			}
		}

	}).get("/api/resolve", [this](auto* res, auto* req) {
		std::string roomId(req->getQuery("roomId"));
		if (roomId.empty()) {
			res->writeStatus("400 Bad Request")->writeHeader("Content-Type", "application/json")
				->end(R"({"error":"roomId required"})");
			return;
		}
		// Get client IP: prefer query param, then X-Forwarded-For, then remote address
		std::string clientIp(req->getQuery("clientIp"));
		if (clientIp.empty()) {
			std::string xff(req->getHeader("x-forwarded-for"));
			if (!xff.empty()) {
				auto comma = xff.find(',');
				clientIp = (comma != std::string::npos) ? xff.substr(0, comma) : xff;
			}
		}
		if (clientIp.empty()) clientIp = std::string(res->getRemoteAddressAsText());
		auto result = roomService_.resolveRoom(roomId, clientIp);
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(result.dump());

	}).get("/api/node-load", [this](auto* res, auto*) {
		auto load = roomService_.getNodeLoad();
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(load.dump());

	}).get("/api/recordings", [this](auto* res, auto*) {
		if (recordDir_.empty()) {
			res->writeHeader("Content-Type", "application/json")->end("[]");
			return;
		}
		json result = json::array();
		DIR* dir = opendir(recordDir_.c_str());
		if (!dir) { res->writeHeader("Content-Type", "application/json")->end("[]"); return; }
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.') continue;
			std::string roomPath = recordDir_ + "/" + ent->d_name;
			struct stat st;
			if (stat(roomPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
			std::string roomId(ent->d_name);
			DIR* rdir = opendir(roomPath.c_str());
			if (!rdir) continue;
			struct dirent* rent;
			while ((rent = readdir(rdir))) {
				std::string fname(rent->d_name);
				if (fname.size() < 6 || fname.substr(fname.size()-5) != ".webm") continue;
				std::string base = fname.substr(0, fname.size()-5);
				auto pos = base.rfind('_');
				std::string peerId = (pos != std::string::npos) ? base.substr(0, pos) : base;
				int64_t ts = 0;
				if (pos != std::string::npos) try { ts = std::stoll(base.substr(pos+1)); } catch(...) {}
				std::string fullPath = roomPath + "/" + fname;
				struct stat fst;
				int64_t fileSize = 0;
				if (stat(fullPath.c_str(), &fst) == 0) fileSize = fst.st_size;
				bool hasQos = (stat((roomPath + "/" + base + ".qos.json").c_str(), &fst) == 0);
				result.push_back({
					{"roomId", roomId}, {"peerId", peerId}, {"timestamp", ts},
					{"file", fname}, {"size", fileSize}, {"hasQos", hasQos}
				});
			}
			closedir(rdir);
		}
		closedir(dir);
		res->writeHeader("Content-Type", "application/json")->end(result.dump());

	}).get("/recordings/*", [this](auto* res, auto* req) {
		if (recordDir_.empty()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
		std::string url(req->getUrl());
		std::string relPath = url.substr(12);
		if (relPath.find("..") != std::string::npos) {
			res->writeStatus("403 Forbidden")->end("Forbidden"); return;
		}
		std::string fullPath = recordDir_ + "/" + relPath;
		std::ifstream file(fullPath, std::ios::binary);
		if (!file.is_open()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		std::string ct = "application/octet-stream";
		if (relPath.find(".webm") != std::string::npos) ct = "video/webm";
		else if (relPath.find(".json") != std::string::npos) ct = "application/json";
		res->writeHeader("Content-Type", ct)->end(content);

	}).get("/*", [](auto* res, auto* req) {
		std::string url(req->getUrl());
		if (url == "/") url = "/index.html";
		std::string path = "public" + url;
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		std::string ct = "text/plain";
		if (url.find(".html") != std::string::npos) ct = "text/html";
		else if (url.find(".js") != std::string::npos) ct = "application/javascript";
		else if (url.find(".css") != std::string::npos) ct = "text/css";
		res->writeHeader("Content-Type", ct)->end(content);

	}).listen(port_, [this, &gcTimer](auto* listenSocket) {
		if (listenSocket) {
			spdlog::info("SignalingServer listening on port {}", port_);
			auto* loop = uWS::Loop::get();

			// GC timer — dispatch to worker thread
			gcTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			SignalingServer* self = this;
			memcpy(us_timer_ext(gcTimer), &self, sizeof(SignalingServer*));
			us_timer_set(gcTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				s->postWork([s] {
					s->roomService_.cleanIdleRooms(kIdleRoomTimeoutSec);
				});
			}, kGcIntervalMs, kGcIntervalMs);

			// Health check timer — dispatch to worker thread
			auto* healthTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			memcpy(us_timer_ext(healthTimer), &self, sizeof(SignalingServer*));
			us_timer_set(healthTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				s->postWork([s] {
					try { s->roomService_.checkRoomHealth(); } catch (const std::exception& e) {
						spdlog::error("checkRoomHealth exception: {}", e.what());
					} catch (...) {
						spdlog::error("checkRoomHealth unknown exception");
					}
				});
			}, kHealthCheckIntervalMs, kHealthCheckIntervalMs);

			// Stats broadcast timer — dispatch to worker thread
			auto* statsTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			memcpy(us_timer_ext(statsTimer), &self, sizeof(SignalingServer*));
			us_timer_set(statsTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				s->postWork([s] {
					try { s->roomService_.broadcastStats(); } catch (const std::exception& e) {
						spdlog::error("broadcastStats exception: {}", e.what());
					} catch (...) {
						spdlog::error("broadcastStats unknown exception");
					}
				});
			}, kStatsBroadcastIntervalMs, kStatsBroadcastIntervalMs);

			// Redis heartbeat timer — dispatch to worker thread
			auto* redisTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
			memcpy(us_timer_ext(redisTimer), &self, sizeof(SignalingServer*));
			us_timer_set(redisTimer, [](struct us_timer_t* t) {
				SignalingServer* s;
				memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
				s->postWork([s] {
					s->roomService_.heartbeatRegistry();
				});
			}, kRedisHeartbeatIntervalSec * 1000, kRedisHeartbeatIntervalSec * 1000);

			struct ShutdownCtx { us_listen_socket_t* sock; };
			auto* shutdownTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(ShutdownCtx));
			ShutdownCtx sctx{listenSocket};
			memcpy(us_timer_ext(shutdownTimer), &sctx, sizeof(ShutdownCtx));
			us_timer_set(shutdownTimer, [](struct us_timer_t* t) {
				if (g_shutdown) {
					ShutdownCtx ctx;
					memcpy(&ctx, us_timer_ext(t), sizeof(ShutdownCtx));
					us_listen_socket_close(0, ctx.sock);
					us_timer_close(t);
				}
			}, kShutdownPollIntervalMs, kShutdownPollIntervalMs);
		} else {
			spdlog::error("SignalingServer failed to listen on port {}", port_);
		}
	}).run();
}

void SignalingServer::stop() {
	running_ = false;
	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		workerStop_ = true;
	}
	queueCv_.notify_one();
	if (workerThread_.joinable()) workerThread_.join();
}

} // namespace mediasoup
