#include "RoomService.h"
#include <algorithm>
#include <dirent.h>
#include <sys/stat.h>

namespace mediasoup {

RoomService::RoomService(RoomManager& roomManager, RoomRegistry* registry,
	const std::string& recordDir)
	: roomManager_(roomManager), registry_(registry)
	, recordDir_(recordDir)
	, logger_(Logger::Get("RoomService")) {}

// ── join / leave ──

RoomService::Result RoomService::join(const std::string& roomId, const std::string& peerId,
	const std::string& displayName, const json& rtpCapabilities, const std::string& clientIp)
{
	// Let claimRoom handle redirect first — even if we're full, the room
	// may already exist on another node and we should redirect there.
	if (registry_) {
		try {
			std::string addr = registry_->claimRoom(roomId, clientIp);
			if (!addr.empty()) return {false, {}, addr, ""};
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] claimRoom failed ({}), degrading to local", roomId, peerId, e.what());
		}
	}

	// Check local capacity only for NEW rooms (not already on this node)
	auto existingRoom = roomManager_.getRoom(roomId);
	if (!existingRoom) {
		size_t maxRooms = roomManager_.workerManager().maxTotalRouters();
		if (maxRooms > 0 && roomManager_.roomCount() >= maxRooms) {
			MS_WARN(logger_, "[{} {}] local node at capacity ({}/{})", roomId, peerId,
				roomManager_.roomCount(), maxRooms);
			if (registry_) {
				try {
					registry_->unregisterRoom(roomId);
					// Try to find another node with capacity
					auto resolved = registry_->resolveRoom(roomId, clientIp);
					if (!resolved.wsUrl.empty() && resolved.wsUrl != registry_->nodeAddress())
						return {false, {}, resolved.wsUrl, ""};
				} catch (...) {}
			}
			return {false, {}, "", "no available capacity"};
		}
	}

	if (!rtpCapabilities.is_null() && !rtpCapabilities.empty() && !rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] join validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
	}

	bool roomCreated = (existingRoom == nullptr);
	auto room = roomManager_.createRoom(roomId);
	if (roomCreated && roomLifecycle_) {
		roomLifecycle_(roomId, true);
	}
	auto peer = std::make_shared<Peer>();
	peer->id = peerId;
	peer->displayName = displayName;
	if (!rtpCapabilities.empty())
		peer->rtpCapabilities = rtpCapabilities.get<RtpCapabilities>();

	auto oldPeer = room->replacePeer(peer);
	bool isReconnect = false;
	if (oldPeer) {
		isReconnect = true;
		// Clean up old peer's producers from router
		for (auto& [pid, _] : oldPeer->producers)
			room->router()->removeProducer(pid);
		oldPeer->close();

		// Clean up recorder and client stats for old session
		std::string key = roomId + "/" + peerId;
		std::shared_ptr<PeerRecorder> recToStop;
		auto it = recorders_.find(key);
		if (it != recorders_.end()) {
			recToStop = std::move(it->second);
			recorders_.erase(it);
		}
		recorderTransports_.erase(key);
		if (recToStop) recToStop->stop();
		clientStats_.erase(key);
	}

	if (registry_) {
		auto* reg = registry_;
		std::string rid = roomId;
		postRegistryTask([reg, rid] { reg->refreshRoom(rid); });
	}

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
			{"data", {{"peerId", peerId}, {"displayName", displayName},
				{"reconnect", isReconnect}}}
		});
	}

	return {true, {
		{"routerRtpCapabilities", room->router()->rtpCapabilities()},
		{"existingProducers", existingProducers},
		{"participants", room->getParticipants()}
	}};
}

