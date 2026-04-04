#pragma once
#include "RoomManager.h"
#include "RoomRegistry.h"
#include "WebRtcTransport.h"
#include "Consumer.h"
#include "Producer.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>

namespace mediasoup {

using json = nlohmann::json;

class RoomService {
public:
	using NotifyFn = std::function<void(const std::string&, const json&)>;
	using BroadcastFn = std::function<void(const std::string&, const std::string&, const json&)>;

	RoomService(RoomManager& roomManager, RoomRegistry* registry)
		: roomManager_(roomManager), registry_(registry)
		, logger_(Logger::Get("RoomService")) {}

	void setNotify(NotifyFn fn) { notify_ = std::move(fn); }
	void setBroadcast(BroadcastFn fn) { broadcast_ = std::move(fn); }

	struct Result {
		bool ok = true;
		json data;
		std::string redirect;
		std::string error;
	};

	Result join(const std::string& roomId, const std::string& peerId,
		const std::string& displayName, const json& rtpCapabilities)
	{
		if (registry_) {
			std::string addr = registry_->claimRoom(roomId);
			if (!addr.empty()) return {false, {}, addr, ""};
		}

		auto room = roomManager_.createRoom(roomId);
		auto peer = std::make_shared<Peer>();
		peer->id = peerId;
		peer->displayName = displayName;
		if (!rtpCapabilities.empty())
			peer->rtpCapabilities = rtpCapabilities.get<RtpCapabilities>();
		room->addPeer(peer);

		if (registry_) registry_->refreshRoom(roomId);

		json existingProducers = json::array();
		for (auto& other : room->getOtherPeers(peerId)) {
			for (auto& [pid, prod] : other->producers) {
				existingProducers.push_back({
					{"producerId", prod->id()}, {"producerPeerId", other->id},
					{"kind", prod->kind()}
				});
			}
		}

		if (broadcast_) {
			broadcast_(roomId, peerId, {
				{"notification", true}, {"method", "peerJoined"},
				{"data", {{"peerId", peerId}, {"displayName", displayName}}}
			});
		}

		return {true, {
			{"routerRtpCapabilities", room->router()->rtpCapabilities()},
			{"existingProducers", existingProducers},
			{"participants", room->getParticipants()}
		}};
	}

	Result leave(const std::string& roomId, const std::string& peerId) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {true, {}};

		room->removePeer(peerId);

		if (broadcast_) {
			broadcast_(roomId, peerId, {
				{"notification", true}, {"method", "peerLeft"},
				{"data", {{"peerId", peerId}}}
			});
		}

		if (room->empty()) {
			if (registry_) registry_->unregisterRoom(roomId);
			roomManager_.removeRoom(roomId);
		}
		return {true, {}};
	}

	Result createTransport(const std::string& roomId, const std::string& peerId,
		bool producing, bool consuming)
	{
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto peer = room->getPeer(peerId);
		if (!peer) return {false, {}, "", "peer not found"};

		WebRtcTransportOptions opts;
		opts.listenInfos = roomManager_.listenInfos();
		opts.enableUdp = true;
		opts.enableTcp = true;
		opts.preferUdp = true;

		auto transport = room->router()->createWebRtcTransport(opts);
		if (producing) peer->sendTransport = transport;
		else           peer->recvTransport = transport;

		json result = transport->toJson();

		// When recvTransport is created, auto-subscribe to all existing producers
		if (!producing && peer->recvTransport) {
			json consumers = json::array();
			for (auto& other : room->getOtherPeers(peerId)) {
				for (auto& [pid, prod] : other->producers) {
					try {
						json consumeOpts = {
							{"producerId", prod->id()},
							{"rtpCapabilities", peer->rtpCapabilities},
							{"consumableRtpParameters", prod->consumableRtpParameters()}
						};
						auto consumer = peer->recvTransport->consume(consumeOpts);
						peer->consumers[consumer->id()] = consumer;
						consumers.push_back({
							{"peerId", other->id}, {"producerId", prod->id()},
							{"id", consumer->id()}, {"kind", consumer->kind()},
							{"rtpParameters", consumer->rtpParameters()},
							{"producerPaused", prod->paused()}
						});
					} catch (const std::exception& e) {
						MS_ERROR(logger_, "auto-subscribe on createTransport FAILED for producer {}: {}", pid, e.what());
					}
				}
			}
			result["consumers"] = consumers;
		}

		return {true, result};
	}

