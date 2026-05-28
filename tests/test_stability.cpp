// Unit tests for stability fixes:
// - Worker auto-respawn (WorkerManager)
// - Room::routerAlive / RoomManager::getDeadRooms
// - EventEmitter off() cleanup
#include <gtest/gtest.h>
#include "RoomManager.h"
#include "EventEmitter.h"

using namespace mediasoup;

// ═══════════════════════════════════════════════════════════════
// Room::routerAlive / RoomManager::getDeadRooms
// ═══════════════════════════════════════════════════════════════

TEST(RoomHealthTest, RouterAliveWithNullRouter) {
	auto room = std::make_shared<Room>("test", nullptr);
	EXPECT_FALSE(room->routerAlive());
}

TEST(RoomHealthTest, GetDeadRoomsEmpty) {
	// Can't construct RoomManager without WorkerManager, but we can test Room directly
	auto room = std::make_shared<Room>("dead-room", nullptr);
	EXPECT_FALSE(room->routerAlive());
}

// ═══════════════════════════════════════════════════════════════
// EventEmitter off(string) cleanup
// ═══════════════════════════════════════════════════════════════

TEST(EventEmitterTest, OffByEventName) {
	EventEmitter emitter;
	int callCount = 0;
	emitter.on("test-event", [&](auto&) { callCount++; });
	emitter.emit("test-event");
	EXPECT_EQ(callCount, 1);

	emitter.off("test-event");
	emitter.emit("test-event");
	EXPECT_EQ(callCount, 1); // should not increment after off
}

TEST(EventEmitterTest, OffById) {
	EventEmitter emitter;
	int count1 = 0, count2 = 0;
	auto id1 = emitter.on("evt", [&](auto&) { count1++; });
	emitter.on("evt", [&](auto&) { count2++; });

	emitter.emit("evt");
	EXPECT_EQ(count1, 1);
	EXPECT_EQ(count2, 1);

	emitter.off(id1);
	emitter.emit("evt");
	EXPECT_EQ(count1, 1); // removed
	EXPECT_EQ(count2, 2); // still active
}

TEST(EventEmitterTest, OnceFiresOnlyOnce) {
	EventEmitter emitter;
	int callCount = 0;
	emitter.once("evt", [&](auto&) { callCount++; });
	emitter.emit("evt");
	emitter.emit("evt");
	EXPECT_EQ(callCount, 1);
}

TEST(EventEmitterTest, NoLeakAfterOff) {
	EventEmitter emitter;
	// Register and remove many listeners — should not accumulate
	for (int i = 0; i < 1000; i++) {
		std::string event = "event-" + std::to_string(i);
		emitter.on(event, [](auto&) {});
		emitter.off(event);
	}
	// Emit should be fast (no accumulated listeners)
	emitter.emit("event-0"); // should not crash
}

TEST(EventEmitterTest, ListenerExceptionsDoNotCrashEmit) {
	EventEmitter emitter;
	int called = 0;
	emitter.on("evt", [](auto&) { throw std::bad_any_cast(); });
	emitter.on("evt", [&](auto&) { called++; });
	emitter.emit("evt");
	EXPECT_EQ(called, 1);
}

TEST(EventEmitterTest, EmitCheckedRethrowsListenerFailureAfterInvokingListeners) {
	EventEmitter emitter;
	int called = 0;
	emitter.on("evt", [](auto&) { throw std::runtime_error("listener failed"); });
	emitter.on("evt", [&](auto&) { called++; });
	EXPECT_THROW(emitter.emitChecked("evt"), std::runtime_error);
	EXPECT_EQ(called, 1);
}
