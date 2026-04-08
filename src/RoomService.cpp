#include "RoomService.h"
#include <future>
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
	const std::string& displayName, const json& rtpCapabilities)
{
	if (registry_) {
		std::string addr = registry_->claimRoom(roomId);
		if (!addr.empty()) return {false, {}, addr, ""};
	}

	if (!rtpCapabilities.is_null() && !rtpCapabilities.empty() && !rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] join validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
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

RoomService::Result RoomService::leave(const std::string& roomId, const std::string& peerId) {
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {true, {}};

	// Stop recorder outside lock to avoid blocking broadcastStats
	std::shared_ptr<PeerRecorder> recToStop;
	{
		std::string key = roomId + "/" + peerId;
		std::lock_guard<std::mutex> lock(recorderMutex_);
		auto it = recorders_.find(key);
		if (it != recorders_.end()) {
			recToStop = std::move(it->second);
			recorders_.erase(it);
		}
		recorderTransports_.erase(key);
	}
	if (recToStop) recToStop->stop();

	{
		std::lock_guard<std::mutex> lock(clientStatsMutex_);
		clientStats_.erase(roomId + "/" + peerId);
	}

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
		if (registry_) registry_->unregisterRoom(roomId);
		roomManager_.removeRoom(roomId);
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
	std::lock_guard<std::mutex> lock(recorderMutex_);

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
		if (registry_) registry_->unregisterRoom(roomId);
		roomManager_.removeRoom(roomId);
	}
}

void RoomService::cleanIdleRooms(int idleSeconds) {
	for (auto& id : roomManager_.getIdleRooms(idleSeconds)) {
		MS_DEBUG(logger_, "GC idle room: {}", id);
		cleanupRoomResources(id);
		if (registry_) registry_->unregisterRoom(id);
		roomManager_.removeRoom(id);
	}
	cleanOldRecordings();
}

void RoomService::cleanupRoomResources(const std::string& roomId) {
	std::vector<std::shared_ptr<PeerRecorder>> recordersToStop;
	{
		std::lock_guard<std::mutex> lock(recorderMutex_);
		std::string prefix = roomId + "/";
		for (auto it = recorders_.begin(); it != recorders_.end(); ) {
			if (it->first.compare(0, prefix.size(), prefix) == 0) {
				if (it->second) recordersToStop.push_back(it->second);
				recorderTransports_.erase(it->first);
				it = recorders_.erase(it);
			} else ++it;
		}
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
	{
		std::lock_guard<std::mutex> lock(clientStatsMutex_);
		std::string prefix = roomId + "/";
		for (auto it = clientStats_.begin(); it != clientStats_.end(); )
			if (it->first.compare(0, prefix.size(), prefix) == 0) it = clientStats_.erase(it);
			else ++it;
	}
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
		{
			std::lock_guard<std::mutex> lock(recorderMutex_);
			bool active = false;
			std::string dirName = d.path.substr(d.path.rfind('/') + 1);
			std::string prefix = dirName + "/";
			for (auto& [key, _] : recorders_) {
				if (key.compare(0, prefix.size(), prefix) == 0) { active = true; break; }
			}
			if (active) continue;
		}
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
	std::lock_guard<std::mutex> lock(clientStatsMutex_);
	clientStats_[roomId + "/" + peerId] = stats;
}

json RoomService::collectPeerStats(const std::string& roomId, const std::string& peerId) {
	static constexpr auto kTimeout = std::chrono::milliseconds(kStatsTimeoutMs);

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {};
	auto peer = room->getPeer(peerId);
	if (!peer) return {};

	json result = {{"peerId", peerId}};

	auto timedGetStats = [&](auto& transport) -> json {
		auto fut = std::async(std::launch::async, [&]{ return transport->getStats(); });
		if (fut.wait_for(kTimeout) == std::future_status::ready) return fut.get();
		MS_WARN(logger_, "[{} {}] getStats timeout", roomId, peerId);
		return nullptr;
	};

	if (peer->sendTransport) {
		try { result["sendTransport"] = timedGetStats(peer->sendTransport); }
		catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: {}", roomId, peerId, e.what());
			result["sendTransport"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] sendTransport getStats failed: unknown error", roomId, peerId);
			result["sendTransport"] = nullptr;
		}
	}
	if (peer->recvTransport) {
		try { result["recvTransport"] = timedGetStats(peer->recvTransport); }
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
		try {
			auto fut = std::async(std::launch::async, [&]{ return prod->getStats(); });
			if (fut.wait_for(kTimeout) == std::future_status::ready)
				pstat["stats"] = fut.get();
			else { MS_WARN(logger_, "[{} {}] producer {} getStats timeout", roomId, peerId, pid); pstat["stats"] = nullptr; }
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] producer {} getStats failed: {}", roomId, peerId, pid, e.what());
			pstat["stats"] = nullptr;
		} catch (...) {
			MS_WARN(logger_, "[{} {}] producer {} getStats failed: unknown error", roomId, peerId, pid);
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
		// Get consumer score from getStats (reliable) with notification as fallback
		try {
			auto fut = std::async(std::launch::async, [c = cons]{ return c->getStats(); });
			if (fut.wait_for(kTimeout) == std::future_status::ready) {
				auto stats = fut.get();
				if (!stats.empty() && stats[0].contains("score"))
					cstat["score"] = stats[0]["score"];
				else
					cstat["score"] = cons->currentScore().score;
			} else {
				cstat["score"] = cons->currentScore().score;
			}
		} catch (...) {
			cstat["score"] = cons->currentScore().score;
		}
		// Get producerScore from the actual Producer object
		auto prod = room->router()->getProducerById(cons->producerId());
		if (prod && !prod->scores().empty()) {
			cstat["producerScore"] = prod->scores().back().score;
		} else {
			cstat["producerScore"] = cons->currentScore().producerScore;
		}
		consumers[cid] = cstat;
	}
	result["consumers"] = consumers;

	{
		std::lock_guard<std::mutex> lock(clientStatsMutex_);
		auto it = clientStats_.find(roomId + "/" + peerId);
		if (it != clientStats_.end()) result["clientStats"] = it->second;
	}

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

json RoomService::resolveRoom(const std::string& roomId) {
	if (!registry_) return {{"wsUrl", ""}, {"isNew", true}};
	auto result = registry_->resolveRoom(roomId);
	return {{"wsUrl", result.wsUrl}, {"isNew", result.isNew}};
}

json RoomService::getNodeLoad() const {
	return {
		{"rooms", roomManager_.roomCount()},
		{"maxRooms", roomManager_.workerManager().maxTotalRouters()}
	};
}

void RoomService::broadcastStats() {
	auto roomIds = roomManager_.getRoomIds();
	if (roomIds.empty()) return;

	std::string names;
	for (auto& id : roomIds) { if (!names.empty()) names += ", "; names += id; }
	MS_DEBUG(logger_, "broadcastStats: {} rooms [{}]", roomIds.size(), names);

	for (auto& roomId : roomIds) {
		auto room = roomManager_.getRoom(roomId);
		if (!room) continue;

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

		if (allStats.empty()) continue;

		if (broadcast_) {
			broadcast_(roomId, "", {
				{"notification", true}, {"method", "statsReport"},
				{"data", {{"roomId", roomId}, {"peers", allStats}}}
			});
		}

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

} // namespace mediasoup