	Result connectTransport(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const DtlsParameters& dtlsParams)
	{
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto peer = room->getPeer(peerId);
		if (!peer) return {false, {}, "", "peer not found"};
		auto transport = peer->getTransport(transportId);
		if (!transport) return {false, {}, "", "transport not found"};
		auto wt = std::dynamic_pointer_cast<WebRtcTransport>(transport);
		if (!wt) return {false, {}, "", "not a WebRtcTransport"};
		return {true, wt->connect(dtlsParams)};
	}

	Result produce(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const std::string& kind,
		const json& rtpParameters)
	{
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto peer = room->getPeer(peerId);
		if (!peer) return {false, {}, "", "peer not found"};
		auto transport = peer->getTransport(transportId);
		if (!transport) return {false, {}, "", "transport not found"};

		json produceOpts = {
			{"kind", kind}, {"rtpParameters", rtpParameters},
			{"routerRtpCapabilities", room->router()->rtpCapabilities()}
		};
		auto producer = transport->produce(produceOpts);
		room->router()->addProducer(producer);
		peer->producers[producer->id()] = producer;

		// Server-driven auto-subscribe
		auto others = room->getOtherPeers(peerId);
		MS_DEBUG(logger_, "auto-subscribe: producer {} by peer {}, otherPeers={}", producer->id(), peerId, others.size());
		for (auto& other : others) {
			MS_DEBUG(logger_, "  peer {} recvTransport={}", other->id, other->recvTransport ? "yes" : "NO");
			if (!other->recvTransport) continue;
			try {
				MS_DEBUG(logger_, "  creating consumer on recvTransport {} for producer {}", other->recvTransport->id(), producer->id());
				MS_DEBUG(logger_, "  rtpCaps codecs count={}", other->rtpCapabilities.codecs.size());
				json consumeOpts = {
					{"producerId", producer->id()},
					{"rtpCapabilities", other->rtpCapabilities},
					{"consumableRtpParameters", producer->consumableRtpParameters()}
				};
				auto consumer = other->recvTransport->consume(consumeOpts);
				other->consumers[consumer->id()] = consumer;

				if (notify_) {
					notify_(other->id, {
						{"notification", true}, {"method", "newConsumer"},
						{"data", {
							{"peerId", peerId}, {"producerId", producer->id()},
							{"id", consumer->id()}, {"kind", consumer->kind()},
							{"rtpParameters", consumer->rtpParameters()},
							{"producerPaused", producer->paused()}
						}}
					});
				}
			} catch (const std::exception& e) {
				MS_ERROR(logger_, "auto-subscribe FAILED for peer {}: {}", other->id, e.what());
			}
		}

		return {true, {{"id", producer->id()}}};
	}

	Result consume(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const std::string& producerId,
		const json& rtpCapabilities)
	{
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto peer = room->getPeer(peerId);
		if (!peer) return {false, {}, "", "peer not found"};
		auto transport = peer->getTransport(transportId);
		if (!transport) return {false, {}, "", "transport not found"};
		auto producer = room->router()->getProducerById(producerId);
		if (!producer) return {false, {}, "", "producer not found"};

		json consumeOpts = {
			{"producerId", producerId},
			{"rtpCapabilities", rtpCapabilities},
			{"consumableRtpParameters", producer->consumableRtpParameters()}
		};
		auto consumer = transport->consume(consumeOpts);
		peer->consumers[consumer->id()] = consumer;
		return {true, consumer->toJson()};
	}

	Result pauseProducer(const std::string& roomId, const std::string& producerId) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto producer = room->router()->getProducerById(producerId);
		if (producer) producer->pause();
		return {true, {}};
	}

	Result resumeProducer(const std::string& roomId, const std::string& producerId) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto producer = room->router()->getProducerById(producerId);
		if (producer) producer->resume();
		return {true, {}};
	}

	Result restartIce(const std::string& roomId, const std::string& peerId,
		const std::string& transportId)
	{
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {false, {}, "", "room not found"};
		auto peer = room->getPeer(peerId);
		if (!peer) return {false, {}, "", "peer not found"};
		auto wt = std::dynamic_pointer_cast<WebRtcTransport>(peer->getTransport(transportId));
		if (!wt) return {false, {}, "", "transport not found"};
		return {true, wt->restartIce()};
	}

	void cleanIdleRooms(int idleSeconds = 30) {
		for (auto& id : roomManager_.getIdleRooms(idleSeconds)) {
			MS_DEBUG(logger_, "GC idle room: {}", id);
			if (registry_) registry_->unregisterRoom(id);
			roomManager_.removeRoom(id);
		}
	}

private:
	RoomManager& roomManager_;
	RoomRegistry* registry_;
	NotifyFn notify_;
	BroadcastFn broadcast_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
