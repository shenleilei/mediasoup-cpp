#include <gtest/gtest.h>
#include "EventEmitter.h"
#include <thread>
#include <atomic>
#include <vector>

using namespace mediasoup;

// --- on() ---

TEST(EventEmitterTest, OnRegistersListenerAndEmitCallsIt) {
	EventEmitter emitter;
	int called = 0;
	emitter.on("test", [&](const std::vector<std::any>&) { ++called; });
	emitter.emit("test");
	EXPECT_EQ(called, 1);
}

TEST(EventEmitterTest, OnCalledMultipleTimesOnSameEvent) {
	EventEmitter emitter;
	int count = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { count += 1; });
	emitter.on("evt", [&](const std::vector<std::any>&) { count += 10; });
	emitter.emit("evt");
	EXPECT_EQ(count, 11);
}

TEST(EventEmitterTest, OnReturnsUniqueIds) {
	EventEmitter emitter;
	auto id1 = emitter.on("a", [](const std::vector<std::any>&) {});
	auto id2 = emitter.on("a", [](const std::vector<std::any>&) {});
	auto id3 = emitter.on("b", [](const std::vector<std::any>&) {});
	EXPECT_NE(id1, id2);
	EXPECT_NE(id2, id3);
}

TEST(EventEmitterTest, OnListenerCalledRepeatedly) {
	EventEmitter emitter;
	int count = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { ++count; });
	emitter.emit("evt");
	emitter.emit("evt");
	emitter.emit("evt");
	EXPECT_EQ(count, 3);
}

// --- once() ---

TEST(EventEmitterTest, OnceListenerCalledOnlyOnce) {
	EventEmitter emitter;
	int count = 0;
	emitter.once("evt", [&](const std::vector<std::any>&) { ++count; });
	emitter.emit("evt");
	emitter.emit("evt");
	EXPECT_EQ(count, 1);
}

TEST(EventEmitterTest, OnceReturnsUniqueId) {
	EventEmitter emitter;
	auto id1 = emitter.once("a", [](const std::vector<std::any>&) {});
	auto id2 = emitter.on("a", [](const std::vector<std::any>&) {});
	EXPECT_NE(id1, id2);
}

TEST(EventEmitterTest, OnceMixedWithOnPreservesOnListeners) {
	EventEmitter emitter;
	int persistentCount = 0;
	int onceCount = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { ++persistentCount; });
	emitter.once("evt", [&](const std::vector<std::any>&) { ++onceCount; });
	emitter.emit("evt");
	emitter.emit("evt");
	EXPECT_EQ(persistentCount, 2);
	EXPECT_EQ(onceCount, 1);
}

// --- off(id) ---

TEST(EventEmitterTest, OffByIdRemovesSpecificListener) {
	EventEmitter emitter;
	int countA = 0, countB = 0;
	auto idA = emitter.on("evt", [&](const std::vector<std::any>&) { ++countA; });
	emitter.on("evt", [&](const std::vector<std::any>&) { ++countB; });
	emitter.off(idA);
	emitter.emit("evt");
	EXPECT_EQ(countA, 0);
	EXPECT_EQ(countB, 1);
}

TEST(EventEmitterTest, OffByIdWithInvalidIdDoesNothing) {
	EventEmitter emitter;
	int count = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { ++count; });
	emitter.off(99999);  // non-existent id
	emitter.emit("evt");
	EXPECT_EQ(count, 1);
}

// --- off(event) ---

TEST(EventEmitterTest, OffByEventRemovesAllListenersForEvent) {
	EventEmitter emitter;
	int countA = 0, countB = 0;
	emitter.on("a", [&](const std::vector<std::any>&) { ++countA; });
	emitter.on("a", [&](const std::vector<std::any>&) { ++countA; });
	emitter.on("b", [&](const std::vector<std::any>&) { ++countB; });
	emitter.off("a");
	emitter.emit("a");
	emitter.emit("b");
	EXPECT_EQ(countA, 0);
	EXPECT_EQ(countB, 1);
}

TEST(EventEmitterTest, OffByNonExistentEventDoesNothing) {
	EventEmitter emitter;
	int count = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { ++count; });
	emitter.off("nonexistent");
	emitter.emit("evt");
	EXPECT_EQ(count, 1);
}

// --- emit() ---

TEST(EventEmitterTest, EmitWithNoListenersDoesNotCrash) {
	EventEmitter emitter;
	EXPECT_NO_THROW(emitter.emit("nonexistent"));
}

