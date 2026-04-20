#include "SignalingServerHttp.h"

#include "Logger.h"
#include "RoomRegistry.h"
#include "StaticFileResponder.h"
#include "WorkerThread.h"

#include <App.h>
#include <atomic>
#include <chrono>
#include <dirent.h>
#include <sstream>
#include <sys/stat.h>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {
namespace {

std::atomic<uint64_t> g_nextResolveRequestId{1};

template<typename Response, typename Request>
std::string ResolveClientIp(Response* res, Request* req)
{
	std::string clientIp(req->getQuery("clientIp"));
	if (clientIp.empty()) {
		std::string xff(req->getHeader("x-forwarded-for"));
		if (!xff.empty()) {
			auto comma = xff.find(',');
			clientIp = (comma != std::string::npos) ? xff.substr(0, comma) : xff;
		}
	}
	if (clientIp.empty()) {
		clientIp = std::string(res->getRemoteAddressAsText());
	}
	return clientIp;
}

} // namespace

void SignalingServerHttp::RegisterHttpRoutes(uWS::App& app, SignalingServer& server, uWS::Loop* loop)
{
	app.get("/api/resolve", [&server, loop](auto* res, auto* req) {
		const auto requestId = g_nextResolveRequestId.fetch_add(1, std::memory_order_relaxed);
		std::string roomId(req->getQuery("roomId"));
		if (roomId.empty()) {
			res->writeStatus("400 Bad Request")->writeHeader("Content-Type", "application/json")
				->end(R"({"error":"roomId required"})");
			return;
		}
		const std::string clientIp = ResolveClientIp(res, req);
		const auto requestStart = std::chrono::steady_clock::now();
		spdlog::warn("api.resolve received [req:{} roomId:{} clientIp:{} registry:{}]",
			requestId, roomId, clientIp, server.registry_ != nullptr);

		if (!server.registry_) {
			res->writeHeader("Content-Type", "application/json")
				->writeHeader("Access-Control-Allow-Origin", "*")
				->end(R"({"wsUrl":"","isNew":true})");
			return;
		}

		auto aborted = std::make_shared<std::atomic<bool>>(false);
		res->onAborted([aborted] {
			aborted->store(true, std::memory_order_relaxed);
		});

		server.enqueueRegistryTask([&server, loop, res, aborted, roomId, clientIp, requestId, requestStart] {
			RoomRegistry::ResolveResult result{"", true};
			const auto resolveStart = std::chrono::steady_clock::now();
			spdlog::warn("api.resolve executing [req:{} roomId:{} clientIp:{}]",
				requestId, roomId, clientIp);
			try {
				result = server.registry_->resolveRoom(roomId, clientIp);
			} catch (const std::exception& e) {
				spdlog::warn("api.resolve failed [req:{} roomId:{} error:{}]",
					requestId, roomId, e.what());
			} catch (...) {
				spdlog::warn("api.resolve failed [req:{} roomId:{} error:unknown]",
					requestId, roomId);
			}
			const auto resolveMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - resolveStart).count();
			spdlog::warn("api.resolve resolved [req:{} roomId:{} wsUrl:{} isNew:{} resolveMs:{}]",
				requestId, roomId, result.wsUrl, result.isNew, resolveMs);

			json responseBody;
			if (!result.wsUrl.empty()) {
				responseBody = {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
			} else {
				responseBody = {{"wsUrl", ""}, {"isNew", true}};
			}
			std::string body = responseBody.dump();

			loop->defer([res, aborted, body = std::move(body)] {
				if (aborted->load(std::memory_order_relaxed)) return;
				res->writeHeader("Content-Type", "application/json")
					->writeHeader("Access-Control-Allow-Origin", "*")
					->end(body);
			});
			const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - requestStart).count();
			spdlog::warn("api.resolve response scheduled [req:{} roomId:{} totalMs:{}]",
				requestId, roomId, totalMs);
		}, "api.resolve#" + std::to_string(requestId) + " room=" + roomId);
	});

	app.get("/api/node-load", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		json load = {
			{"rooms", snapshot.totalRooms},
			{"maxRooms", snapshot.totalMaxRooms},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", server.workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads},
			{"knownNodes", snapshot.knownNodes},
			{"registryEnabled", snapshot.registryEnabled},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"healthy", server.isHealthy(snapshot)},
			{"dispatchRooms", snapshot.dispatchRooms},
			{"staleRequestDrops", snapshot.staleRequestDrops},
			{"rejectedClientStats", snapshot.rejectedClientStats},
			{"workerQueueStats", snapshot.workerQueues},
			{"roomOwnership", snapshot.roomOwnership}
		};
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(load.dump());
	});

	app.get("/healthz", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		bool healthy = server.isHealthy(snapshot);
		json health = {
			{"ok", healthy},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"registryEnabled", snapshot.registryEnabled},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", server.workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads},
			{"rooms", snapshot.totalRooms},
			{"maxRooms", snapshot.totalMaxRooms}
		};
		if (healthy) {
			res->writeHeader("Content-Type", "application/json")->end(health.dump());
		} else {
			res->writeStatus("503 Service Unavailable")
				->writeHeader("Content-Type", "application/json")
				->end(health.dump());
		}
	});

	app.get("/metrics", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		res->writeHeader("Content-Type", "text/plain; version=0.0.4")
			->end(server.buildPrometheusMetrics(snapshot));
	});

	app.get("/api/recordings", [&server](auto* res, auto*) {
		if (server.recordDir_.empty()) {
			res->writeHeader("Content-Type", "application/json")->end("[]");
			return;
		}
		json result = json::array();
		DIR* dir = opendir(server.recordDir_.c_str());
		if (!dir) { res->writeHeader("Content-Type", "application/json")->end("[]"); return; }
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.') continue;
			std::string roomPath = server.recordDir_ + "/" + ent->d_name;
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
	});

	app.get("/recordings/*", [&server](auto* res, auto* req) {
		if (server.recordDir_.empty()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
		std::string url(req->getUrl());
		std::string relPath = url.substr(12);
		bool forbidden = false;
		auto resolved = ResolveFileUnderBase(server.recordDir_, relPath, forbidden);
		if (!resolved) {
			if (forbidden) res->writeStatus("403 Forbidden")->end("Forbidden");
			else res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}
		ServeResolvedFile(res, *resolved, ContentTypeForPath(relPath));
	});

	app.get("/*", [](auto* res, auto* req) {
		std::string url(req->getUrl());
		if (url == "/") url = "/index.html";
		bool forbidden = false;
		auto resolved = ResolveFileUnderBase("public", url, forbidden);
		if (!resolved) {
			if (forbidden) res->writeStatus("403 Forbidden")->end("Forbidden");
			else res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}
		ServeResolvedFile(res, *resolved, ContentTypeForPath(url));
	});
}