RoomService::Result RoomService::leave(const std::string& roomId, const std::string& peerId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {true, {}};

	// Stop recorder after erasing it from single-threaded service state.
	std::shared_ptr<PeerRecorder> recToStop;
	std::string key = roomId + "/" + peerId;
	auto it = recorders_.find(key);
	if (it != recorders_.end()) {
		recToStop = std::move(it->second);
		recorders_.erase(it);
	}
	recorderTransports_.erase(key);
	if (recToStop) recToStop->stop();

	clientStats_.erase(roomId + "/" + peerId);

	// Remove peer's producers from router before closing peer
	auto peer = room->getPeer(peerId);
	if (peer) {
		for (auto& [pid, _] : peer->producers)
			room->router()->removeProducer(pid);
	}

	room->removePeer(peerId);

	if (broadcast_) {
		broadcast_(roomId, peerId, {
			{"notification", true}, {"method", "peerLeft"},
			{"data", {{"peerId", peerId}}}
		});
	}

	if (room->empty()) {
		if (registry_) {
			auto* reg = registry_;
			std::string rid = roomId;
			postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
		}
		roomManager_.removeRoom(roomId);
		if (roomLifecycle_) roomLifecycle_(roomId, false);
	}
	return {true, {}};
}

// ── transport ──

RoomService::Result RoomService::createTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming)
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
					MS_ERROR(logger_, "[{} {}] auto-subscribe on createTransport FAILED for producer {}: {}",
						roomId, peerId, pid, e.what());
				}
			}
		}
		result["consumers"] = consumers;
	}

	return {true, result};
}

RoomService::Result RoomService::connectTransport(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const DtlsParameters& dtlsParams)
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

// ── produce / consume ──

