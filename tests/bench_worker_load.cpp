// Worker load benchmark: simulated 1080p-equivalent RTP load, 1 Producer + 2 Consumers per room
// Uses audio/opus with 1200-byte packets at 300 pps (equivalent bandwidth to 1080p 30fps video)
// Video codecs require keyframe negotiation which doesn't work with PlainTransport-only setup
// Usage: ./mediasoup_bench [--rooms-per-round=10] [--warmup-sec=20] [--round-sec=5]
//
// Data flow per room:
//   sender UDP sock ──RTP──► PlainTransport(producer, comedia=false)
//                                │ Router
//                                ├──► PlainTransport(consumer1, PIPE)   ──► recv sock 1
//                                └──► PlainTransport(consumer2, PIPE)   ──► recv sock 2

#include "Worker.h"
#include "Router.h"
#include "PlainTransport.h"
#include "Producer.h"
#include "Consumer.h"
#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <random>

using namespace mediasoup;
using json = nlohmann::json;
using Clock = std::chrono::steady_clock;

static std::atomic<bool> g_stop{false};
static void sigHandler(int) { g_stop = true; }
static std::string g_bindIp = "127.0.0.1"; // --ip= to override

// ── /proc helpers ──────────────────────────────────────────────────────────────

struct CpuSample {
	uint64_t totalJiffies = 0;
	uint64_t procJiffies = 0;
};

static CpuSample sampleCpu(int pid) {
	CpuSample s;
	{
		std::ifstream f("/proc/stat");
		std::string line;
		if (std::getline(f, line) && line.substr(0, 3) == "cpu") {
			std::istringstream iss(line.substr(5));
			uint64_t v;
			while (iss >> v) s.totalJiffies += v;
		}
	}
	{
		std::ifstream f("/proc/" + std::to_string(pid) + "/stat");
		std::string line;
		if (std::getline(f, line)) {
			auto pos = line.rfind(')');
			if (pos != std::string::npos) {
				std::istringstream iss(line.substr(pos + 2));
				std::string tok;
				for (int i = 1; i <= 13; i++) {
					iss >> tok;
					if (i == 12 || i == 13)
						s.procJiffies += std::stoull(tok);
				}
			}
		}
	}
	return s;
}

static double cpuPercent(const CpuSample& a, const CpuSample& b) {
	auto dt = b.totalJiffies - a.totalJiffies;
	auto dp = b.procJiffies - a.procJiffies;
	if (dt == 0) return 0.0;
	// Normalize: /proc/stat total is across all cores, so multiply by ncpu
	// to get single-core percentage
	return 100.0 * (double)dp / (double)dt * std::thread::hardware_concurrency();
}

static uint64_t readRssKb(int pid) {
	std::ifstream f("/proc/" + std::to_string(pid) + "/status");
	std::string line;
	while (std::getline(f, line)) {
		if (line.find("VmRSS:") == 0) {
			std::istringstream iss(line.substr(6));
			uint64_t kb;
			iss >> kb;
			return kb;
		}
	}
	return 0;
}

static uint64_t readUdpDrops() {
	uint64_t total = 0;
	std::ifstream f("/proc/net/udp");
	std::string line;
	std::getline(f, line); // skip header
	while (std::getline(f, line)) {
		// The drops field is the last (13th) column
		std::istringstream iss(line);
		std::vector<std::string> tokens;
		std::string tok;
		while (iss >> tok) tokens.push_back(tok);
		if (tokens.size() >= 13) {
			try { total += std::stoull(tokens[12]); } catch (...) {}
		}
	}
	return total;
}

static bool workerAlive(int pid) {
	return kill(pid, 0) == 0;
}

// ── UDP socket helper ──────────────────────────────────────────────────────────

static int createUdpSocket(uint16_t& outPort, int rcvBuf = 0) {
	int fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	if (fd < 0) return -1;
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, g_bindIp.c_str(), &addr.sin_addr);
	addr.sin_port = 0;
	if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	socklen_t alen = sizeof(addr);
	getsockname(fd, (sockaddr*)&addr, &alen);
	outPort = ntohs(addr.sin_port);
	if (rcvBuf > 0)
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvBuf, sizeof(rcvBuf));
	return fd;
}

