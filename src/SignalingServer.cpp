#include "SignalingServer.h"
#include <App.h>
#include <fstream>
#include <unordered_map>
#include <mutex>

namespace mediasoup {

struct PerSocketData {
	std::string peerId;
	std::string roomId;
};

SignalingServer::SignalingServer(int port, RoomService& roomService)
	: port_(port), roomService_(roomService) {}

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
				json resp = {{"response", true}, {"id", 0}, {"ok", false}, {"error", e.what()}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
			}
		},

		.close = [this, wsMap](auto* ws, int, std::string_view) {
			auto* sd = ws->getUserData();
			if (!sd->peerId.empty()) {
				{
					std::lock_guard<std::mutex> lock(wsMap->mutex);
					wsMap->peers.erase(sd->peerId);
				}
				if (!sd->roomId.empty())
					roomService_.leave(sd->roomId, sd->peerId);
			}
		}

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
		} else {
			spdlog::error("SignalingServer failed to listen on port {}", port_);
		}
	}).run();
}

void SignalingServer::stop() {
	running_ = false;
}

} // namespace mediasoup