void SignalingServerHttp::StartBackgroundTimers(
	SignalingServer& server,
	uWS::Loop* loop,
	us_listen_socket_t* listenSocket,
	us_timer_t*& statsTimer,
	us_timer_t*& redisTimer,
	us_timer_t*& shutdownTimer)
{
	statsTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
	SignalingServer* self = &server;
	memcpy(us_timer_ext(statsTimer), &self, sizeof(SignalingServer*));
	us_timer_set(statsTimer, [](struct us_timer_t* t) {
		SignalingServer* s;
		memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
		for (auto& wt : s->workerThreads_) {
			wt->post([wtp = wt.get()] {
				try {
					if (wtp->roomService())
						wtp->roomService()->broadcastStats();
				} catch (const std::exception& e) {
					spdlog::error("broadcastStats exception: {}", e.what());
				}
			});
		}
	}, kStatsBroadcastIntervalMs, kStatsBroadcastIntervalMs);

	redisTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
	memcpy(us_timer_ext(redisTimer), &self, sizeof(SignalingServer*));
	us_timer_set(redisTimer, [](struct us_timer_t* t) {
		SignalingServer* s;
		memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
		if (s->registry_) {
			size_t totalRooms = 0;
			size_t maxRooms = 0;
			for (auto& wt : s->workerThreads_) {
				totalRooms += wt->roomCount();
				maxRooms += wt->maxRoomsCapacity();
			}
			std::vector<std::string> allRoomIds;
			allRoomIds.reserve(s->roomDispatch_.size());
			for (auto& [rid, assignedThread] : s->roomDispatch_) {
				(void)assignedThread;
				allRoomIds.push_back(rid);
			}
			s->enqueueRegistryTask([s, totalRooms, maxRooms, allRoomIds = std::move(allRoomIds)]() mutable {
				try {
					s->registry_->heartbeat();
					if (!allRoomIds.empty())
						s->registry_->refreshRooms(allRoomIds);
					s->registry_->updateLoad(totalRooms, maxRooms);
				} catch (const std::exception& e) {
					spdlog::error("registry heartbeat failed: {}", e.what());
				}
			}, "registry.heartbeat rooms=" + std::to_string(allRoomIds.size()));
		}
	}, kRedisHeartbeatIntervalSec * 1000, kRedisHeartbeatIntervalSec * 1000);

	struct ShutdownCtx { us_listen_socket_t* sock; };
	shutdownTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(ShutdownCtx));
	ShutdownCtx sctx{listenSocket};
	memcpy(us_timer_ext(shutdownTimer), &sctx, sizeof(ShutdownCtx));
	us_timer_set(shutdownTimer, [](struct us_timer_t* t) {
		auto* ctx = static_cast<ShutdownCtx*>(us_timer_ext(t));
		if (g_shutdown && ctx && ctx->sock) {
			us_listen_socket_close(0, ctx->sock);
			ctx->sock = nullptr;
		}
	}, kShutdownPollIntervalMs, kShutdownPollIntervalMs);
}

} // namespace mediasoup
