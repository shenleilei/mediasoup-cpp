// Unit tests for SignalingServer worker thread offloading:
// - Task queue basic operation (post + execute)
// - Task ordering preserved (FIFO)
// - Worker thread stops cleanly
// - Tasks survive high concurrency posting
#include <gtest/gtest.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>
#include <vector>
#include <chrono>

// Minimal reproduction of SignalingServer's worker thread + task queue
// (avoids pulling in uWS/RoomService dependencies for unit testing)
class WorkerQueue {
public:
	void start() {
		thread_ = std::thread(&WorkerQueue::loop, this);
	}

	void stop() {
		{
			std::lock_guard<std::mutex> lock(mu_);
			stop_ = true;
		}
		cv_.notify_one();
		if (thread_.joinable()) thread_.join();
	}

	void post(std::function<void()> fn) {
		{
			std::lock_guard<std::mutex> lock(mu_);
			queue_.push(std::move(fn));
		}
		cv_.notify_one();
	}

	size_t pending() {
		std::lock_guard<std::mutex> lock(mu_);
		return queue_.size();
	}

private:
	void loop() {
		while (true) {
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(mu_);
				cv_.wait(lock, [this]{ return stop_ || !queue_.empty(); });
				if (stop_ && queue_.empty()) return;
				task = std::move(queue_.front());
				queue_.pop();
			}
			task();
		}
	}

	std::thread thread_;
	std::mutex mu_;
	std::condition_variable cv_;
	std::queue<std::function<void()>> queue_;
	bool stop_ = false;
};

// ─── Test 1: Basic post and execute ───
TEST(WorkerQueueTest, PostAndExecute) {
	WorkerQueue wq;
	wq.start();

	std::atomic<bool> executed{false};
	wq.post([&]{ executed = true; });

	// Wait for execution
	for (int i = 0; i < 100 && !executed; ++i)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	EXPECT_TRUE(executed);
	wq.stop();
}

// ─── Test 2: FIFO ordering preserved ───
TEST(WorkerQueueTest, FifoOrdering) {
	WorkerQueue wq;
	wq.start();

	std::vector<int> order;
	std::mutex mu;
	std::condition_variable done;
	constexpr int N = 100;

	for (int i = 0; i < N; ++i) {
		wq.post([&, i]{
			std::lock_guard<std::mutex> lock(mu);
			order.push_back(i);
			if ((int)order.size() == N) done.notify_one();
		});
	}

	{
		std::unique_lock<std::mutex> lock(mu);
		done.wait_for(lock, std::chrono::seconds(5), [&]{ return (int)order.size() == N; });
	}

	ASSERT_EQ((int)order.size(), N);
	for (int i = 0; i < N; ++i)
		EXPECT_EQ(order[i], i) << "Out of order at index " << i;
	wq.stop();
}

// ─── Test 3: Stop drains remaining tasks ───
TEST(WorkerQueueTest, StopDrainsQueue) {
	WorkerQueue wq;
	wq.start();

	// Post a slow task to block the worker
	std::atomic<int> count{0};
	wq.post([&]{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		count++;
	});

	// Post more tasks while worker is busy
	for (int i = 0; i < 5; ++i)
		wq.post([&]{ count++; });

	wq.stop();  // should wait for all tasks to complete
	EXPECT_EQ(count.load(), 6);
}

// ─── Test 4: Concurrent posting from multiple threads ───
TEST(WorkerQueueTest, ConcurrentPosting) {
	WorkerQueue wq;
	wq.start();

	std::atomic<int> count{0};
	constexpr int THREADS = 8;
	constexpr int PER_THREAD = 50;

	std::vector<std::thread> posters;
	for (int t = 0; t < THREADS; ++t) {
		posters.emplace_back([&]{
			for (int i = 0; i < PER_THREAD; ++i)
				wq.post([&]{ count++; });
		});
	}
	for (auto& t : posters) t.join();

	wq.stop();
	EXPECT_EQ(count.load(), THREADS * PER_THREAD);
}

// ─── Test 5: Blocking task doesn't block poster ───
TEST(WorkerQueueTest, BlockingTaskDoesNotBlockPoster) {
	WorkerQueue wq;
	wq.start();

	// Post a task that blocks for 500ms
	wq.post([]{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	});

	// Posting should return immediately (< 10ms)
	auto start = std::chrono::steady_clock::now();
	std::atomic<bool> done{false};
	wq.post([&]{ done = true; });
	auto elapsed = std::chrono::steady_clock::now() - start;
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
	EXPECT_LT(ms, 50) << "post() blocked for " << ms << "ms";

	wq.stop();
	EXPECT_TRUE(done);
}

// ─── Test 6: Alive token pattern (ws close detection) ───
TEST(WorkerQueueTest, AliveTokenPattern) {
	WorkerQueue wq;
	wq.start();

	auto alive = std::make_shared<std::atomic<bool>>(true);
	std::atomic<int> sent{0};

	// Simulate: post work, then "close" before work completes
	wq.post([]{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	});

	wq.post([alive, &sent]{
		if (alive->load()) sent++;
	});

	// "Close" the connection before the second task runs
	alive->store(false);

	wq.stop();
	EXPECT_EQ(sent.load(), 0) << "Should not send after alive=false";
}