// ── BenchRoom ──────────────────────────────────────────────────────────────────

static const int PKTS_PER_FRAME = 10;
static const int PKT_SIZE = 1200;
static const uint32_t TS_INCREMENT = 90000 / 30; // 3000 per frame at 90kHz clock

struct BenchRoom {
	std::shared_ptr<Router> router;
	std::shared_ptr<PlainTransport> prodTransport;
	std::shared_ptr<PlainTransport> cons1Transport;
	std::shared_ptr<PlainTransport> cons2Transport;
	std::shared_ptr<Producer> producer;
	std::shared_ptr<Consumer> consumer1;
	std::shared_ptr<Consumer> consumer2;

	int senderFd = -1;
	int recv1Fd = -1;
	int recv2Fd = -1;
	uint16_t prodPort = 0;  // Worker's PlainTransport listen port
	uint16_t recv1Port = 0;
	uint16_t recv2Port = 0;

	uint16_t seq = 0;
	uint32_t ts = 0;
	uint32_t ssrc = 0;

	// Aligned to avoid false sharing. Written by sender/receiver threads respectively.
	alignas(64) volatile uint64_t sentPkts = 0;
	alignas(64) volatile uint64_t recv1Pkts = 0;
	alignas(64) volatile uint64_t recv2Pkts = 0;

	void resetCounters() {
		sentPkts = 0;
		recv1Pkts = 0;
		recv2Pkts = 0;
	}

	void cleanup() {
		if (senderFd >= 0) { close(senderFd); senderFd = -1; }
		if (recv1Fd >= 0) { close(recv1Fd); recv1Fd = -1; }
		if (recv2Fd >= 0) { close(recv2Fd); recv2Fd = -1; }
		if (router) { try { router->close(); } catch (...) {} }
	}
};

static void sendFrame(BenchRoom& room) {
	uint8_t buf[PKT_SIZE];
	memset(buf, 0x00, PKT_SIZE);

	sockaddr_in dest{};
	dest.sin_family = AF_INET;
	inet_pton(AF_INET, g_bindIp.c_str(), &dest.sin_addr);
	dest.sin_port = htons(room.prodPort);

	for (int i = 0; i < PKTS_PER_FRAME; i++) {
		buf[0] = 0x80; // V=2
		buf[1] = 100 | ((i == PKTS_PER_FRAME - 1) ? 0x80 : 0x00); // PT=100 (opus)
		buf[2] = (room.seq >> 8) & 0xFF;
		buf[3] = room.seq & 0xFF;
		buf[4] = (room.ts >> 24) & 0xFF;
		buf[5] = (room.ts >> 16) & 0xFF;
		buf[6] = (room.ts >> 8) & 0xFF;
		buf[7] = room.ts & 0xFF;
		buf[8] = (room.ssrc >> 24) & 0xFF;
		buf[9] = (room.ssrc >> 16) & 0xFF;
		buf[10] = (room.ssrc >> 8) & 0xFF;
		buf[11] = room.ssrc & 0xFF;

		// Opus payload (dummy)
		memset(buf + 12, 0xFC, PKT_SIZE - 12);

		sendto(room.senderFd, buf, PKT_SIZE, 0, (sockaddr*)&dest, sizeof(dest));
		room.seq++;
		room.sentPkts++;
	}
	room.ts += TS_INCREMENT;
}

// ── Room factory ───────────────────────────────────────────────────────────────

static const std::vector<json> kMediaCodecs = {
	{{"mimeType", "audio/opus"}, {"clockRate", 48000}, {"channels", 2}},
	{{"mimeType", "video/VP8"}, {"clockRate", 90000}}
};

