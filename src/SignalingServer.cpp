#include "SignalingServer.h"
#include <App.h>
#include <fstream>
#include <unordered_map>
#include <mutex>
#include <dirent.h>
#include <sys/stat.h>

namespace mediasoup {

struct PerSocketData {
	std::string peerId;
	std::string roomId;
};

SignalingServer::SignalingServer(int port, RoomService& roomService, const std::string& recordDir)
	: port_(port), roomService_(roomService), recordDir_(recordDir) {}

void SignalingServer::run() {
	running_ = true;

	// peerId → ws* mapping for notify/broadcast
	struct WsMap {
		std::mutex mutex;
		std::unordered_map<std::string, uWS::WebSocket<false, true, PerSocketData>*> peers;
	};
	auto wsMap = std::make_shared<WsMap>();

	// Wire up RoomService callbacks
	roomService_.setNotify([wsMap](const std::string& peerId, const json& msg) {
		std::lock_guard<std::mutex> lock(wsMap->mutex);
		auto it = wsMap->peers.find(peerId);
		if (it != wsMap->peers.end())
			it->second->send(msg.dump(), uWS::OpCode::TEXT);
	});

	roomService_.setBroadcast([wsMap](const std::string& roomId,
		const std::string& excludePeerId, const json& msg)
	{
		std::string data = msg.dump();
		std::lock_guard<std::mutex> lock(wsMap->mutex);
		for (auto& [pid, ws] : wsMap->peers) {
			if (pid == excludePeerId) continue;
			auto* sd = ws->getUserData();
			if (sd->roomId == roomId)
				ws->send(data, uWS::OpCode::TEXT);
		}
	});

	// GC timer
	struct us_timer_t* gcTimer = nullptr;

	uWS::App().ws<PerSocketData>("/ws", {
		.compression = uWS::DISABLED,
		.maxPayloadLength = 16 * 1024,
		.idleTimeout = 120,

		.open = [wsMap](auto* ws) {
			auto* d = ws->getUserData();
			d->peerId = "";
			d->roomId = "";
		},

		.message = [this, wsMap](auto* ws, std::string_view message, uWS::OpCode) {
			try {
				auto req = json::parse(message);
				if (!req.contains("request") || !req["request"].get<bool>()) return;

				std::string method = req.value("method", "");
				uint64_t id = req.value("id", 0);
				json data = req.value("data", json::object());
				auto* sd = ws->getUserData();

				// Log every request with room/peer context
				if (method != "clientStats") {
					spdlog::debug("[room:{} {}] {} id={}", sd->roomId, sd->peerId, method, id);
				}

				RoomService::Result result;

				if (method == "join") {
					std::string roomId = data.value("roomId", "default");
					std::string peerId = data.value("peerId", "");
					std::string displayName = data.value("displayName", peerId);
					json rtpCaps = data.value("rtpCapabilities", json::object());

					result = roomService_.join(roomId, peerId, displayName, rtpCaps);

					if (!result.redirect.empty()) {
						// Redirect to correct node
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"redirect", result.redirect}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
						return;
					}

					if (result.ok) {
						sd->roomId = roomId;
						sd->peerId = peerId;
						spdlog::info("[room:{} {}] joined", roomId, peerId);
						std::lock_guard<std::mutex> lock(wsMap->mutex);
						wsMap->peers[peerId] = ws;
					}

				} else if (method == "createWebRtcTransport") {
					result = roomService_.createTransport(sd->roomId, sd->peerId,
						data.value("producing", false), data.value("consuming", false));

				} else if (method == "connectWebRtcTransport") {
					result = roomService_.connectTransport(sd->roomId, sd->peerId,
						data.at("transportId").get<std::string>(),
						data.at("dtlsParameters").get<DtlsParameters>());

				} else if (method == "produce") {
					result = roomService_.produce(sd->roomId, sd->peerId,
						data.at("transportId").get<std::string>(),
						data.at("kind").get<std::string>(),
						data.at("rtpParameters"));

				} else if (method == "consume") {
					result = roomService_.consume(sd->roomId, sd->peerId,
						data.at("transportId").get<std::string>(),
						data.at("producerId").get<std::string>(),
						data.at("rtpCapabilities"));

				} else if (method == "pauseProducer") {
					result = roomService_.pauseProducer(sd->roomId,
						data.at("producerId").get<std::string>());

				} else if (method == "resumeProducer") {
					result = roomService_.resumeProducer(sd->roomId,
						data.at("producerId").get<std::string>());

				} else if (method == "restartIce") {
					result = roomService_.restartIce(sd->roomId, sd->peerId,
						data.at("transportId").get<std::string>());

				} else if (method == "getStats") {
					json stats = roomService_.collectPeerStats(sd->roomId,
						data.value("peerId", sd->peerId));
					result = {true, stats};

				} else if (method == "clientStats") {
					roomService_.setClientStats(sd->roomId, sd->peerId, data);
					result = {true};

				} else {
					result = {false, {}, "", "unknown method: " + method};
				}

				json resp;
				if (result.ok)
					resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", result.data}};
				else
					resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", result.error}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);

			} catch (const std::exception& e) {
				auto* esd = ws->getUserData();
				spdlog::error("[room:{} {}] signaling error: {}", esd->roomId, esd->peerId, e.what());
				json resp = {{"response", true}, {"id", 0}, {"ok", false}, {"error", e.what()}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
			}
		},

		.close = [this, wsMap](auto* ws, int, std::string_view) {
			auto* sd = ws->getUserData();
			if (!sd->peerId.empty()) {
				spdlog::info("[room:{} {}] disconnected", sd->roomId, sd->peerId);
				{
					std::lock_guard<std::mutex> lock(wsMap->mutex);
					wsMap->peers.erase(sd->peerId);
				}
				if (!sd->roomId.empty())
					roomService_.leave(sd->roomId, sd->peerId);
			}
		}

	}).get("/api/recordings", [this](auto* res, auto*) {
		// List all recordings grouped by room
		if (recordDir_.empty()) {
			res->writeHeader("Content-Type", "application/json")->end("[]");
			return;
		}
		json rooms = json::array();
		// Scan recordDir_ for subdirectories (rooms)
		DIR* dir = opendir(recordDir_.c_str());
		if (!dir) { res->writeHeader("Content-Type", "application/json")->end("[]"); return; }
		struct dirent* ent;
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.') continue;
			std::string roomPath = recordDir_ + "/" + ent->d_name;
			struct stat st;
			if (stat(roomPath.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;
			json files = json::array();
			DIR* rdir = opendir(roomPath.c_str());
			if (!rdir) continue;
			struct dirent* rent;
			while ((rent = readdir(rdir))) {
				std::string fname(rent->d_name);
				if (fname.size() > 5 && fname.substr(fname.size()-5) == ".webm")
					files.push_back(fname);
			}
			closedir(rdir);
			if (!files.empty())
				rooms.push_back({{"roomId", std::string(ent->d_name)}, {"files", files}});
		}
		closedir(dir);
		res->writeHeader("Content-Type", "application/json")->end(rooms.dump());

	}).get("/recordings/*", [this](auto* res, auto* req) {
		// Serve recording files (webm + qos.json)
		if (recordDir_.empty()) { res->writeStatus("404 Not Found")->end("Not Found"); return; }
		std::string url(req->getUrl()); // e.g. /recordings/roomId/file.webm
		std::string relPath = url.substr(12); // strip "/recordings/"
		// Prevent path traversal
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
			gcTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(RoomService*));
			RoomService* svcPtr = &roomService_;
			memcpy(us_timer_ext(gcTimer), &svcPtr, sizeof(RoomService*));
			us_timer_set(gcTimer, [](struct us_timer_t* t) {
				RoomService* svc;
				memcpy(&svc, us_timer_ext(t), sizeof(RoomService*));
				svc->cleanIdleRooms(30);
			}, 30000, 30000);

			// Stats broadcast timer — every 2 seconds
			auto* statsTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(RoomService*));
			memcpy(us_timer_ext(statsTimer), &svcPtr, sizeof(RoomService*));
			us_timer_set(statsTimer, [](struct us_timer_t* t) {
				RoomService* svc;
				memcpy(&svc, us_timer_ext(t), sizeof(RoomService*));
				try { svc->broadcastStats(); } catch (...) {}
			}, 2000, 2000);
		} else {
			spdlog::error("SignalingServer failed to listen on port {}", port_);
		}
	}).run();
}

void SignalingServer::stop() {
	running_ = false;
}

} // namespace mediasoup