RoomService::Result RoomService::produce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& kind, const json& rtpParameters)
{
	if (kind != "audio" && kind != "video") {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid kind '{}'", roomId, peerId, kind);
		return {false, {}, "", "invalid kind"};
	}
	if (!rtpParameters.is_object()) {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid rtpParameters type", roomId, peerId);
		return {false, {}, "", "invalid rtpParameters"};
	}

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
	for (auto& other : room->getOtherPeers(peerId)) {
		if (!other->recvTransport) {
			MS_DEBUG(logger_, "[{} {}] auto-subscribe skip {}: no recvTransport", roomId, peerId, other->id);
			continue;
		}
		MS_DEBUG(logger_, "[{} {}] auto-subscribe → {} for producer {}", roomId, peerId, other->id, producer->id());
		try {
			json consumeOpts = {
				{"producerId", producer->id()},
				{"rtpCapabilities", other->rtpCapabilities},
				{"consumableRtpParameters", producer->consumableRtpParameters()}
			};
			auto consumer = other->recvTransport->consume(consumeOpts);
			other->consumers[consumer->id()] = consumer;

			if (notify_) {
				notify_(roomId, other->id, {
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
			MS_ERROR(logger_, "[{} {}] auto-subscribe FAILED for {}: {}",
				roomId, peerId, other->id, e.what());
		}
	}

	autoRecord(roomId, peerId, room, producer);
	return {true, {{"id", producer->id()}}};
}

RoomService::Result RoomService::consume(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& producerId, const json& rtpCapabilities)
{
	if (!rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] consume validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
	}

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

RoomService::Result RoomService::pauseProducer(const std::string& roomId,
	const std::string& producerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (producer) producer->pause();
	return {true, {}};
}

RoomService::Result RoomService::resumeProducer(const std::string& roomId,
	const std::string& producerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (producer) producer->resume();
	return {true, {}};
}

RoomService::Result RoomService::restartIce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto wt = std::dynamic_pointer_cast<WebRtcTransport>(peer->getTransport(transportId));
	if (!wt) return {false, {}, "", "transport not found"};
	return {true, wt->restartIce()};
}

// ── auto-record ──

void RoomService::autoRecord(const std::string& roomId, const std::string& peerId,
	std::shared_ptr<Room> room, std::shared_ptr<Producer> producer)
{
	if (recordDir_.empty()) return;

	std::string key = roomId + "/" + peerId;
	auto& rec = recorders_[key];
	if (!rec) {
		uint8_t audioPT = 100, videoPT = 101;
		VideoCodec videoCodec = VideoCodec::VP8;
		auto caps = room->router()->rtpCapabilities();
		for (auto& c : caps.codecs) {
			if (c.mimeType.find("/rtx") != std::string::npos) continue;
			if (c.mimeType.find("opus") != std::string::npos) {
				audioPT = c.preferredPayloadType;
			} else if (c.mimeType.find("H264") != std::string::npos || c.mimeType.find("h264") != std::string::npos) {
				if (videoCodec != VideoCodec::H264) { videoPT = c.preferredPayloadType; videoCodec = VideoCodec::H264; }
			} else if (c.mimeType.find("VP8") != std::string::npos) {
				if (videoCodec != VideoCodec::H264) { videoPT = c.preferredPayloadType; videoCodec = VideoCodec::VP8; }
			}
		}
		std::string dir = recordDir_ + "/" + roomId;
		mkdir(dir.c_str(), 0755);
		auto ts = std::chrono::system_clock::now().time_since_epoch();
		auto secs = std::chrono::duration_cast<std::chrono::seconds>(ts).count();
		std::string path = dir + "/" + peerId + "_" + std::to_string(secs) + ".webm";

		rec = std::make_shared<PeerRecorder>(peerId, path, audioPT, videoPT, 48000, 90000, videoCodec, roomId);
		int port = rec->createSocket();
		if (port < 0) { MS_ERROR(logger_, "[{}] Failed to create recorder socket", key); rec.reset(); return; }
		if (!rec->start()) { MS_ERROR(logger_, "[{}] Failed to start recorder", key); rec.reset(); return; }

		PlainTransportOptions opts;
		opts.listenInfos = {{{"ip", "127.0.0.1"}}};
		opts.rtcpMux = true;
		opts.comedia = false;
		auto pt = room->router()->createPlainTransport(opts);
		recorderTransports_[key] = pt;
		pt->connect("127.0.0.1", port);
		MS_DEBUG(logger_, "[{}] Recorder started → port {} file {}", key, port, path);
	}

	auto pt = recorderTransports_[key];
	if (!pt) return;

	try {
		json consumeOpts = {
			{"producerId", producer->id()},
			{"rtpCapabilities", room->router()->rtpCapabilities()},
			{"consumableRtpParameters", producer->consumableRtpParameters()},
			{"pipe", true}
		};
		pt->consume(consumeOpts);
		MS_DEBUG(logger_, "[{}] Recording consumer created: producer {} kind={}", key, producer->id(), producer->kind());
	} catch (const std::exception& e) {
		MS_ERROR(logger_, "[{}] Failed to create recording consumer: {}", key, e.what());
	}
}

// ── health / GC ──

void RoomService::checkRoomHealth() {
	auto deadRooms = roomManager_.getDeadRooms();
	if (deadRooms.empty()) return;

	MS_WARN(logger_, "checkRoomHealth: found {} dead rooms", deadRooms.size());
	for (auto& roomId : deadRooms) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) continue;

		MS_WARN(logger_, "Room {} has dead router, notifying {} peers to reconnect",
			roomId, room->getPeerIds().size());

		if (broadcast_) {
			broadcast_(roomId, "", {
				{"notification", true}, {"method", "serverRestart"},
				{"data", {{"roomId", roomId}, {"reason", "worker crashed"}}}
			});
		}

		cleanupRoomResources(roomId);
		if (registry_) {
			auto* reg = registry_;
			std::string rid = roomId;
			postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
		}
		roomManager_.removeRoom(roomId);
		if (roomLifecycle_) roomLifecycle_(roomId, false);
	}
}

void RoomService::cleanIdleRooms(int idleSeconds) {
	for (auto& id : roomManager_.getIdleRooms(idleSeconds)) {
		MS_DEBUG(logger_, "GC idle room: {}", id);
		cleanupRoomResources(id);
		if (registry_) {
			auto* reg = registry_;
			std::string rid = id;
			postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
		}
		roomManager_.removeRoom(id);
		if (roomLifecycle_) roomLifecycle_(id, false);
	}
	cleanOldRecordings();
}

