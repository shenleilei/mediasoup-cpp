#pragma once
#include "WorkerManager.h"
#include "Router.h"
#include "Peer.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

namespace mediasoup {

using json = nlohmann::json;

class Room {
public:
	Room(const std::string& id, std::shared_ptr<Router> router)
		: id_(id), router_(router) { touch(); }

	const std::string& id() const { return id_; }
	std::shared_ptr<Router> router() const { return router_; }

	void addPeer(std::shared_ptr<Peer> peer) {
		std::lock_guard<std::mutex> lock(mutex_);
		peers_[peer->id] = peer;
		touch();
	}

	std::shared_ptr<Peer> getPeer(const std::string& peerId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = peers_.find(peerId);
		return it != peers_.end() ? it->second : nullptr;
	}

	void removePeer(const std::string& peerId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = peers_.find(peerId);
		if (it != peers_.end()) {
			it->second->close();
			peers_.erase(it);
		}
		touch();
	}

	bool empty() const { std::lock_guard<std::mutex> lock(mutex_); return peers_.empty(); }
	size_t peerCount() const { std::lock_guard<std::mutex> lock(mutex_); return peers_.size(); }

	std::vector<std::shared_ptr<Peer>> getOtherPeers(const std::string& excludeId) {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<std::shared_ptr<Peer>> result;
		for (auto& [id, p] : peers_)
			if (id != excludeId) result.push_back(p);
		return result;
	}

	json getParticipants() const {
		std::lock_guard<std::mutex> lock(mutex_);
		json arr = json::array();
		for (auto& [_, p] : peers_) arr.push_back(p->toJson());
		return arr;
	}

	void touch() { lastActivity_ = std::chrono::steady_clock::now(); }
	bool isIdle(int seconds) const {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!peers_.empty()) return false;
		auto elapsed = std::chrono::steady_clock::now() - lastActivity_;
		return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= seconds;
	}

	void close() {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& [_, p] : peers_) p->close();
		peers_.clear();
		router_->close();
	}

private:
	std::string id_;
	std::shared_ptr<Router> router_;
	std::unordered_map<std::string, std::shared_ptr<Peer>> peers_;
	std::chrono::steady_clock::time_point lastActivity_;
	mutable std::mutex mutex_;
};

class RoomManager {
public:
	RoomManager(WorkerManager& workerManager, const std::vector<json>& mediaCodecs,
		const std::vector<json>& listenInfos)
		: workerManager_(workerManager), mediaCodecs_(mediaCodecs), listenInfos_(listenInfos) {}

	std::shared_ptr<Room> createRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = rooms_.find(roomId);
		if (it != rooms_.end()) return it->second;
		auto worker = workerManager_.getLeastLoadedWorker();
		if (!worker) throw std::runtime_error("no available worker");
		auto router = worker->createRouter(mediaCodecs_);
		auto room = std::make_shared<Room>(roomId, router);
		rooms_[roomId] = room;
		return room;
	}

	std::shared_ptr<Room> getRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = rooms_.find(roomId);
		return it != rooms_.end() ? it->second : nullptr;
	}

	void removeRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = rooms_.find(roomId);
		if (it != rooms_.end()) { it->second->close(); rooms_.erase(it); }
	}

	std::vector<std::string> getIdleRooms(int seconds) {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<std::string> result;
		for (auto& [id, room] : rooms_)
			if (room->isIdle(seconds)) result.push_back(id);
		return result;
	}

	size_t roomCount() const { std::lock_guard<std::mutex> lock(mutex_); return rooms_.size(); }
	const std::vector<json>& listenInfos() const { return listenInfos_; }

private:
	WorkerManager& workerManager_;
	std::vector<json> mediaCodecs_;
	std::vector<json> listenInfos_;
	std::unordered_map<std::string, std::shared_ptr<Room>> rooms_;
	mutable std::mutex mutex_;
};

} // namespace mediasoup
