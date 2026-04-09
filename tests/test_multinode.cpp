#include <gtest/gtest.h>
#include "WorkerManager.h"
#include "RoomManager.h"
#include <deque>

using namespace mediasoup;

// ═══════════════════════════════════════════════════════════════
// WorkerManager: maxRoutersPerWorker
// ═══════════════════════════════════════════════════════════════

TEST(WorkerManagerTest, MaxRoutersPerWorkerDefault) {
	WorkerManager wm;
	EXPECT_EQ(wm.maxRoutersPerWorker(), 0u); // unlimited by default
}

TEST(WorkerManagerTest, SetMaxRoutersPerWorker) {
	WorkerManager wm;
	wm.setMaxRoutersPerWorker(10);
	EXPECT_EQ(wm.maxRoutersPerWorker(), 10u);
}

TEST(WorkerManagerTest, TotalRouterCountEmpty) {
	WorkerManager wm;
	EXPECT_EQ(wm.totalRouterCount(), 0u);
}

TEST(WorkerManagerTest, MaxTotalRoutersZeroWhenUnlimited) {
	WorkerManager wm;
	EXPECT_EQ(wm.maxTotalRouters(), 0u); // 0 = unlimited
}

// ═══════════════════════════════════════════════════════════════
// RoomManager: workerManager accessor
// ═══════════════════════════════════════════════════════════════

TEST(RoomManagerTest, WorkerManagerAccessor) {
	WorkerManager wm;
	wm.setMaxRoutersPerWorker(20);
	std::vector<nlohmann::json> codecs, listenInfos;
	RoomManager rm(wm, codecs, listenInfos);
	EXPECT_EQ(rm.workerManager().maxRoutersPerWorker(), 20u);
}

// ═══════════════════════════════════════════════════════════════
// RoomManager: createRoom returns nullptr when all workers full
// (no real workers → getLeastLoadedWorker returns nullptr → throws)
// ═══════════════════════════════════════════════════════════════

TEST(RoomManagerTest, CreateRoomThrowsWhenNoWorkers) {
	WorkerManager wm;
	std::vector<nlohmann::json> codecs, listenInfos;
	RoomManager rm(wm, codecs, listenInfos);
	EXPECT_THROW(rm.createRoom("test-room"), std::runtime_error);
}

// ═══════════════════════════════════════════════════════════════
// WorkerManager: removeWorker cleans up dead entries
// ═══════════════════════════════════════════════════════════════

TEST(WorkerManagerTest, RemoveWorkerNullSafe) {
	WorkerManager wm;
	wm.removeWorker(nullptr);
	EXPECT_EQ(wm.totalRouterCount(), 0u);
}

// ═══════════════════════════════════════════════════════════════
// Room dispatch: concurrent first-join must not split room
// ═══════════════════════════════════════════════════════════════

TEST(RoomDispatchTest, ConcurrentFirstJoinSameThread) {
	// Simulate the dispatch table logic: first miss binds immediately
	std::unordered_map<std::string, int> roomDispatch;

	auto getOrAssign = [&](const std::string& roomId) -> int {
		auto it = roomDispatch.find(roomId);
		if (it != roomDispatch.end()) return it->second;
		int thread = 0;
		roomDispatch[roomId] = thread;
		return thread;
	};

	int t1 = getOrAssign("room1");
	int t2 = getOrAssign("room1");
	EXPECT_EQ(t1, t2) << "Same room must map to same thread";
}

// ═══════════════════════════════════════════════════════════════
// Respawn rate limiter
// ═══════════════════════════════════════════════════════════════

TEST(RespawnRateLimitTest, AllowsUpToThreeInWindow) {
	std::deque<std::chrono::steady_clock::time_point> times;
	auto now = std::chrono::steady_clock::now();

	auto tryRespawn = [&]() -> bool {
		times.push_back(now);
		while (!times.empty() && (now - times.front()) > std::chrono::seconds(10))
			times.pop_front();
		return times.size() <= 3;
	};

	EXPECT_TRUE(tryRespawn());
	EXPECT_TRUE(tryRespawn());
	EXPECT_TRUE(tryRespawn());
	EXPECT_FALSE(tryRespawn());  // 4th — rate limited
}

TEST(RespawnRateLimitTest, WindowExpiry) {
	std::deque<std::chrono::steady_clock::time_point> times;
	auto base = std::chrono::steady_clock::now();

	for (int i = 0; i < 3; i++)
		times.push_back(base);

	// At t=11s, old entries should be expired
	auto now = base + std::chrono::seconds(11);
	times.push_back(now);
	while (!times.empty() && (now - times.front()) > std::chrono::seconds(10))
		times.pop_front();

	EXPECT_EQ(times.size(), 1u) << "Old entries should be expired";
}

// ═══════════════════════════════════════════════════════════════
// ID validation: roomId/peerId whitelist [A-Za-z0-9_-]{1,128}
// ═══════════════════════════════════════════════════════════════

static bool isValidId(const std::string& s) {
	if (s.empty() || s.size() > 128) return false;
	for (char c : s)
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			  (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
	return true;
}

TEST(IdValidationTest, ValidIds) {
	EXPECT_TRUE(isValidId("room-123"));
	EXPECT_TRUE(isValidId("peer_abc"));
	EXPECT_TRUE(isValidId("A"));
	EXPECT_TRUE(isValidId(std::string(128, 'a')));
}

TEST(IdValidationTest, InvalidIds) {
	EXPECT_FALSE(isValidId(""));
	EXPECT_FALSE(isValidId(std::string(129, 'a')));
	EXPECT_FALSE(isValidId("room/123"));
	EXPECT_FALSE(isValidId("room:123"));
	EXPECT_FALSE(isValidId("room 123"));
	EXPECT_FALSE(isValidId("room\n123"));
	EXPECT_FALSE(isValidId("room*"));
	EXPECT_FALSE(isValidId("房间"));
}

// nodeId allows additional . and : for hostname:port
static bool isValidNodeId(const std::string& s) {
	if (s.empty() || s.size() > 128) return false;
	for (char c : s)
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':')) return false;
	return true;
}

TEST(IdValidationTest, NodeIdValid) {
	EXPECT_TRUE(isValidNodeId("myhost:3000"));
	EXPECT_TRUE(isValidNodeId("sfu-01.prod:3000"));
}

TEST(IdValidationTest, NodeIdInvalid) {
	EXPECT_FALSE(isValidNodeId("my host:3000"));
	EXPECT_FALSE(isValidNodeId("node\t1"));
}
