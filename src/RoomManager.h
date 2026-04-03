#pragma once
#include "WorkerManager.h"
#include "Router.h"
#include "Transport.h"
#include "WebRtcTransport.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>

namespace mediasoup {

using json = nlohmann::json;

class Room {
public:
	Room(const std::string& id, std::shared_ptr<Router> router)
		: id_(id), router_(router) {}

	const std::string& id() const { return id_; }
	std::shared_ptr<Router> router() const { return router_; }

	void addTransport(const std::string& peerId, std::shared_ptr<Transport> transport) {
		std::lock_guard<std::mutex> lock(mutex_);
		peerTransports_[peerId].push_back(transport);
	}

	std::shared_ptr<Transport> getTransport(const std::string& transportId) {
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& [peerId, transports] : peerTransports_) {
			for (auto& t : transports) {
				if (t->id() == transportId) return t;
			}
		}
		return nullptr;
	}

	void removePeer(const std::string& peerId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = peerTransports_.find(peerId);
		if (it != peerTransports_.end()) {
			for (auto& t : it->second) {
				t->close();
			}
			it->second.clear();
			peerTransports_.erase(it);
		}
	}

	std::vector<std::string> getPeerIds() const {
		std::lock_guard<std::mutex> lock(mutex_);
		std::vector<std::string> ids;
		for (auto& [id, _] : peerTransports_) ids.push_back(id);
		return ids;
	}

private:
	std::string id_;
	std::shared_ptr<Router> router_;
	std::unordered_map<std::string, std::vector<std::shared_ptr<Transport>>> peerTransports_;
	mutable std::mutex mutex_;
};

class RoomManager {
public:
	RoomManager(WorkerManager& workerManager, const std::vector<json>& mediaCodecs,
		const std::vector<json>& listenInfos)
		: workerManager_(workerManager), mediaCodecs_(mediaCodecs), listenInfos_(listenInfos) {}

	std::shared_ptr<Room> getOrCreateRoom(const std::string& roomId) {
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
		return (it != rooms_.end()) ? it->second : nullptr;
	}

	void removeRoom(const std::string& roomId) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = rooms_.find(roomId);
		if (it != rooms_.end()) {
			it->second->router()->close();
			rooms_.erase(it);
		}
	}

	const std::vector<json>& listenInfos() const { return listenInfos_; }

private:
	WorkerManager& workerManager_;
	std::vector<json> mediaCodecs_;
	std::vector<json> listenInfos_;
	std::unordered_map<std::string, std::shared_ptr<Room>> rooms_;
	std::mutex mutex_;
};

} // namespace mediasoup