void RoomService::closeAllRooms() {
	auto roomIds = roomManager_.getRoomIds();
	for (auto& roomId : roomIds) {
		cleanupRoomResources(roomId);
		if (registry_) {
			auto* reg = registry_;
			std::string rid = roomId;
			postRegistryTask([reg, rid] { reg->unregisterRoom(rid); });
		}
		roomManager_.removeRoom(roomId);
		if (roomLifecycle_) roomLifecycle_(roomId, false);
	}
}

void RoomService::cleanupRoomResources(const std::string& roomId) {
	std::vector<std::shared_ptr<PeerRecorder>> recordersToStop;
	std::string prefix = roomId + "/";
	for (auto it = recorders_.begin(); it != recorders_.end(); ) {
		if (it->first.compare(0, prefix.size(), prefix) == 0) {
			if (it->second) recordersToStop.push_back(it->second);
			recorderTransports_.erase(it->first);
			it = recorders_.erase(it);
		} else ++it;
	}
	for (auto& rec : recordersToStop) {
		try {
			rec->stop();
		} catch (const std::exception& e) {
			MS_WARN(logger_, "cleanupRoomResources recorder stop failed [room:{}]: {}", roomId, e.what());
		} catch (...) {
			MS_WARN(logger_, "cleanupRoomResources recorder stop failed [room:{}]: unknown error", roomId);
		}
	}
	for (auto it = clientStats_.begin(); it != clientStats_.end(); )
		if (it->first.compare(0, prefix.size(), prefix) == 0) it = clientStats_.erase(it);
		else ++it;
}

// ── disk cleanup ──

