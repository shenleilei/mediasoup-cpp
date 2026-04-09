#pragma once
#include "RoomManager.h"
#include "RoomRegistry.h"
#include "WebRtcTransport.h"
#include "PipeTransport.h"
#include "PlainTransport.h"
#include "Consumer.h"
#include "Producer.h"
#include "Recorder.h"
#include "Constants.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <functional>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace mediasoup {

using json = nlohmann::json;

class RoomService {
public:
	// NotifyFn(roomId, peerId, msg)
	using NotifyFn = std::function<void(const std::string&, const std::string&, const json&)>;
	using BroadcastFn = std::function<void(const std::string&, const std::string&, const json&)>;
	// RoomLifecycleFn(roomId, created): created=true when room is created, false when destroyed.
	using RoomLifecycleFn = std::function<void(const std::string&, bool)>;
	using RegistryTaskFn = std::function<void(std::function<void()>)>;

	RoomService(RoomManager& roomManager, RoomRegistry* registry,
		const std::string& recordDir = "");

	void setNotify(NotifyFn fn) { notify_ = std::move(fn); }
	void setBroadcast(BroadcastFn fn) { broadcast_ = std::move(fn); }
	void setRoomLifecycle(RoomLifecycleFn fn) { roomLifecycle_ = std::move(fn); }
	void setRegistryTask(RegistryTaskFn fn) { registryTask_ = std::move(fn); }

	void postRegistryTask(std::function<void()> task) {
		if (registryTask_) registryTask_(std::move(task));
		else { try { task(); } catch (...) {} }
	}

	struct Result {
		bool ok = true;
		json data;
		std::string redirect;
		std::string error;
	};

	Result join(const std::string& roomId, const std::string& peerId,
		const std::string& displayName, const json& rtpCapabilities,
		const std::string& clientIp = "");
	Result leave(const std::string& roomId, const std::string& peerId);
	Result createTransport(const std::string& roomId, const std::string& peerId,
		bool producing, bool consuming);
	Result connectTransport(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const DtlsParameters& dtlsParams);
	Result produce(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const std::string& kind,
		const json& rtpParameters);
	Result consume(const std::string& roomId, const std::string& peerId,
		const std::string& transportId, const std::string& producerId,
		const json& rtpCapabilities);
	Result pauseProducer(const std::string& roomId, const std::string& producerId);
	Result resumeProducer(const std::string& roomId, const std::string& producerId);
	Result restartIce(const std::string& roomId, const std::string& peerId,
		const std::string& transportId);

	void checkRoomHealth();
	void cleanIdleRooms(int idleSeconds = kIdleRoomTimeoutSec);
	void closeAllRooms();
	bool hasRoom(const std::string& roomId) { return roomManager_.getRoom(roomId) != nullptr; }
	std::shared_ptr<Room> getRoom(const std::string& roomId) { return roomManager_.getRoom(roomId); }
	void cleanOldRecordings(uint64_t maxBytes = kMaxRecordingDirBytes);
	void setClientStats(const std::string& roomId, const std::string& peerId, const json& stats);
	json collectPeerStats(const std::string& roomId, const std::string& peerId);
	void broadcastStats();
	void heartbeatRegistry();

	// Multi-node: resolve room location
	json resolveRoom(const std::string& roomId, const std::string& clientIp = "");
	// Multi-node: get node load info
	json getNodeLoad() const;

private:
	void autoRecord(const std::string& roomId, const std::string& peerId,
		std::shared_ptr<Room> room, std::shared_ptr<Producer> producer);
	void cleanupRoomResources(const std::string& roomId);

	RoomManager& roomManager_;
	RoomRegistry* registry_;
	std::string recordDir_;
	NotifyFn notify_;
	BroadcastFn broadcast_;
	RoomLifecycleFn roomLifecycle_;
	RegistryTaskFn registryTask_;
	std::shared_ptr<spdlog::logger> logger_;

	std::mutex recorderMutex_;
	std::unordered_map<std::string, std::shared_ptr<PeerRecorder>> recorders_;
	std::unordered_map<std::string, std::shared_ptr<PlainTransport>> recorderTransports_;

	std::mutex clientStatsMutex_;
	std::unordered_map<std::string, json> clientStats_;
};

} // namespace mediasoup
