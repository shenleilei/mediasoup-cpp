#pragma once

#include "Logger.h"
#include "RoomManager.h"
#include "Transport.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace mediasoup::roommedia {

using NotifyFn = std::function<void(const std::string&, const std::string&, const json&)>;

inline std::vector<uint32_t> CollectProducerSsrcs(const std::shared_ptr<Producer>& producer)
{
	std::vector<uint32_t> ssrcs;
	if (!producer) {
		return ssrcs;
	}

	std::unordered_set<uint32_t> seen;
	for (const auto& encoding : producer->rtpParameters().encodings) {
		if (!encoding.ssrc) {
			continue;
		}
		if (seen.insert(*encoding.ssrc).second) {
			ssrcs.push_back(*encoding.ssrc);
		}
	}

	return ssrcs;
}

inline void RegisterPeerProducerSourceMapping(
	const std::shared_ptr<Peer>& peer,
	const std::string& source,
	const std::vector<uint32_t>& ssrcs)
{
	if (!peer || source.empty() || ssrcs.empty()) {
		return;
	}

	auto& mappedSsrcs = peer->sourceSsrcs[source];
	for (uint32_t ssrc : ssrcs) {
		mappedSsrcs.insert(ssrc);
		peer->ssrcSource[ssrc] = source;
	}
}

inline void RemovePeerProducerSourceMapping(
	const std::shared_ptr<Peer>& peer,
	const std::string& source,
	const std::vector<uint32_t>& ssrcs)
{
	if (!peer || source.empty() || ssrcs.empty()) {
		return;
	}

	auto sourceIt = peer->sourceSsrcs.find(source);
	for (uint32_t ssrc : ssrcs) {
		auto reverseIt = peer->ssrcSource.find(ssrc);
		if (reverseIt != peer->ssrcSource.end() && reverseIt->second == source) {
			peer->ssrcSource.erase(reverseIt);
		}
		if (sourceIt != peer->sourceSsrcs.end()) {
			sourceIt->second.erase(ssrc);
		}
	}

	if (sourceIt != peer->sourceSsrcs.end() && sourceIt->second.empty()) {
		peer->sourceSsrcs.erase(sourceIt);
	}
}

inline void TrackPeerProducer(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<Producer>& producer,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (!peer || !producer) {
		return;
	}

	peer->producers[producer->id()] = producer;
	const std::string source = producer->source();
	const auto ssrcs = CollectProducerSsrcs(producer);
	RegisterPeerProducerSourceMapping(peer, source, ssrcs);

	std::weak_ptr<Peer> weakPeer = peer;
	const std::string producerId = producer->id();
	const std::string kind = producer->kind();
	producer->emitter().once("@close", [weakPeer, producerId, source, ssrcs](const std::vector<std::any>&) {
		if (auto lockedPeer = weakPeer.lock()) {
			lockedPeer->producers.erase(producerId);
			RemovePeerProducerSourceMapping(lockedPeer, source, ssrcs);
		}
	});
	producer->emitter().once("@close", [weakPeer, roomId, peerId, producerId, kind, source, logger](const std::vector<std::any>& args) {
		std::string reason = "close";
		if (!args.empty()) {
			try {
				reason = std::any_cast<std::string>(args[0]);
			} catch (...) {
				reason = "close";
			}
		}
		size_t remainingProducers = 0;
		if (auto lockedPeer = weakPeer.lock()) {
			remainingProducers = lockedPeer->producers.size();
		}
		if (logger) {
			MS_INFO(logger, "[{} {}] producer closed producerId={} kind={} source={} reason={} remainingProducers={}",
				roomId, peerId, producerId, kind, source.empty() ? "-" : source, reason, remainingProducers);
		}
	});
}

inline void TrackPeerConsumer(
	const std::string& roomId,
	const std::string& peerId,
	const std::shared_ptr<Peer>& peer,
	const std::shared_ptr<Consumer>& consumer,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (!peer || !consumer) {
		return;
	}

	peer->consumers[consumer->id()] = consumer;

	std::weak_ptr<Peer> weakPeer = peer;
	const std::string consumerId = consumer->id();
	const std::string producerId = consumer->producerId();
	const std::string kind = consumer->kind();
	consumer->emitter().once("@close", [weakPeer, consumerId](const std::vector<std::any>&) {
		if (auto lockedPeer = weakPeer.lock()) {
			lockedPeer->consumers.erase(consumerId);
		}
	});
	consumer->emitter().once("@close", [weakPeer, roomId, peerId, consumerId, producerId, kind, logger](const std::vector<std::any>& args) {
		std::string reason = "close";
		if (!args.empty()) {
			try {
				reason = std::any_cast<std::string>(args[0]);
			} catch (...) {
				reason = "close";
			}
		}
		size_t remainingConsumers = 0;
		if (auto lockedPeer = weakPeer.lock()) {
			remainingConsumers = lockedPeer->consumers.size();
		}
		if (logger) {
			MS_INFO(logger, "[{} {}] consumer closed consumerId={} producerId={} kind={} reason={} remainingConsumers={}",
				roomId, peerId, consumerId, producerId, kind, reason, remainingConsumers);
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

inline bool HasConsumerRtpCapabilities(const std::shared_ptr<Peer>& peer)
{
	return peer && !peer->rtpCapabilities.codecs.empty();
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
	if (!HasConsumerRtpCapabilities(peer)) {
		MS_DEBUG(logger, "[{} {}] {} skipped: peer has empty rtpCapabilities",
			roomId, peerId, failureContext);
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
				TrackPeerConsumer(roomId, peerId, peer, consumer, logger);
				if (consumer->kind() == "video") {
					try {
						consumer->requestKeyFrame();
					} catch (const std::exception& e) {
						MS_WARN(logger, "[{} {}] keyframe request failed for existing producer {}: {}",
							roomId, peerId, producerKey, e.what());
					} catch (...) {
						MS_WARN(logger, "[{} {}] keyframe request failed for existing producer {}: unknown error",
							roomId, peerId, producerKey);
					}
				}
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
			TrackPeerConsumer(roomId, other->id, other, consumer, logger);
			if (consumer->kind() == "video") {
				try {
					consumer->requestKeyFrame();
				} catch (const std::exception& e) {
					MS_WARN(logger, "[{} {}] keyframe request failed for {}: {}",
						roomId, producerPeerId, other->id, e.what());
				} catch (...) {
					MS_WARN(logger, "[{} {}] keyframe request failed for {}: unknown error",
						roomId, producerPeerId, other->id);
				}
			}

			if (notify) {
				MS_INFO(logger, "[{} {}] notify newConsumer target={} producerId={} consumerId={} kind={}",
					roomId, producerPeerId, other->id, producer->id(), consumer->id(), consumer->kind());
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
