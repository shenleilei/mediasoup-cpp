#include <gtest/gtest.h>

#include "RoomRegistry.h"
#include "TestRedisServer.h"

#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cerrno>
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

class DisconnectingRedisServer {
public:
	DisconnectingRedisServer() = default;

	~DisconnectingRedisServer()
	{
		stop();
	}

	DisconnectingRedisServer(const DisconnectingRedisServer&) = delete;
	DisconnectingRedisServer& operator=(const DisconnectingRedisServer&) = delete;

	bool start()
	{
		listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
		if (listenFd_ < 0) {
			return false;
		}

		int reuse = 1;
		::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
			::close(listenFd_);
			listenFd_ = -1;
			return false;
		}
		if (::listen(listenFd_, 4) != 0) {
			::close(listenFd_);
			listenFd_ = -1;
			return false;
		}

		socklen_t addrLen = sizeof(addr);
		if (::getsockname(listenFd_, reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0) {
			::close(listenFd_);
			listenFd_ = -1;
			return false;
		}
		port_ = ntohs(addr.sin_port);
		thread_ = std::thread([this] { run(); });
		return true;
	}

	void stop()
	{
		stop_.store(true, std::memory_order_relaxed);
		if (listenFd_ >= 0) {
			::shutdown(listenFd_, SHUT_RDWR);
			::close(listenFd_);
			listenFd_ = -1;
		}
		if (thread_.joinable()) {
			thread_.join();
		}
	}

	int port() const
	{
		return port_;
	}

private:
	void run()
	{
		while (!stop_.load(std::memory_order_relaxed)) {
			int clientFd = ::accept(listenFd_, nullptr, nullptr);
			if (clientFd < 0) {
				if (stop_.load(std::memory_order_relaxed) || errno == EBADF || errno == EINVAL) {
					return;
				}
				if (errno == EINTR) {
					continue;
				}
				return;
			}

			uint8_t buffer[256];
			(void)::read(clientFd, buffer, sizeof(buffer));
			::close(clientFd);
		}
	}

	std::atomic<bool> stop_{false};
	int listenFd_{-1};
	int port_{0};
	std::thread thread_;
};

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

TEST(RoomRegistrySyncIntegration, FullSyncPreservesExistingCacheWhenSnapshotIncomplete)
{
	DisconnectingRedisServer fakeRedis;
	ASSERT_TRUE(fakeRedis.start());

	mediasoup::RoomRegistry registry(
		"127.0.0.1",
		fakeRedis.port(),
		"self-node",
		"ws://127.0.0.1:15000");

	mediasoup::RoomRegistry::NodeInfo staleNode;
	staleNode.address = "ws://127.0.0.1:19999";
	staleNode.rooms = 1;
	staleNode.maxRooms = 16;
	registry.cache_.nodes["stale-node"] = staleNode;
	registry.cache_.rooms["stale-room"] = staleNode.address;

	registry.syncAllSnapshot();

	const auto staleRoomAddress = registry.cache_.roomAddress("stale-room");
	ASSERT_TRUE(staleRoomAddress.has_value());
	EXPECT_EQ(*staleRoomAddress, staleNode.address);
	EXPECT_EQ(registry.cache_.knownNodeCount(), 1u);
	EXPECT_FALSE(registry.isReady());

	registry.stop();
}
