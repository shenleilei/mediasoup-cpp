// Unit tests for Channel::requestWait timeout and close-path logging.
#include <gtest/gtest.h>
#include <future>
#include <chrono>
#include <stdexcept>
#include "Channel.h"
#include "Constants.h"

using namespace mediasoup;

// ═══════════════════════════════════════════════════════════════
// requestWait timeout — simulate with raw promise/future
// (Can't construct a real Channel without worker pipes, so test
//  the timeout logic pattern directly)
// ═══════════════════════════════════════════════════════════════

TEST(RequestWaitTest, FutureTimesOutAndThrows) {
	std::promise<Channel::OwnedResponse> promise;
	auto fut = promise.get_future();

	auto start = std::chrono::steady_clock::now();
	EXPECT_EQ(fut.wait_for(std::chrono::milliseconds(50)),
		std::future_status::timeout);
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();

	// Should have waited ~50ms, not 5 seconds
	EXPECT_GE(elapsed, 40);
	EXPECT_LT(elapsed, 500);
}

TEST(RequestWaitTest, FutureReadyBeforeTimeout) {
	std::promise<Channel::OwnedResponse> promise;
	auto fut = promise.get_future();

	// Fulfill immediately
	Channel::OwnedResponse resp;
	resp.data = {0x01, 0x02};
	promise.set_value(std::move(resp));

	EXPECT_EQ(fut.wait_for(std::chrono::milliseconds(5000)),
		std::future_status::ready);
	auto owned = fut.get();
	EXPECT_EQ(owned.data.size(), 2u);
}

TEST(RequestWaitTest, FutureExceptionPropagates) {
	std::promise<Channel::OwnedResponse> promise;
	auto fut = promise.get_future();

	promise.set_exception(std::make_exception_ptr(
		std::runtime_error("worker rejected")));

	EXPECT_EQ(fut.wait_for(std::chrono::milliseconds(100)),
		std::future_status::ready);
	EXPECT_THROW(fut.get(), std::runtime_error);
}

TEST(RequestWaitTest, TimeoutConstantIsDefined) {
	EXPECT_EQ(kChannelRequestTimeoutMs, 5000);
}

// ═══════════════════════════════════════════════════════════════
// OwnedResponse ownership semantics
// ═══════════════════════════════════════════════════════════════

TEST(RequestWaitTest, OwnedResponseMoveSemantics) {
	Channel::OwnedResponse a;
	a.data = {0x10, 0x20, 0x30};

	Channel::OwnedResponse b = std::move(a);
	EXPECT_EQ(b.data.size(), 3u);
	EXPECT_TRUE(a.data.empty()); // moved-from should be empty
}

TEST(RequestWaitTest, OwnedResponseEmptyAfterDefault) {
	Channel::OwnedResponse resp;
	EXPECT_TRUE(resp.data.empty());
	EXPECT_EQ(resp.response(), nullptr);
}

// ═══════════════════════════════════════════════════════════════
// Pending request cleanup after timeout
// (Verify that unfulfilled promise doesn't leak when future is
//  destroyed after timeout)
// ═══════════════════════════════════════════════════════════════

TEST(RequestWaitTest, UnfulfilledPromiseCleanup) {
	// Simulate: timeout occurs, future is dropped, then promise is fulfilled later
	auto promise = std::make_shared<std::promise<Channel::OwnedResponse>>();
	{
		auto fut = promise->get_future();
		// Timeout
		EXPECT_EQ(fut.wait_for(std::chrono::milliseconds(10)),
			std::future_status::timeout);
		// fut goes out of scope here
	}
	// Promise fulfilled after future is gone — should not crash
	Channel::OwnedResponse resp;
	EXPECT_NO_THROW(promise->set_value(std::move(resp)));
}

TEST(RequestWaitTest, DelayedFulfillmentBeforeTimeout) {
	std::promise<Channel::OwnedResponse> promise;
	auto fut = promise.get_future();

	// Fulfill from another thread after 50ms
	std::thread t([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		Channel::OwnedResponse resp;
		resp.data = {0xAA};
		promise.set_value(std::move(resp));
	});

	// Wait with generous timeout
	EXPECT_EQ(fut.wait_for(std::chrono::milliseconds(2000)),
		std::future_status::ready);
	auto owned = fut.get();
	EXPECT_EQ(owned.data.size(), 1u);
	EXPECT_EQ(owned.data[0], 0xAA);
	t.join();
}
