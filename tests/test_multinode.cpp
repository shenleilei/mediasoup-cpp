#include <gtest/gtest.h>
#include "WorkerManager.h"
#include "RoomManager.h"

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
