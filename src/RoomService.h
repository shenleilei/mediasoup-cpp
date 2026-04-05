#pragma once
#include "RoomManager.h"
#include "RoomRegistry.h"
#include "WebRtcTransport.h"
#include "PipeTransport.h"
#include "PlainTransport.h"
#include "Consumer.h"
#include "Producer.h"
#include "Recorder.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>
#include <sys/stat.h>

namespace mediasoup {

using json = nlohmann::json;

class RoomService {
public:
	using NotifyFn = std::function<void(const std::string&, const json&)>;
	using BroadcastFn = std::function<void(const std::string&, const std::string&, const json&)>;

	RoomService(RoomManager& roomManager, RoomRegistry* registry,
		const std::string& recordDir = "")
		: roomManager_(roomManager), registry_(registry)
		, recordDir_(recordDir)
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

		// Stop recorder for this peer
		{
			std::string key = roomId + "/" + peerId;
			std::lock_guard<std::mutex> lock(recorderMutex_);
			auto it = recorders_.find(key);
			if (it != recorders_.end()) {
				it->second->stop();
				recorders_.erase(it);
			}
			recorderTransports_.erase(key);
		}

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

		// Auto-record if enabled
		autoRecord(roomId, peerId, room, producer);

		return {true, {{"id", producer->id()}}};
	}

	// Auto-record: called after produce, creates PlainTransport + consume → PeerRecorder
	void autoRecord(const std::string& roomId, const std::string& peerId,
		std::shared_ptr<Room> room, std::shared_ptr<Producer> producer)
	{
		if (recordDir_.empty()) return;

		std::string key = roomId + "/" + peerId;
		std::lock_guard<std::mutex> lock(recorderMutex_);

		// Get or create recorder for this peer
		auto& rec = recorders_[key];
		if (!rec) {
			// Determine PT and codec from router capabilities
			uint8_t audioPT = 100, videoPT = 101;
			VideoCodec videoCodec = VideoCodec::VP8;
			auto caps = room->router()->rtpCapabilities();
			for (auto& c : caps.codecs) {
				if (c.mimeType.find("/rtx") != std::string::npos) continue;
				if (c.mimeType.find("opus") != std::string::npos) {
					audioPT = c.preferredPayloadType;
				} else if (c.mimeType.find("H264") != std::string::npos || c.mimeType.find("h264") != std::string::npos) {
					if (videoCodec != VideoCodec::H264) {
						videoPT = c.preferredPayloadType;
						videoCodec = VideoCodec::H264;
					}
				} else if (c.mimeType.find("VP8") != std::string::npos) {
					if (videoCodec != VideoCodec::H264) {
						videoPT = c.preferredPayloadType;
						videoCodec = VideoCodec::VP8;
					}
				}
			}
			std::string dir = recordDir_ + "/" + roomId;
			mkdir(dir.c_str(), 0755);
			auto ts = std::chrono::system_clock::now().time_since_epoch();
			auto secs = std::chrono::duration_cast<std::chrono::seconds>(ts).count();
			std::string path = dir + "/" + peerId + "_" + std::to_string(secs) + ".webm";

			rec = std::make_shared<PeerRecorder>(peerId, path, audioPT, videoPT, 48000, 90000, videoCodec);
			int port = rec->createSocket();
			if (port < 0) {
				MS_ERROR(logger_, "Failed to create recorder socket for {}", key);
				rec.reset(); return;
			}
			if (!rec->start()) {
				MS_ERROR(logger_, "Failed to start recorder for {}", key);
				rec.reset(); return;
			}

			// Create a PlainTransport for recording (local loopback only)
			PlainTransportOptions opts;
			opts.listenInfos = {{{"ip", "127.0.0.1"}}};
			opts.rtcpMux = true;
			opts.comedia = false;
			auto pt = room->router()->createPlainTransport(opts);
			recorderTransports_[key] = pt;

			// Connect PlainTransport to recorder's UDP port
			auto connResult = pt->connect("127.0.0.1", port);
			MS_DEBUG(logger_, "Recorder PlainTransport connect result: {}", connResult.dump());
			MS_DEBUG(logger_, "Recorder started for {} → port {} file {}", key, port, path);
		}

		// Consume this producer on the PlainTransport
		auto pt = recorderTransports_[key];
		if (!pt) return;

		try {
			// Use pipe consumer for PlainTransport (no capability negotiation needed)
			json consumeOpts = {
				{"producerId", producer->id()},
				{"rtpCapabilities", room->router()->rtpCapabilities()},
				{"consumableRtpParameters", producer->consumableRtpParameters()},
				{"pipe", true}
			};
			auto consumer = pt->consume(consumeOpts);
			MS_DEBUG(logger_, "Recording consumer created for producer {} kind={}", producer->id(), producer->kind());
		} catch (const std::exception& e) {
			MS_ERROR(logger_, "Failed to create recording consumer: {}", e.what());
		}
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

	// Collect full-chain stats for one peer (sendTransport + all producers)
	json collectPeerStats(const std::string& roomId, const std::string& peerId) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) return {};
		auto peer = room->getPeer(peerId);
		if (!peer) return {};