static BenchRoom createBenchRoom(std::shared_ptr<Worker>& worker) {
	BenchRoom room;

	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<uint32_t> dist(100000000, 999999999);
	room.ssrc = dist(rng);
	room.seq = 1; // Start from 1, not 0

	room.router = worker->createRouter(kMediaCodecs);
	auto& caps = room.router->rtpCapabilities();

	// Find opus PT assigned by router
	uint8_t codecPt = 100;
	for (auto& c : caps.codecs) {
		if (c.mimeType.find("opus") != std::string::npos &&
			c.mimeType.find("rtx") == std::string::npos) {
			codecPt = c.preferredPayloadType;
			break;
		}
	}

	PlainTransportOptions opts;
	opts.listenInfos = {{{"ip", g_bindIp}}};
	opts.rtcpMux = true;

	// ── Producer PlainTransport (comedia=false, explicit connect) ──
	opts.comedia = false;
	room.prodTransport = room.router->createPlainTransport(opts);
	room.prodPort = room.prodTransport->tuple().localPort;

	// Sender socket
	uint16_t senderPort;
	room.senderFd = createUdpSocket(senderPort);

	// Connect producer transport: tell Worker our sender address
	room.prodTransport->connect(g_bindIp, senderPort);

	json rtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"},
			{"payloadType", codecPt},
			{"clockRate", 48000},
			{"channels", 2}
		}}},
		{"encodings", {{{"ssrc", room.ssrc}}}},
		{"mid", "0"}
	};
	json produceOpts = {
		{"kind", "audio"},
		{"rtpParameters", rtpParams},
		{"routerRtpCapabilities", caps}
	};
	room.producer = room.prodTransport->produce(produceOpts);

	// ── Consumer1 PlainTransport ──
	opts.comedia = false;
	room.cons1Transport = room.router->createPlainTransport(opts);
	room.recv1Fd = createUdpSocket(room.recv1Port, 4 * 1024 * 1024);
	room.cons1Transport->connect(g_bindIp, room.recv1Port);

	json consumeOpts1 = {
		{"producerId", room.producer->id()},
		{"rtpCapabilities", caps},
		{"consumableRtpParameters", room.producer->consumableRtpParameters()},
		{"pipe", true}
	};
	room.consumer1 = room.cons1Transport->consume(consumeOpts1);

	// ── Consumer2 PlainTransport (simulates recording) ──
	opts.comedia = false;
	room.cons2Transport = room.router->createPlainTransport(opts);
	room.recv2Fd = createUdpSocket(room.recv2Port, 4 * 1024 * 1024);
	room.cons2Transport->connect(g_bindIp, room.recv2Port);

	json consumeOpts2 = {
		{"producerId", room.producer->id()},
		{"rtpCapabilities", caps},
		{"consumableRtpParameters", room.producer->consumableRtpParameters()},
		{"pipe", true}
	};
	room.consumer2 = room.cons2Transport->consume(consumeOpts2);

	return room;
}

// ── Receiver with epoll ────────────────────────────────────────────────────────

struct EpollReceiver {
	int epfd = -1;
	struct FdInfo { volatile uint64_t* counter; };
	std::unordered_map<int, FdInfo> fds;
	std::mutex mu;

	EpollReceiver() { epfd = epoll_create1(0); }
	~EpollReceiver() { if (epfd >= 0) close(epfd); }

	void addFd(int fd, volatile uint64_t* counter) {
		std::lock_guard<std::mutex> lock(mu);
		epoll_event ev{};
		ev.events = EPOLLIN | EPOLLET;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
		fds[fd] = {counter};
	}

	void poll() {
		uint8_t buf[2048];
		epoll_event events[256];
		int n = epoll_wait(epfd, events, 256, 5); // 5ms timeout
		for (int i = 0; i < n; i++) {
			int fd = events[i].data.fd;
			volatile uint64_t* counter = nullptr;
			{
				std::lock_guard<std::mutex> lock(mu);
				auto it = fds.find(fd);
				if (it != fds.end()) counter = it->second.counter;
			}
			if (!counter) continue;
			// Drain all available packets (edge-triggered)
			while (true) {
				int r = recv(fd, buf, sizeof(buf), MSG_DONTWAIT);
				if (r <= 0) break;
				(*counter)++;
			}
		}
	}
};