TEST(EventEmitterTest, EmitPassesArguments) {
	EventEmitter emitter;
	int received = 0;
	emitter.on("evt", [&](const std::vector<std::any>& args) {
		ASSERT_EQ(args.size(), 1u);
		received = std::any_cast<int>(args[0]);
	});
	emitter.emit("evt", {42});
	EXPECT_EQ(received, 42);
}

TEST(EventEmitterTest, EmitMultipleArguments) {
	EventEmitter emitter;
	std::string received;
	int num = 0;
	emitter.on("evt", [&](const std::vector<std::any>& args) {
		ASSERT_EQ(args.size(), 2u);
		num = std::any_cast<int>(args[0]);
		received = std::any_cast<std::string>(args[1]);
	});
	emitter.emit("evt", {42, std::string("hello")});
	EXPECT_EQ(num, 42);
	EXPECT_EQ(received, "hello");
}

TEST(EventEmitterTest, EmitWithEmptyArgs) {
	EventEmitter emitter;
	bool called = false;
	emitter.on("evt", [&](const std::vector<std::any>& args) {
		EXPECT_TRUE(args.empty());
		called = true;
	});
	emitter.emit("evt");
	EXPECT_TRUE(called);
}

// --- Exception safety ---

TEST(EventEmitterTest, ListenerExceptionDoesNotPreventOtherListeners) {
	EventEmitter emitter;
	int count = 0;
	emitter.on("evt", [](const std::vector<std::any>&) {
		throw std::runtime_error("boom");
	});
	emitter.on("evt", [&](const std::vector<std::any>&) { ++count; });
	EXPECT_NO_THROW(emitter.emit("evt"));
	EXPECT_EQ(count, 1);
}

TEST(EventEmitterTest, BadAnyCastInListenerIsCaught) {
	EventEmitter emitter;
	bool afterCalled = false;
	emitter.on("evt", [](const std::vector<std::any>& args) {
		// Try to cast int to string - will throw bad_any_cast
		std::any_cast<std::string>(args[0]);
	});
	emitter.on("evt", [&](const std::vector<std::any>&) { afterCalled = true; });
	EXPECT_NO_THROW(emitter.emit("evt", {42}));
	EXPECT_TRUE(afterCalled);
}

// --- Different events are independent ---

TEST(EventEmitterTest, DifferentEventsAreIndependent) {
	EventEmitter emitter;
	int countA = 0, countB = 0;
	emitter.on("a", [&](const std::vector<std::any>&) { ++countA; });
	emitter.on("b", [&](const std::vector<std::any>&) { ++countB; });
	emitter.emit("a");
	EXPECT_EQ(countA, 1);
	EXPECT_EQ(countB, 0);
}

// --- Thread safety ---

TEST(EventEmitterTest, ConcurrentEmitAndOnDoNotCrash) {
	EventEmitter emitter;
	std::atomic<int> totalCalls{0};

	// Register initial listener
	emitter.on("evt", [&](const std::vector<std::any>&) { totalCalls++; });

	// Concurrently emit and register
	std::vector<std::thread> threads;
	for (int i = 0; i < 4; ++i) {
		threads.emplace_back([&, i]() {
			for (int j = 0; j < 100; ++j) {
				if (i % 2 == 0) {
					emitter.emit("evt");
				} else {
					emitter.on("evt", [&](const std::vector<std::any>&) { totalCalls++; });
				}
			}
		});
	}
	for (auto& t : threads) t.join();
	EXPECT_GT(totalCalls.load(), 0);
}

TEST(EventEmitterTest, ConcurrentOffDoesNotCrash) {
	EventEmitter emitter;
	std::vector<uint64_t> ids;
	for (int i = 0; i < 100; ++i) {
		ids.push_back(emitter.on("evt", [](const std::vector<std::any>&) {}));
	}

	std::vector<std::thread> threads;
	for (int i = 0; i < 4; ++i) {
		threads.emplace_back([&, i]() {
			for (int j = i * 25; j < (i + 1) * 25; ++j) {
				emitter.off(ids[j]);
			}
		});
	}
	for (auto& t : threads) t.join();

	int count = 0;
	emitter.on("evt", [&](const std::vector<std::any>&) { ++count; });
	emitter.emit("evt");
	EXPECT_EQ(count, 1);  // only the new listener remains
}