void RoomService::cleanOldRecordings(uint64_t maxBytes) {
	if (recordDir_.empty()) return;

	struct DirEntry { std::string path; time_t mtime; uint64_t size; };
	std::vector<DirEntry> dirs;
	uint64_t totalSize = 0;

	DIR* dir = opendir(recordDir_.c_str());
	if (!dir) return;
	struct dirent* ent;
	while ((ent = readdir(dir))) {
		if (ent->d_name[0] == '.') continue;
		std::string subdir = recordDir_ + "/" + ent->d_name;
		struct stat st;
		if (stat(subdir.c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) continue;

		uint64_t dirSize = 0;
		time_t newest = 0;
		DIR* rdir = opendir(subdir.c_str());
		if (!rdir) continue;
		struct dirent* rent;
		while ((rent = readdir(rdir))) {
			std::string fpath = subdir + "/" + rent->d_name;
			struct stat fst;
			if (stat(fpath.c_str(), &fst) == 0 && S_ISREG(fst.st_mode)) {
				dirSize += fst.st_size;
				if (fst.st_mtime > newest) newest = fst.st_mtime;
			}
		}
		closedir(rdir);
		totalSize += dirSize;
		dirs.push_back({subdir, newest, dirSize});
	}
	closedir(dir);

	if (totalSize <= maxBytes) return;

	std::sort(dirs.begin(), dirs.end(), [](auto& a, auto& b) { return a.mtime < b.mtime; });

	for (auto& d : dirs) {
		if (totalSize <= maxBytes) break;
		bool active = false;
		std::string dirName = d.path.substr(d.path.rfind('/') + 1);
		std::string prefix = dirName + "/";
		for (auto& [key, _] : recorders_) {
			if (key.compare(0, prefix.size(), prefix) == 0) { active = true; break; }
		}
		if (active) continue;
		DIR* rdir = opendir(d.path.c_str());
		if (rdir) {
			struct dirent* rent;
			while ((rent = readdir(rdir)))
				if (rent->d_name[0] != '.') std::remove((d.path + "/" + rent->d_name).c_str());
			closedir(rdir);
		}
		rmdir(d.path.c_str());
		totalSize -= d.size;
		MS_DEBUG(logger_, "Cleaned old recording dir: {} (freed {} bytes)", d.path, d.size);
	}
}

// ── stats ──

void RoomService::setClientStats(const std::string& roomId, const std::string& peerId,
	const json& stats)
{
	clientStats_[roomId + "/" + peerId] = stats;
}

json RoomService::collectPeerStats(const std::string& roomId, const std::string& peerId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {};
	auto peer = room->getPeer(peerId);
	if (!peer) return {};

	json result = {{"peerId", peerId}};

	// Per-peer deadline: cap total stats collection time to avoid monopolizing
	// the control thread. Individual getStats calls use kStatsTimeoutMs, but
	// we also enforce an overall budget.
	static constexpr int kPerPeerBudgetMs = 2000;
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(kPerPeerBudgetMs);

	auto budgetLeft = [&]() -> int {
		auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
			deadline - std::chrono::steady_clock::now()).count();
		return remaining > 0 ? static_cast<int>(remaining) : 0;
	};

	auto statsTimeout = [&]() -> int {
		return std::min(kStatsTimeoutMs, budgetLeft());
	};

	if (peer->sendTransport && budgetLeft() > 0) {
		try { result["sendTransport"] = peer->sendTransport->getStats(statsTimeout()); }
		catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: {}", roomId, peerId, e.what());
			result["sendTransport"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: unknown error", roomId, peerId);
			result["sendTransport"] = nullptr;
		}
	}
	if (peer->recvTransport && budgetLeft() > 0) {
		try { result["recvTransport"] = peer->recvTransport->getStats(statsTimeout()); }
		catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] recvTransport getStats failed: {}", roomId, peerId, e.what());
			result["recvTransport"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] recvTransport getStats failed: unknown error", roomId, peerId);
			result["recvTransport"] = nullptr;
		}
	}

	json producers = json::object();
	for (auto& [pid, prod] : peer->producers) {
		json pstat;
		if (budgetLeft() > 0) {
			try { pstat["stats"] = prod->getStats(statsTimeout()); }
			catch (const std::exception& e) {
				MS_WARN(logger_, "[{} {}] producer {} getStats failed: {}", roomId, peerId, pid, e.what());
				pstat["stats"] = nullptr;
			} catch (...) {
				MS_WARN(logger_, "[{} {}] producer {} getStats failed: unknown error", roomId, peerId, pid);
				pstat["stats"] = nullptr;
			}
		} else {
			pstat["stats"] = nullptr;
		}
		json scores = json::array();
		for (auto& s : prod->scores())
			scores.push_back({{"ssrc", s.ssrc}, {"score", s.score}, {"rid", s.rid}});
		pstat["scores"] = scores;
		pstat["kind"] = prod->kind();
		producers[pid] = pstat;
	}
	result["producers"] = producers;

	json consumers = json::object();
	for (auto& [cid, cons] : peer->consumers) {
		json cstat;
		cstat["kind"] = cons->kind();
		cstat["producerId"] = cons->producerId();
		if (budgetLeft() > 0) {
			try {
				auto stats = cons->getStats(statsTimeout());
				if (!stats.empty() && stats[0].contains("score"))
					cstat["score"] = stats[0]["score"];
				else
					cstat["score"] = cons->currentScore().score;
			} catch (...) {
				cstat["score"] = cons->currentScore().score;
			}
		} else {
			cstat["score"] = cons->currentScore().score;
		}
		auto prod = room->router()->getProducerById(cons->producerId());
		if (prod && !prod->scores().empty()) {
			cstat["producerScore"] = prod->scores().back().score;
		} else {
			cstat["producerScore"] = cons->currentScore().producerScore;
		}
		consumers[cid] = cstat;
	}
	result["consumers"] = consumers;

	auto statsIt = clientStats_.find(roomId + "/" + peerId);
	if (statsIt != clientStats_.end()) result["clientStats"] = statsIt->second;

	return result;
}

