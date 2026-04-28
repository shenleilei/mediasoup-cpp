#include <gtest/gtest.h>

#include "RoomRegistry.h"
#include "TestRedisServer.h"

#include <hiredis/hiredis.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

namespace {

bool seedRemoteNodeKeys(int redisPort, size_t nodeCount)
{
	redisContext* ctx = redisConnect("127.0.0.1", redisPort);
	if (!ctx || ctx->err) {
		if (ctx) redisFree(ctx);
		return false;
	}

	size_t pendingReplies = 0;
	for (size_t index = 0; index < nodeCount; ++index) {
		const std::string key = "sfu:node:remote_" + std::to_string(index);
		const std::string value =
			"ws://127.0.0.1:" + std::to_string(20000 + static_cast<int>(index % 10000))
			+ "|0|0|0|0||";
		if (redisAppendCommand(ctx, "SET %s %s EX 300", key.c_str(), value.c_str()) != REDIS_OK) {
			redisFree(ctx);
			return false;
		}
		pendingReplies++;
		if (pendingReplies < 256 && index + 1 < nodeCount) {
			continue;
		}

		for (size_t replyIndex = 0; replyIndex < pendingReplies; ++replyIndex) {
			void* rawReply = nullptr;
			if (redisGetReply(ctx, &rawReply) != REDIS_OK || rawReply == nullptr) {
				redisFree(ctx);
				return false;
			}
			freeReplyObject(rawReply);
		}
		pendingReplies = 0;
	}

	redisFree(ctx);
	return true;
}

} // namespace

TEST(RoomRegistrySyncIntegration, ConcurrentUpdateLoadDoesNotWaitForWholeResolveRefresh)
{
	TestRedisServer redisServer;
	ASSERT_TRUE(redisServer.start()) << redisServer.failureMessage();

	mediasoup::RoomRegistry registry(
		"127.0.0.1",
		redisServer.port(),
		"self-node",
		"ws://127.0.0.1:15000");

	registry.updateLoad(0, 16);
	ASSERT_TRUE(seedRemoteNodeKeys(redisServer.port(), 12000))
		<< "failed to seed remote node keys";

	std::atomic<int64_t> resolveElapsedMs{-1};
	auto resolveFuture = std::async(std::launch::async, [&]() {
		const auto start = std::chrono::steady_clock::now();
		const auto result = registry.resolveRoom("sync-locking-room", "127.0.0.1");
		resolveElapsedMs.store(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - start).count(),
			std::memory_order_relaxed);
		return result;
	});

	std::this_thread::sleep_for(std::chrono::milliseconds(25));

	const auto updateStart = std::chrono::steady_clock::now();
	registry.updateLoad(1, 16);
	const auto updateElapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - updateStart).count();

	EXPECT_EQ(resolveFuture.wait_for(std::chrono::milliseconds(0)), std::future_status::timeout)
		<< "concurrent updateLoad should not wait for the whole resolve-driven sync refresh";

	const auto resolveResult = resolveFuture.get();
	const auto totalResolveMs = resolveElapsedMs.load(std::memory_order_relaxed);

	EXPECT_FALSE(resolveResult.wsUrl.empty());
	EXPECT_GE(totalResolveMs, 0);
	EXPECT_LT(updateElapsedMs, totalResolveMs)
		<< "updateLoad should complete before the whole sync-triggered resolve refresh";

	registry.stop();
}
