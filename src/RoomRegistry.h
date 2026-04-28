#pragma once

#include "Constants.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct redisContext;
struct redisReply;

namespace spdlog {
class logger;
}

namespace mediasoup {

class GeoRouter;

class RoomRegistry {
public:
	RoomRegistry(const std::string& redisHost, int redisPort,
		const std::string& nodeId, const std::string& nodeAddress,
		double nodeLat = 0, double nodeLng = 0, const std::string& nodeIsp = "",
		const std::string& nodeCountry = "", bool countryIsolation = false);
	~RoomRegistry();

	void start();
	size_t knownNodeCount() const;
	void heartbeat();
	void updateLoad(size_t rooms, size_t maxRooms);
	void refreshRooms(const std::vector<std::string>& roomIds);

	struct ResolveResult {
		std::string wsUrl;
		bool isNew = false;
	};

	ResolveResult resolveRoom(const std::string& roomId, const std::string& clientIp = "");
	std::string claimRoom(const std::string& roomId, const std::string& clientIp = "");
	void refreshRoom(const std::string& roomId);
	void unregisterRoom(const std::string& roomId);
	bool isReady() const;

	const std::string& nodeId() const { return nodeId_; }
	const std::string& nodeAddress() const { return nodeAddress_; }
	void setGeoRouter(GeoRouter* geo) { geo_ = geo; }
	void stop();

private:
	bool reconnect();
	bool ensureConnected();
	void handleDisconnect();

	std::string buildNodeValue(size_t rooms, size_t maxRooms);

	struct NodeInfo {
		std::string address;
		size_t rooms = 0;
		size_t maxRooms = 0;
		double lat = 0;
		double lng = 0;
		std::string isp;
		std::string country;
	};

	NodeInfo parseNodeValue(const std::string& val);
	void registerNode();

	static constexpr const char* kChannelNodes = "sfu:ch:nodes";
	static constexpr const char* kChannelRooms = "sfu:ch:rooms";
	static constexpr const char* kKeyPrefixRoom = "sfu:room:";
	static constexpr const char* kKeyPrefixNode = "sfu:node:";

	void publishNodeUpdate(const std::string& nodeId, const std::string& nodeValue);
	void publishRoomUpdate(const std::string& roomId, const std::string& ownerAddr);
	void startSubscriber();
	void subscriberLoop();
	void handlePubSubMessage(const std::string& channel, const std::string& data);
	void syncAll();
	void evictDeadNodes();
	redisReply* mgetArgv(const std::vector<std::string>& keys);
	std::vector<std::string> scanKeys(const char* pattern);
	void syncNodesUnlocked();
	void syncAllUnlocked();
	bool refreshCachedRemoteRoomAddressUnlocked(
		const std::string& roomId,
		const std::string& cachedAddress,
		std::string* refreshedAddress);
	std::string findBestNodeCached(const std::string& clientIp);
	bool hasRemoteNodeCached() const;

	struct CommandConnection {
		CommandConnection() = default;
		~CommandConnection();

		CommandConnection(const CommandConnection&) = delete;
		CommandConnection& operator=(const CommandConnection&) = delete;

		bool reconnect(
			const std::string& redisHost,
			int redisPort,
			const std::shared_ptr<spdlog::logger>& logger);
		bool ensureConnected(
			const std::string& redisHost,
			int redisPort,
			const std::shared_ptr<spdlog::logger>& logger);
		void disconnect(
			const std::shared_ptr<spdlog::logger>& logger,
			bool logAsWarning = true);
		bool connected() const;
		redisReply* command(const char* format, ...);
		int appendCommand(const char* format, ...);
		int getReply(void** reply);
		redisReply* commandArgv(
			int argc,
			const char** argv,
			const size_t* argvLen);

		redisContext* ctx = nullptr;
		std::mutex mutex;
	};

	struct CacheView {
		std::unordered_map<std::string, NodeInfo> nodes;
		std::unordered_map<std::string, std::string> rooms;
		mutable std::mutex mutex;

		size_t knownNodeCount() const {
			std::lock_guard<std::mutex> lock(mutex);
			return nodes.size();
		}

		std::optional<std::string> roomAddress(const std::string& roomId) const {
			std::lock_guard<std::mutex> lock(mutex);
			auto it = rooms.find(roomId);
			if (it == rooms.end() || it->second.empty()) return std::nullopt;
			return it->second;
		}

		std::optional<std::string> nodeAddressForId(const std::string& nodeId) const {
			std::lock_guard<std::mutex> lock(mutex);
			auto it = nodes.find(nodeId);
			if (it == nodes.end() || it->second.address.empty()) return std::nullopt;
			return it->second.address;
		}

		void upsertNode(const std::string& nodeId, const NodeInfo& info) {
			std::lock_guard<std::mutex> lock(mutex);
			nodes[nodeId] = info;
		}

		void upsertRoom(const std::string& roomId, const std::string& address) {
			std::lock_guard<std::mutex> lock(mutex);
			rooms[roomId] = address;
		}

		void eraseRoom(const std::string& roomId) {
			std::lock_guard<std::mutex> lock(mutex);
			rooms.erase(roomId);
		}

		void eraseNodeAndOwnedRooms(const std::string& nodeId) {
			std::lock_guard<std::mutex> lock(mutex);
			auto it = nodes.find(nodeId);
			if (it == nodes.end()) return;
			const std::string deadAddr = it->second.address;
			nodes.erase(it);
			if (deadAddr.empty()) return;
			for (auto rit = rooms.begin(); rit != rooms.end(); ) {
				if (rit->second == deadAddr) rit = rooms.erase(rit);
				else ++rit;
			}
		}

		std::vector<std::string> otherNodeIds(const std::string& selfNodeId) const {
			std::lock_guard<std::mutex> lock(mutex);
			std::vector<std::string> result;
			for (const auto& [nodeId, _] : nodes)
				if (nodeId != selfNodeId) result.push_back(nodeId);
			return result;
		}

		void mergeNodes(const std::unordered_map<std::string, NodeInfo>& newNodes) {
			if (newNodes.empty()) return;
			std::lock_guard<std::mutex> lock(mutex);
			for (const auto& [nodeId, info] : newNodes)
				nodes[nodeId] = info;
		}

		void replaceAll(
			std::unordered_map<std::string, NodeInfo> newNodes,
			std::unordered_map<std::string, std::string> newRooms)
		{
			std::lock_guard<std::mutex> lock(mutex);
			nodes = std::move(newNodes);
			rooms = std::move(newRooms);
		}
	};

	std::string nodeId_;
	std::string nodeAddress_;
	double nodeLat_;
	double nodeLng_;
	std::string nodeIsp_;
	std::string nodeCountry_;
	bool countryIsolation_ = false;
	std::string redisHost_;
	int redisPort_;

	CommandConnection command_;
	CacheView cache_;
	std::atomic<bool> commandReady_{false};

	std::thread subThread_;
	std::atomic<bool> subStop_{false};

	int roomTTL_ = kRedisRoomTtlSec;
	int nodeTTL_ = kRedisNodeTtlSec;
	GeoRouter* geo_ = nullptr;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
