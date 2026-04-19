#pragma once

#include "Logger.h"
#include "RoomManager.h"
#include "Transport.h"
#include <functional>
#include <memory>
#include <string>

namespace mediasoup::roommedia {

using NotifyFn = std::function<void(const std::string&, const std::string&, const json&)>;

inline void TrackPeerProducer(
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<Producer>& producer)
{
	if (!peer || !producer) {
		return;
	}

	peer->producers[producer->id()] = producer;

	std::weak_ptr<Peer> weakPeer = peer;
	const std::string producerId = producer->id();
	producer->emitter().once("@close", [weakPeer, producerId](const std::vector<std::any>&) {
		if (auto lockedPeer = weakPeer.lock()) {
			lockedPeer->producers.erase(producerId);
		}
	});
}

inline void TrackPeerConsumer(
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<Consumer>& consumer)
{
	if (!peer || !consumer) {
		return;
	}

	peer->consumers[consumer->id()] = consumer;

	std::weak_ptr<Peer> weakPeer = peer;
	const std::string consumerId = consumer->id();
	consumer->emitter().once("@close", [weakPeer, consumerId](const std::vector<std::any>&) {
		if (auto lockedPeer = weakPeer.lock()) {
			lockedPeer->consumers.erase(consumerId);
		}
	});
}

inline json BuildConsumerData(
	const std::string& peerId,
	const std::shared_ptr<Producer>& producer,
	const std::shared_ptr<Consumer>& consumer,
	bool includeProducerPaused = true)
{
	json data = {
		{"peerId", peerId},
		{"producerId", producer->id()},
		{"id", consumer->id()},
		{"kind", consumer->kind()},
		{"rtpParameters", consumer->rtpParameters()}
	};
	if (includeProducerPaused) {
		data["producerPaused"] = producer->paused();
	}

	return data;
}

inline json ConsumeExistingProducers(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Room>& room,
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<Transport>& transport,
	const std::shared_ptr<spdlog::logger>& logger,
	const char* failureContext,
	bool includeProducerPaused = true)
{
	json consumers = json::array();
	if (!room || !peer || !transport) {
		return consumers;
	}

	for (const auto& other : room->getOtherPeers(peerId)) {
		for (const auto& [producerKey, producer] : other->producers) {
			try {
				json consumeOpts = {
					{"producerId", producer->id()},
					{"rtpCapabilities", peer->rtpCapabilities},
					{"consumableRtpParameters", producer->consumableRtpParameters()}
				};
				auto consumer = transport->consume(consumeOpts);
				TrackPeerConsumer(peer, consumer);
				consumers.push_back(BuildConsumerData(
					other->id,
					producer,
					consumer,
					includeProducerPaused));
			} catch (const std::exception& e) {
				MS_ERROR(logger, "[{} {}] {} FAILED for producer {}: {}",
					roomId, peerId, failureContext, producerKey, e.what());
			}
		}
	}

	return consumers;
}

inline std::shared_ptr<Transport> ResolveSubscriberTransport(
	const std::shared_ptr<Peer>& peer,
	bool allowPlainFallback)
{
	if (!peer) {
		return nullptr;
	}
	if (peer->recvTransport) {
		return std::static_pointer_cast<Transport>(peer->recvTransport);
	}
	if (allowPlainFallback && peer->plainRecvTransport) {
		return std::static_pointer_cast<Transport>(peer->plainRecvTransport);
	}

	return nullptr;
}

inline void AutoSubscribeProducerToOtherPeers(
	const std::string& roomId,
	const std::string& producerPeerId,
	const std::shared_ptr<Room>& room,
	const std::shared_ptr<Producer>& producer,
	const std::shared_ptr<spdlog::logger>& logger,
	const NotifyFn& notify,
	bool allowPlainFallback)
{
	if (!room || !producer) {
		return;
	}

	for (const auto& other : room->getOtherPeers(producerPeerId)) {
		auto recvTransport = ResolveSubscriberTransport(other, allowPlainFallback);
		if (!recvTransport) {
			MS_DEBUG(logger, "[{} {}] auto-subscribe skip {}: no compatible recv transport",
				roomId, producerPeerId, other->id);
			continue;
		}

		MS_DEBUG(logger, "[{} {}] auto-subscribe → {} for producer {}",
			roomId, producerPeerId, other->id, producer->id());

		try {
			json consumeOpts = {
				{"producerId", producer->id()},
				{"rtpCapabilities", other->rtpCapabilities},
				{"consumableRtpParameters", producer->consumableRtpParameters()}
			};
			auto consumer = recvTransport->consume(consumeOpts);
			TrackPeerConsumer(other, consumer);

			if (notify) {
				notify(roomId, other->id, {
					{"notification", true},
					{"method", "newConsumer"},
					{"data", BuildConsumerData(producerPeerId, producer, consumer)}
				});
			}
		} catch (const std::exception& e) {
			MS_ERROR(logger, "[{} {}] auto-subscribe FAILED for {}: {}",
				roomId, producerPeerId, other->id, e.what());
		}
	}
}

} // namespace mediasoup::roommedia
