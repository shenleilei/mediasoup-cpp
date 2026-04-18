#pragma once

#include "Constants.h"

#include <atomic>
#include <memory>
#include <mutex>
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
	std::string findBestNodeCached(const std::string& clientIp);
	bool hasRemoteNodeCached() const;

	std::string nodeId_;
	std::string nodeAddress_;
	double nodeLat_;
	double nodeLng_;
	std::string nodeIsp_;
	std::string nodeCountry_;
	bool countryIsolation_ = false;
	std::string redisHost_;
	int redisPort_;

	redisContext* ctx_ = nullptr;
	std::mutex cmdMutex_;

	std::unordered_map<std::string, NodeInfo> nodeCache_;
	std::unordered_map<std::string, std::string> roomCache_;
	mutable std::mutex cacheMutex_;

	std::thread subThread_;
	std::atomic<bool> subStop_{false};

	int roomTTL_ = kRedisRoomTtlSec;
	int nodeTTL_ = kRedisNodeTtlSec;
	GeoRouter* geo_ = nullptr;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