		json result = {{"peerId", peerId}};

		// sendTransport stats (vehicle uplink)
		if (peer->sendTransport) {
			try { result["sendTransport"] = peer->sendTransport->getStats(); }
			catch (...) { result["sendTransport"] = nullptr; }
		}

		// recvTransport stats (operator downlink)
		if (peer->recvTransport) {
			try { result["recvTransport"] = peer->recvTransport->getStats(); }
			catch (...) { result["recvTransport"] = nullptr; }
		}

		// Producer stats + scores
		json producers = json::object();
		for (auto& [pid, prod] : peer->producers) {
			json pstat;
			try { pstat["stats"] = prod->getStats(); }
			catch (...) { pstat["stats"] = nullptr; }
			// Scores from PRODUCER_SCORE notifications
			json scores = json::array();
			for (auto& s : prod->scores())
				scores.push_back({{"ssrc", s.ssrc}, {"score", s.score}, {"rid", s.rid}});
			pstat["scores"] = scores;
			pstat["kind"] = prod->kind();
			producers[pid] = pstat;
		}
		result["producers"] = producers;

		// Consumer scores (from CONSUMER_SCORE notifications)
		json consumers = json::object();
		for (auto& [cid, cons] : peer->consumers) {
			auto& sc = cons->currentScore();
			consumers[cid] = {
				{"kind", cons->kind()}, {"producerId", cons->producerId()},
				{"score", sc.score}, {"producerScore", sc.producerScore}
			};
		}
		result["consumers"] = consumers;

		return result;
	}

	// Broadcast stats for all peers in all rooms
	void broadcastStats() {
		auto roomIds = roomManager_.getRoomIds();
		MS_DEBUG(logger_, "broadcastStats: {} rooms", roomIds.size());
		for (auto& roomId : roomIds) {
			auto room = roomManager_.getRoom(roomId);
			if (!room) continue;

			// Collect stats for each peer
			json allStats = json::array();
			auto participants = room->getPeerIds();
			for (auto& peerId : participants) {
				try {
				auto stats = collectPeerStats(roomId, peerId);
				if (!stats.empty()) allStats.push_back(stats);
			} catch (...) {
				// Still record a minimal entry so recorder gets QoS snapshots
				allStats.push_back({{"peerId", peerId}});
			}
			}

			if (allStats.empty()) continue;

			// Push to all peers in the room
			if (broadcast_) {
				broadcast_(roomId, "", {
					{"notification", true}, {"method", "statsReport"},
					{"data", {{"roomId", roomId}, {"peers", allStats}}}
				});
			}

			// Write QoS snapshots to active recorders
			{
				std::lock_guard<std::mutex> lock(recorderMutex_);
				for (auto& peerStats : allStats) {
					std::string key = roomId + "/" + peerStats.value("peerId", "");
					auto it = recorders_.find(key);
					if (it != recorders_.end() && it->second)
						it->second->appendQosSnapshot(peerStats);
				}
			}
		}
	}

private:
	RoomManager& roomManager_;
	RoomRegistry* registry_;
	std::string recordDir_;
	NotifyFn notify_;
	BroadcastFn broadcast_;
	std::shared_ptr<spdlog::logger> logger_;

	std::mutex recorderMutex_;
	std::unordered_map<std::string, std::shared_ptr<PeerRecorder>> recorders_;
	std::unordered_map<std::string, std::shared_ptr<PlainTransport>> recorderTransports_;
};

} // namespace mediasoup