void RoomService::heartbeatRegistry() {
	if (!registry_) return;
	registry_->heartbeat();
	// Refresh TTL for all active rooms
	auto roomIds = roomManager_.getRoomIds();
	if (!roomIds.empty())
		registry_->refreshRooms(roomIds);
	// Update node load info
	registry_->updateLoad(roomManager_.roomCount(), roomManager_.workerManager().maxTotalRouters());
}

json RoomService::resolveRoom(const std::string& roomId, const std::string& clientIp) {
	if (!registry_) return {{"wsUrl", ""}, {"isNew", true}};
	try {
		auto result = registry_->resolveRoom(roomId, clientIp);
		if (result.isNew && result.wsUrl.empty())
			return {{"error", "no available nodes"}, {"wsUrl", ""}, {"isNew", true}};
		return {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
	} catch (const std::exception& e) {
		MS_WARN(logger_, "resolveRoom failed ({}), degrading to local", e.what());
		return {{"wsUrl", ""}, {"isNew", true}};
	}
}

json RoomService::getNodeLoad() const {
	return {
		{"rooms", roomManager_.roomCount()},
		{"maxRooms", roomManager_.workerManager().maxTotalRouters()}
	};
}

	void RoomService::broadcastStats() {
		if (statsBroadcastActive_) return;

	auto roomIds = roomManager_.getRoomIds();
	if (roomIds.empty()) return;

	std::string names;
	for (auto& id : roomIds) { if (!names.empty()) names += ", "; names += id; }
	MS_DEBUG(logger_, "broadcastStats: {} rooms [{}]", roomIds.size(), names);

		statsBroadcastActive_ = true;
		pendingStatsRooms_.clear();
		for (auto& roomId : roomIds) pendingStatsRooms_.push_back(roomId);
		try {
			continueBroadcastStats();
		} catch (...) {
			statsBroadcastActive_ = false;
			pendingStatsRooms_.clear();
			throw;
		}
	}

void RoomService::continueBroadcastStats() {
	if (pendingStatsRooms_.empty()) {
		statsBroadcastActive_ = false;
		return;
	}

		std::string roomId = std::move(pendingStatsRooms_.front());
		pendingStatsRooms_.pop_front();
		try {
			broadcastStatsForRoom(roomId);
		} catch (...) {
			statsBroadcastActive_ = false;
			pendingStatsRooms_.clear();
			throw;
		}

	if (pendingStatsRooms_.empty()) {
		statsBroadcastActive_ = false;
		return;
	}

	if (taskPoster_) {
		taskPoster_([this] { continueBroadcastStats(); });
	} else {
		continueBroadcastStats();
	}
}

void RoomService::broadcastStatsForRoom(const std::string& roomId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return;

	json allStats = json::array();
	for (auto& peerId : room->getPeerIds()) {
		try {
			auto stats = collectPeerStats(roomId, peerId);
			if (!stats.empty()) allStats.push_back(stats);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "broadcastStats collectPeerStats failed [room:{} peer:{}]: {}", roomId, peerId, e.what());
			allStats.push_back({{"peerId", peerId}});
		} catch (...) {
			MS_WARN(logger_, "broadcastStats collectPeerStats failed [room:{} peer:{}]: unknown error", roomId, peerId);
			allStats.push_back({{"peerId", peerId}});
		}
	}

	if (allStats.empty()) return;

	if (broadcast_) {
		broadcast_(roomId, "", {
			{"notification", true}, {"method", "statsReport"},
			{"data", {{"roomId", roomId}, {"peers", allStats}}}
		});
	}

	for (auto& peerStats : allStats) {
		std::string key = roomId + "/" + peerStats.value("peerId", "");
		auto it = recorders_.find(key);
		if (it != recorders_.end() && it->second)
			it->second->appendQosSnapshot(peerStats);
	}
}

} // namespace mediasoup
