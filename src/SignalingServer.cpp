#include "SignalingServer.h"
#include "RoomManager.h"
#include "Producer.h"
#include "Consumer.h"
#include <App.h>
#include <nlohmann/json.hpp>
#include <fstream>

namespace mediasoup {

struct PerSocketData {
	std::string peerId;
	std::string roomId;
};

SignalingServer::SignalingServer(int port, RoomManager& roomManager)
	: port_(port), roomManager_(roomManager) {}

void SignalingServer::run() {
	running_ = true;

	uWS::App().ws<PerSocketData>("/ws", {
		.compression = uWS::DISABLED,
		.maxPayloadLength = 16 * 1024,
		.idleTimeout = 120,

		.open = [this](auto* ws) {
			auto* data = ws->getUserData();
			data->peerId = "";
			data->roomId = "";
		},

		.message = [this](auto* ws, std::string_view message, uWS::OpCode) {
			try {
				auto req = json::parse(message);
				if (!req.contains("request") || !req["request"].get<bool>()) return;

				std::string method = req.value("method", "");
				uint64_t id = req.value("id", 0);
				json data = req.value("data", json::object());
				auto* socketData = ws->getUserData();

				json responseData;
				bool ok = true;
				std::string errorMsg;

				try {
					if (method == "join") {
						std::string roomId = data.value("roomId", "default");
						std::string peerId = data.value("peerId", "");
						socketData->roomId = roomId;
						socketData->peerId = peerId;

						auto room = roomManager_.getOrCreateRoom(roomId);
						auto rtpCaps = room->router()->rtpCapabilities();

						// Collect existing producers for the new peer
						json existingProducers = json::array();
						auto router = room->router();
						for (auto& [tid, transport] : router->transports()) {
							for (auto& [pid, producer] : transport->producers()) {
								existingProducers.push_back({
									{"producerId", producer->id()},
									{"kind", producer->kind()}
								});
							}
						}

						responseData = {
							{"routerRtpCapabilities", rtpCaps},
							{"existingProducers", existingProducers}
						};

					} else if (method == "createWebRtcTransport") {
						auto room = roomManager_.getRoom(socketData->roomId);
						if (!room) throw std::runtime_error("room not found");

						bool producing = data.value("producing", false);
						bool consuming = data.value("consuming", false);

						WebRtcTransportOptions opts;
						opts.listenInfos = roomManager_.listenInfos();
						opts.enableUdp = true;
						opts.enableTcp = true;
						opts.preferUdp = true;

						auto transport = room->router()->createWebRtcTransport(opts);

						// Store transport for this peer
						room->addTransport(socketData->peerId, transport);

						responseData = transport->toJson();

					} else if (method == "connectWebRtcTransport") {
						auto room = roomManager_.getRoom(socketData->roomId);
						if (!room) throw std::runtime_error("room not found");

						std::string transportId = data.at("transportId").get<std::string>();
						DtlsParameters dtlsParams = data.at("dtlsParameters").get<DtlsParameters>();

						auto transport = room->getTransport(transportId);
						if (!transport) throw std::runtime_error("transport not found");

						auto webRtcTransport = std::dynamic_pointer_cast<WebRtcTransport>(transport);
						if (!webRtcTransport) throw std::runtime_error("not a WebRtcTransport");

						responseData = webRtcTransport->connect(dtlsParams);

					} else if (method == "produce") {
						auto room = roomManager_.getRoom(socketData->roomId);
						if (!room) throw std::runtime_error("room not found");

						std::string transportId = data.at("transportId").get<std::string>();
						auto transport = room->getTransport(transportId);
						if (!transport) throw std::runtime_error("transport not found");

						json produceOptions = {
							{"kind", data.at("kind")},
							{"rtpParameters", data.at("rtpParameters")},
							{"routerRtpCapabilities", room->router()->rtpCapabilities()}
						};

						auto producer = transport->produce(produceOptions);
						room->router()->addProducer(producer);

						responseData = {{"id", producer->id()}};

						// Notify other peers about new producer
						std::string producerPeerId = socketData->peerId;
						json notification = {
							{"notification", true},
							{"method", "newProducer"},
							{"data", {
								{"producerId", producer->id()},
								{"producerPeerId", producerPeerId},
								{"kind", producer->kind()}
							}}
						};
						std::string notifStr = notification.dump();

						// Broadcast to room (uWS publish)
						ws->publish(socketData->roomId, notifStr, uWS::OpCode::TEXT);

					} else if (method == "consume") {
						auto room = roomManager_.getRoom(socketData->roomId);
						if (!room) throw std::runtime_error("room not found");

						std::string transportId = data.at("transportId").get<std::string>();
						std::string producerId = data.at("producerId").get<std::string>();
						RtpCapabilities rtpCapabilities = data.at("rtpCapabilities").get<RtpCapabilities>();

						auto transport = room->getTransport(transportId);
						if (!transport) throw std::runtime_error("transport not found");

						auto producer = room->router()->getProducerById(producerId);
						if (!producer) throw std::runtime_error("producer not found");

						json consumeOptions = {
							{"producerId", producerId},
							{"rtpCapabilities", rtpCapabilities},
							{"consumableRtpParameters", producer->consumableRtpParameters()}
						};

						auto consumer = transport->consume(consumeOptions);

						responseData = consumer->toJson();

					} else if (method == "pauseProducer") {
						auto room = roomManager_.getRoom(socketData->roomId);
						auto producer = room->router()->getProducerById(data.at("producerId").get<std::string>());
						if (producer) producer->pause();
						responseData = json::object();

					} else if (method == "resumeProducer") {
						auto room = roomManager_.getRoom(socketData->roomId);
						auto producer = room->router()->getProducerById(data.at("producerId").get<std::string>());
						if (producer) producer->resume();
						responseData = json::object();

					} else if (method == "pauseConsumer") {
						// Consumer pause handled via transport
						responseData = json::object();

					} else if (method == "resumeConsumer") {
						responseData = json::object();

					} else if (method == "restartIce") {
						auto room = roomManager_.getRoom(socketData->roomId);
						std::string transportId = data.at("transportId").get<std::string>();
						auto transport = room->getTransport(transportId);
						auto webRtcTransport = std::dynamic_pointer_cast<WebRtcTransport>(transport);
						if (webRtcTransport) responseData = webRtcTransport->restartIce();
						else responseData = json::object();

					} else {
						throw std::runtime_error("unknown method: " + method);
					}
				} catch (const std::exception& e) {
					ok = false;
					errorMsg = e.what();
				}

				json response;
				if (ok) {
					response = {{"response", true}, {"id", id}, {"ok", true}, {"data", responseData}};
				} else {
					response = {{"response", true}, {"id", id}, {"ok", false}, {"error", errorMsg}};
				}

				ws->send(response.dump(), uWS::OpCode::TEXT);

				// Subscribe to room topic after join
				if (method == "join" && ok) {
					ws->subscribe(socketData->roomId);
				}

			} catch (const std::exception& e) {
				json errResp = {{"response", true}, {"id", 0}, {"ok", false}, {"error", e.what()}};
				ws->send(errResp.dump(), uWS::OpCode::TEXT);
			}
		},

		.close = [this](auto* ws, int, std::string_view) {
			auto* data = ws->getUserData();
			if (!data->roomId.empty() && !data->peerId.empty()) {
				auto room = roomManager_.getRoom(data->roomId);
				if (room) {
					room->removePeer(data->peerId);
				}
			}
		}
	}).get("/*", [](auto* res, auto* req) {
		// Serve static files from public/
		std::string url(req->getUrl());
		if (url == "/") url = "/index.html";

		std::string path = "public" + url;
		std::ifstream file(path, std::ios::binary);
		if (!file.is_open()) {
			res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}

		std::string content((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());

		// Content type
		std::string ct = "text/plain";
		if (url.find(".html") != std::string::npos) ct = "text/html";
		else if (url.find(".js") != std::string::npos) ct = "application/javascript";
		else if (url.find(".css") != std::string::npos) ct = "text/css";

		res->writeHeader("Content-Type", ct)->end(content);

	}).listen(port_, [this](auto* listenSocket) {
		if (listenSocket) {
			spdlog::info("SignalingServer listening on port {}", port_);
		} else {
			spdlog::error("SignalingServer failed to listen on port {}", port_);
		}
	}).run();
}

void SignalingServer::stop() {
	running_ = false;
}

} // namespace mediasoup