// ── Main ───────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	int roomsPerRound = 10;
	int warmupSec = 20;
	int roundSec = 5;
	int maxRooms = 500;

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg.find("--rooms-per-round=") == 0)
			roomsPerRound = std::stoi(arg.substr(18));
		else if (arg.find("--warmup-sec=") == 0)
			warmupSec = std::stoi(arg.substr(13));
		else if (arg.find("--round-sec=") == 0)
			roundSec = std::stoi(arg.substr(12));
		else if (arg.find("--max-rooms=") == 0)
			maxRooms = std::stoi(arg.substr(12));
		else if (arg.find("--ip=") == 0)
			g_bindIp = arg.substr(5);
	}

	printf("=== mediasoup Worker Load Benchmark ===\n");
	printf("1080p-equivalent RTP load (opus %d-byte packets), 1P+2C per room, %d pps/stream\n",
		PKT_SIZE, PKTS_PER_FRAME * 30);
	printf("Rooms/round=%d, warmup=%ds, round=%ds, max=%d, ip=%s\n\n",
		roomsPerRound, warmupSec, roundSec, maxRooms, g_bindIp.c_str());

	// ── Create Worker ──
	WorkerSettings ws;
	ws.workerBin = "./mediasoup-worker";
	ws.logLevel = "warn";
	ws.rtcMinPort = 10000;
	ws.rtcMaxPort = 59999;

	std::shared_ptr<Worker> worker;
	try {
		worker = std::make_shared<Worker>(ws);
	} catch (const std::exception& e) {
		fprintf(stderr, "Failed to create Worker: %s\n", e.what());
		return 1;
	}

	int workerPid = worker->pid();
	printf("Worker started [pid:%d]\n", workerPid);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	if (!workerAlive(workerPid)) {
		fprintf(stderr, "Worker died immediately\n");
		return 1;
	}
	printf("Worker RSS: %lu KB\n\n", readRssKb(workerPid));

	// ── Rooms + threads ──
	std::vector<std::unique_ptr<BenchRoom>> rooms;
	std::mutex roomsMutex;

	// Sender threads: round-robin rooms, 30fps per room
	std::atomic<bool> running{true};
	const int NUM_SENDERS = 2;

	auto senderFn = [&](int threadIdx) {
		while (running && !g_stop) {
			auto t0 = Clock::now();
			{
				std::lock_guard<std::mutex> lock(roomsMutex);
				for (size_t i = threadIdx; i < rooms.size(); i += NUM_SENDERS)
					sendFrame(*rooms[i]);
			}
			auto elapsed = Clock::now() - t0;
			auto remain = std::chrono::microseconds(33333) - elapsed;
			if (remain.count() > 0)
				std::this_thread::sleep_for(remain);
		}
	};

	// Receiver threads with epoll
	const int NUM_RECEIVERS = 2;
	std::vector<EpollReceiver> receivers(NUM_RECEIVERS);

	auto receiverFn = [&](int threadIdx) {
		while (running && !g_stop)
			receivers[threadIdx].poll();
	};

	// Helper: register a room's recv fds with receivers
	auto registerRoom = [&](BenchRoom& r, size_t roomIdx) {
		int ri = roomIdx % NUM_RECEIVERS;
		receivers[ri].addFd(r.recv1Fd, &r.recv1Pkts);
		receivers[ri].addFd(r.recv2Fd, &r.recv2Pkts);
	};

	// Start threads
	std::vector<std::thread> senderThreads, receiverThreads;
	for (int i = 0; i < NUM_SENDERS; i++)
		senderThreads.emplace_back(senderFn, i);
	for (int i = 0; i < NUM_RECEIVERS; i++)
		receiverThreads.emplace_back(receiverFn, i);

	// ── Warmup ──
	printf("Creating %d warmup rooms...\n", roomsPerRound);
	for (int i = 0; i < roomsPerRound && !g_stop; i++) {
		try {
			auto r = std::make_unique<BenchRoom>(createBenchRoom(worker));
			registerRoom(*r, rooms.size());
			std::lock_guard<std::mutex> lock(roomsMutex);
			rooms.push_back(std::move(r));
		} catch (const std::exception& e) {
			fprintf(stderr, "Warmup room creation failed: %s\n", e.what());
			break;
		}
	}

	printf("Warming up for %d seconds...\n", warmupSec);
	std::this_thread::sleep_for(std::chrono::seconds(warmupSec));

	{
		std::lock_guard<std::mutex> lock(roomsMutex);
		for (auto& r : rooms) r->resetCounters();
	}
	printf("Warmup done.\n\n");

	// ── Table header ──
	printf("%-6s | %-11s | %-7s | %-9s | %-9s | %-6s | %-8s\n",
		"Rooms", "CPU%(1core)", "RSS(MB)", "Send pps", "Recv pps", "Loss%", "UdpDrops");
	printf("-------+-------------+---------+-----------+-----------+--------+----------\n");
	fflush(stdout);

	int consecutiveHighLoss = 0;
	int peakRooms = 0;
	double peakCpu = 0;
	uint64_t peakRssKb = 0;

	// ── Main loop ──
	while (!g_stop && (int)rooms.size() < maxRooms) {
		// Add rooms
		int toAdd = roomsPerRound;
		if ((int)rooms.size() + toAdd > maxRooms)
			toAdd = maxRooms - (int)rooms.size();

		for (int i = 0; i < toAdd && !g_stop; i++) {
			try {
				auto r = std::make_unique<BenchRoom>(createBenchRoom(worker));
				registerRoom(*r, rooms.size());
				std::lock_guard<std::mutex> lock(roomsMutex);
				rooms.push_back(std::move(r));
			} catch (const std::exception& e) {
				fprintf(stderr, "Room creation failed at %zu rooms: %s\n",
					rooms.size(), e.what());
				g_stop = true;
				break;
			}
		}
		if (g_stop) break;

		// Reset counters for this round's measurement
		{
			std::lock_guard<std::mutex> lock(roomsMutex);
			for (auto& r : rooms) r->resetCounters();
		}

		auto cpuBefore = sampleCpu(workerPid);
		uint64_t udpDropsBefore = readUdpDrops();

		std::this_thread::sleep_for(std::chrono::seconds(roundSec));

		if (!workerAlive(workerPid)) {
			printf("\n*** Worker crashed at %zu rooms! ***\n", rooms.size());
			break;
		}

		auto cpuAfter = sampleCpu(workerPid);
		uint64_t udpDropsAfter = readUdpDrops();
		uint64_t rssKb = readRssKb(workerPid);

		// Collect incremental stats
		uint64_t totalSent = 0, totalRecv = 0;
		{
			std::lock_guard<std::mutex> lock(roomsMutex);
			for (auto& r : rooms) {
				totalSent += r->sentPkts;
				totalRecv += r->recv1Pkts + r->recv2Pkts;
			}
		}

		double cpu = cpuPercent(cpuBefore, cpuAfter);
		uint64_t udpDrops = udpDropsAfter - udpDropsBefore;
		uint64_t expectedRecv = totalSent * 2;
		double loss = expectedRecv > 0
			? 100.0 * (double)(expectedRecv - std::min(totalRecv, expectedRecv)) / (double)expectedRecv
			: 0.0;
		double sendPps = (double)totalSent / roundSec;
		double recvPps = (double)totalRecv / roundSec;

		printf("%5zu | %10.1f%% | %7.1f | %9.0f | %9.0f | %5.1f%% | %8lu\n",
			rooms.size(), cpu, rssKb / 1024.0, sendPps, recvPps, loss, udpDrops);
		fflush(stdout);

		if (loss <= 1.0) {
			peakRooms = (int)rooms.size();
			peakCpu = cpu;
			peakRssKb = rssKb;
			consecutiveHighLoss = 0;
		} else {
			consecutiveHighLoss++;
			if (consecutiveHighLoss >= 3) {
				printf("\n*** Stopping: 3 consecutive rounds with >1%% loss ***\n");
				break;
			}
		}
	}

	// ── Summary ──
	printf("\n=== Result ===\n");
	printf("Peak: %d rooms (1P+2C each), CPU %.1f%% (single core), RSS %.1f MB\n",
		peakRooms, peakCpu, peakRssKb / 1024.0);
	printf("= %d producers + %d consumers on 1 Worker\n",
		peakRooms, peakRooms * 2);
	printf("= %d pps in, %d pps out (theoretical max at peak)\n",
		peakRooms * PKTS_PER_FRAME * 30, peakRooms * PKTS_PER_FRAME * 30 * 2);

	// ── Cleanup ──
	printf("\nCleaning up...\n");
	running = false;
	for (auto& t : senderThreads) t.join();
	for (auto& t : receiverThreads) t.join();
	{
		std::lock_guard<std::mutex> lock(roomsMutex);
		for (auto& r : rooms) r->cleanup();
		rooms.clear();
	}
	worker->close();
	printf("Done.\n");
	return 0;
}
