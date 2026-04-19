// test_thread_model.cpp — Unit tests for multi-thread model components
// Tests: SpscQueue, EncodedAccessUnit, SenderStatsSnapshot, TrackControlCommand,
//        NetworkThread (offline), SourceWorker command handling
#include <gtest/gtest.h>

#include "../client/ThreadTypes.h"
#include "../client/NetworkThread.h"
#include "../client/ThreadedControlHelpers.h"
#include "../client/qos/QosController.h"

#include <thread>
#include <chrono>
#include <vector>
#include <numeric>

using namespace mt;

// ═══════════════════════════════════════════════════════════
// SpscQueue unit tests
// ═══════════════════════════════════════════════════════════

TEST(SpscQueue, PushPopSingleItem) {
	SpscQueue<int, 4> q;
	EXPECT_TRUE(q.empty());
	EXPECT_TRUE(q.tryPush(42));
	EXPECT_FALSE(q.empty());
	int val = 0;
	EXPECT_TRUE(q.tryPop(val));
	EXPECT_EQ(val, 42);
	EXPECT_TRUE(q.empty());
}

TEST(SpscQueue, FullQueueRejectsPush) {
	SpscQueue<int, 2> q;
	EXPECT_TRUE(q.tryPush(1));
	EXPECT_TRUE(q.tryPush(2));
	EXPECT_FALSE(q.tryPush(3)); // full
	int val = 0;
	EXPECT_TRUE(q.tryPop(val));
	EXPECT_EQ(val, 1);
	EXPECT_TRUE(q.tryPush(3)); // space freed
}

TEST(SpscQueue, EmptyQueueRejectsPop) {
	SpscQueue<int, 4> q;
	int val = 0;
	EXPECT_FALSE(q.tryPop(val));
}

TEST(SpscQueue, FIFOOrder) {
	SpscQueue<int, 8> q;
	for (int i = 0; i < 8; ++i) { int v = i; EXPECT_TRUE(q.tryPush(std::move(v))); }
	for (int i = 0; i < 8; ++i) {
		int val = -1;
		EXPECT_TRUE(q.tryPop(val));
		EXPECT_EQ(val, i);
	}
}

TEST(SpscQueue, MoveOnlyType) {
	SpscQueue<std::unique_ptr<int>, 4> q;
	auto p = std::make_unique<int>(99);
	EXPECT_TRUE(q.tryPush(std::move(p)));
	EXPECT_EQ(p, nullptr);
	std::unique_ptr<int> out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_NE(out, nullptr);
	EXPECT_EQ(*out, 99);
}

TEST(SpscQueue, ConcurrentProducerConsumer) {
	constexpr int N = 100000;
	SpscQueue<int, 256> q;
	std::vector<int> received;
	received.reserve(N);

	std::thread producer([&]() {
		for (int i = 0; i < N; ++i) {
			int copy = i;
			while (!q.tryPush(std::move(copy))) std::this_thread::yield();
		}
	});

	std::thread consumer([&]() {
		int count = 0;
		while (count < N) {
			int val;
			if (q.tryPop(val)) {
				received.push_back(val);
				count++;
			} else {
				std::this_thread::yield();
			}
		}
	});

	producer.join();
	consumer.join();

	ASSERT_EQ(received.size(), (size_t)N);
	for (int i = 0; i < N; ++i) EXPECT_EQ(received[i], i);
}

// ═══════════════════════════════════════════════════════════
// EncodedAccessUnit tests
// ═══════════════════════════════════════════════════════════

TEST(EncodedAccessUnit, AssignCopiesData) {
	EncodedAccessUnit au;
	uint8_t src[] = {0x00, 0x00, 0x01, 0x65, 0xAA, 0xBB};
	au.assign(src, sizeof(src));
	EXPECT_EQ(au.size, sizeof(src));
	EXPECT_EQ(std::memcmp(au.data.get(), src, sizeof(src)), 0);
}

TEST(EncodedAccessUnit, MoveSemantics) {
	EncodedAccessUnit au;
	au.assign((const uint8_t*)"hello", 5);
	au.ssrc = 12345;
	au.isKeyframe = true;

	EncodedAccessUnit moved = std::move(au);
	EXPECT_EQ(moved.ssrc, 12345u);
	EXPECT_TRUE(moved.isKeyframe);
	EXPECT_EQ(moved.size, 5u);
	EXPECT_EQ(au.data, nullptr);
}

TEST(EncodedAccessUnit, ThroughQueue) {
	SpscQueue<EncodedAccessUnit, 4> q;
	EncodedAccessUnit au;
	au.trackIndex = 1;
	au.ssrc = 11111;
	au.payloadType = 96;
	au.rtpTimestamp = 90000;
	au.isKeyframe = true;
	au.encoderRecreated = true;
	uint8_t payload[] = {0x00, 0x00, 0x01, 0x67, 0x42};
	au.assign(payload, sizeof(payload));

	EXPECT_TRUE(q.tryPush(std::move(au)));

	EncodedAccessUnit out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.trackIndex, 1u);
	EXPECT_EQ(out.ssrc, 11111u);
	EXPECT_EQ(out.payloadType, 96);
	EXPECT_EQ(out.rtpTimestamp, 90000u);
	EXPECT_TRUE(out.isKeyframe);
	EXPECT_TRUE(out.encoderRecreated);
	EXPECT_EQ(out.size, sizeof(payload));
	EXPECT_EQ(std::memcmp(out.data.get(), payload, sizeof(payload)), 0);
}

// ═══════════════════════════════════════════════════════════
// SenderStatsSnapshot tests
// ═══════════════════════════════════════════════════════════

TEST(SenderStatsSnapshot, DefaultValues) {
	SenderStatsSnapshot snap;
	EXPECT_EQ(snap.timestampMs, 0);
	EXPECT_EQ(snap.packetCount, 0u);
	EXPECT_EQ(snap.octetCount, 0u);
	EXPECT_EQ(snap.rrCumulativeLost, 0u);
	EXPECT_DOUBLE_EQ(snap.rrRttMs, -1.0);
	EXPECT_DOUBLE_EQ(snap.rrJitterMs, -1.0);
}

TEST(SenderStatsSnapshot, ThroughQueue) {
	SpscQueue<SenderStatsSnapshot, 4> q;
	SenderStatsSnapshot snap;
	snap.timestampMs = 12345;
	snap.trackIndex = 2;
	snap.ssrc = 22222;
	snap.packetCount = 100;
	snap.octetCount = 50000;
	snap.rrCumulativeLost = 3;
	snap.rrRttMs = 45.5;
	snap.rrJitterMs = 12.3;

	EXPECT_TRUE(q.tryPush(std::move(snap)));
	SenderStatsSnapshot out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.timestampMs, 12345);
	EXPECT_EQ(out.trackIndex, 2u);
	EXPECT_EQ(out.packetCount, 100u);
	EXPECT_DOUBLE_EQ(out.rrRttMs, 45.5);
}

// ═══════════════════════════════════════════════════════════
// TrackControlCommand tests
// ═══════════════════════════════════════════════════════════

TEST(TrackControlCommand, SetEncodingParameters) {
	SpscQueue<TrackControlCommand, 8> q;
	TrackControlCommand cmd;
	cmd.type = TrackCommandType::SetEncodingParameters;
	cmd.trackIndex = 0;
	cmd.bitrateBps = 500000;
	cmd.fps = 15;
	cmd.scaleResolutionDownBy = 2.0;
	EXPECT_TRUE(q.tryPush(std::move(cmd)));

	TrackControlCommand out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.type, TrackCommandType::SetEncodingParameters);
	EXPECT_EQ(out.bitrateBps, 500000);
	EXPECT_EQ(out.fps, 15);
	EXPECT_DOUBLE_EQ(out.scaleResolutionDownBy, 2.0);
}

TEST(TrackControlCommand, LatestWinsSemantics) {
	// Simulate latest-wins: push multiple, consumer only cares about last
	SpscQueue<TrackControlCommand, 8> q;
	for (int br = 100000; br <= 500000; br += 100000) {
		TrackControlCommand cmd;
		cmd.type = TrackCommandType::SetEncodingParameters;
		cmd.bitrateBps = br;
		q.tryPush(std::move(cmd));
	}

	TrackControlCommand last;
	TrackControlCommand tmp;
	while (q.tryPop(tmp)) last = tmp;
	EXPECT_EQ(last.bitrateBps, 500000);
}

// ═══════════════════════════════════════════════════════════
// NetworkToSourceCommand tests
// ═══════════════════════════════════════════════════════════

TEST(NetworkToSourceCommand, ForceKeyframe) {
	SpscQueue<NetworkToSourceCommand, 4> q;
	NetworkToSourceCommand cmd;
	cmd.type = NetworkToSourceCommand::ForceKeyframe;
	cmd.trackIndex = 1;
	EXPECT_TRUE(q.tryPush(std::move(cmd)));

	NetworkToSourceCommand out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.type, NetworkToSourceCommand::ForceKeyframe);
	EXPECT_EQ(out.trackIndex, 1u);
}

// ═══════════════════════════════════════════════════════════
// Backpressure simulation
// ═══════════════════════════════════════════════════════════

TEST(Backpressure, EncodedAUQueueDropsOnFull) {
	SpscQueue<EncodedAccessUnit, 2> q;

	auto makeAU = [](uint32_t ts) {
		EncodedAccessUnit au;
		au.rtpTimestamp = ts;
		au.assign((const uint8_t*)"x", 1);
		return au;
	};

	EXPECT_TRUE(q.tryPush(makeAU(1)));
	EXPECT_TRUE(q.tryPush(makeAU(2)));
	EXPECT_FALSE(q.tryPush(makeAU(3))); // dropped

	EncodedAccessUnit out;
	EXPECT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.rtpTimestamp, 1u); // FIFO preserved
}

// ═══════════════════════════════════════════════════════════
// Integration: multi-queue pipeline simulation
// ═══════════════════════════════════════════════════════════

TEST(PipelineIntegration, SourceToNetworkToControl) {
	// Simulate: source worker → AU queue → network thread → stats queue → control
	SpscQueue<EncodedAccessUnit, 16> auQueue;
	SpscQueue<SenderStatsSnapshot, 8> statsQueue;

	// Source worker produces AUs
	for (int i = 0; i < 5; ++i) {
		EncodedAccessUnit au;
		au.trackIndex = 0;
		au.ssrc = 11111;
		au.rtpTimestamp = i * 3000;
		au.isKeyframe = (i == 0);
		au.assign((const uint8_t*)"frame", 5);
		EXPECT_TRUE(auQueue.tryPush(std::move(au)));
	}

	// Network thread drains AUs and produces stats
	int auCount = 0;
	EncodedAccessUnit au;
	while (auQueue.tryPop(au)) auCount++;
	EXPECT_EQ(auCount, 5);

	SenderStatsSnapshot snap;
	snap.trackIndex = 0;
	snap.ssrc = 11111;
	snap.packetCount = 25; // 5 frames * ~5 RTP packets
	snap.octetCount = 6000;
	snap.rrRttMs = 50.0;
	EXPECT_TRUE(statsQueue.tryPush(std::move(snap)));

	// Control thread reads stats
	SenderStatsSnapshot ctrlSnap;
	EXPECT_TRUE(statsQueue.tryPop(ctrlSnap));
	EXPECT_EQ(ctrlSnap.packetCount, 25u);
	EXPECT_DOUBLE_EQ(ctrlSnap.rrRttMs, 50.0);
}

TEST(PipelineIntegration, ControlCommandToSourceWorker) {
	// Simulate: control → command queue → source worker reads
	SpscQueue<TrackControlCommand, 8> ctrlQueue;
	SpscQueue<NetworkToSourceCommand, 8> netCmdQueue;

	// QoS decides to lower bitrate
	TrackControlCommand cmd;
	cmd.type = TrackCommandType::SetEncodingParameters;
	cmd.bitrateBps = 450000;
	cmd.fps = 12;
	cmd.scaleResolutionDownBy = 2.0;
	EXPECT_TRUE(ctrlQueue.tryPush(std::move(cmd)));

	// Network thread forwards PLI
	NetworkToSourceCommand ncmd;
	ncmd.type = NetworkToSourceCommand::ForceKeyframe;
	ncmd.trackIndex = 0;
	EXPECT_TRUE(netCmdQueue.tryPush(std::move(ncmd)));

	// Source worker drains both
	TrackControlCommand outCmd;
	EXPECT_TRUE(ctrlQueue.tryPop(outCmd));
	EXPECT_EQ(outCmd.type, TrackCommandType::SetEncodingParameters);
	EXPECT_EQ(outCmd.bitrateBps, 450000);

	NetworkToSourceCommand outNcmd;
	EXPECT_TRUE(netCmdQueue.tryPop(outNcmd));
	EXPECT_EQ(outNcmd.type, NetworkToSourceCommand::ForceKeyframe);
}

TEST(PipelineIntegration, MultiTrackQueuesIndependent) {
	// Each track has its own set of queues — verify isolation
	constexpr int kTracks = 4;
	struct TrackQueues {
		SpscQueue<EncodedAccessUnit, 8> au;
		SpscQueue<TrackControlCommand, 8> ctrl;
		SpscQueue<NetworkToSourceCommand, 8> net;
	};
	TrackQueues tracks[kTracks];

	// Each track pushes to its own AU queue
	for (int t = 0; t < kTracks; ++t) {
		EncodedAccessUnit au;
		au.trackIndex = t;
		au.ssrc = 11111 + t * 1111;
		au.assign((const uint8_t*)"data", 4);
		EXPECT_TRUE(tracks[t].au.tryPush(std::move(au)));
	}

	// Verify each queue only has its own track's data
	for (int t = 0; t < kTracks; ++t) {
		EncodedAccessUnit au;
		EXPECT_TRUE(tracks[t].au.tryPop(au));
		EXPECT_EQ(au.trackIndex, (uint32_t)t);
		EXPECT_EQ(au.ssrc, (uint32_t)(11111 + t * 1111));
		EXPECT_TRUE(tracks[t].au.empty());
	}
}

TEST(ThreadedControlHelpers, QueueFullDoesNotCreatePendingCommand) {
	SpscQueue<TrackControlCommand, kControlCommandQueueCapacity> q;
	uint64_t nextCommandId = 1;
	PendingTrackCommand pending;

	// Fill queue to capacity.
	for (size_t i = 0; i < kControlCommandQueueCapacity; ++i) {
		TrackControlCommand prefill;
		prefill.type = TrackCommandType::PauseTrack;
		ASSERT_TRUE(q.tryPush(std::move(prefill)));
	}

	qos::PlannedAction action;
	action.type = qos::ActionType::SetEncodingParameters;
	action.level = 1;
	action.encodingParameters = qos::EncodingParameters{300000, 15, 2.0, {}};

	EXPECT_FALSE(enqueueTrackAction(action, 0, nextCommandId, q, pending, 0));
	EXPECT_FALSE(pending.pending);
	EXPECT_EQ(pending.commandId, 0u);
}

TEST(ThreadedControlHelpers, ApplyCommandAckUpdatesStateOnApplied) {
	qos::PublisherQosController::Options opts;
	opts.source = qos::Source::Camera;
	opts.trackId = "video";
	opts.initialLevel = 0;
	opts.actionSink = [](const qos::PlannedAction&) { return false; };
	qos::PublisherQosController ctrl(opts);

	ThreadedTrackControlState trackState{500000, 25, 1.0, false, 0, &ctrl};
	ThreadedTrackStatsState statsState;
	PendingTrackCommand pending;
	pending.commandId = 5;
	pending.pending = true;
	pending.action.type = qos::ActionType::EnterAudioOnly;
	pending.action.level = 4;

	CommandAck ack;
	ack.commandId = 5;
	ack.type = TrackCommandType::PauseTrack;
	ack.applied = true;

	EXPECT_TRUE(applyCommandAck(ack, pending, trackState, statsState));
	EXPECT_TRUE(trackState.videoSuppressed);
	EXPECT_FALSE(pending.pending);
}

TEST(ThreadedControlHelpers, ApplyCommandAckIgnoresMismatchedCommandId) {
	ThreadedTrackControlState trackState;
	ThreadedTrackStatsState statsState;
	PendingTrackCommand pending;
	pending.commandId = 7;
	pending.pending = true;

	CommandAck ack;
	ack.commandId = 3;
	ack.type = TrackCommandType::SetEncodingParameters;
	ack.applied = true;

	EXPECT_FALSE(applyCommandAck(ack, pending, trackState, statsState));
	EXPECT_TRUE(pending.pending);
}

TEST(ThreadedControlHelpers, DuplicateAckIgnoredAfterPendingCleared) {
	ThreadedTrackControlState trackState;
	ThreadedTrackStatsState statsState;
	PendingTrackCommand pending;
	pending.commandId = 11;
	pending.pending = false; // already consumed

	CommandAck ack;
	ack.commandId = 11;
	ack.type = TrackCommandType::PauseTrack;
	ack.applied = true;

	EXPECT_FALSE(applyCommandAck(ack, pending, trackState, statsState));
	EXPECT_FALSE(trackState.videoSuppressed);
}

TEST(ThreadedControlHelpers, EnqueueTrackActionPropagatesConfigGeneration) {
	SpscQueue<TrackControlCommand, kControlCommandQueueCapacity> q;
	uint64_t nextCommandId = 10;
	PendingTrackCommand pending;

	qos::PlannedAction action;
	action.type = qos::ActionType::SetEncodingParameters;
	action.level = 2;
	action.encodingParameters = qos::EncodingParameters{450000, 12, 1.5, {}};

	ASSERT_TRUE(enqueueTrackAction(action, 3, nextCommandId, q, pending, 7));

	TrackControlCommand out;
	ASSERT_TRUE(q.tryPop(out));
	EXPECT_EQ(out.trackIndex, 3u);
	EXPECT_EQ(out.commandId, 10u);
	EXPECT_EQ(out.configGeneration, 7u);
}

TEST(ThreadedControlHelpers, ApplyCommandAckIgnoresOlderGeneration) {
	ThreadedTrackControlState trackState{500000, 25, 1.0, false, 3, nullptr};
	ThreadedTrackStatsState statsState;
	PendingTrackCommand pending;
	pending.commandId = 20;
	pending.pending = true;

	CommandAck ack;
	ack.commandId = 20;
	ack.type = TrackCommandType::SetEncodingParameters;
	ack.applied = true;
	ack.configGeneration = 2; // stale generation

	EXPECT_FALSE(applyCommandAck(ack, pending, trackState, statsState));
	EXPECT_TRUE(pending.pending);
	EXPECT_EQ(trackState.configGeneration, 3u);
}

TEST(ThreadedControlHelpers, OlderAckIgnoredAfterPendingCommandReplaced) {
	ThreadedTrackControlState trackState{500000, 25, 1.0, false, 0, nullptr};
	ThreadedTrackStatsState statsState;
	PendingTrackCommand pending;

	// First command would have been commandId=20, but pending gets replaced by commandId=21.
	pending.commandId = 21;
	pending.pending = true;
	pending.action.type = qos::ActionType::SetEncodingParameters;
	pending.action.level = 1;

	CommandAck stale;
	stale.commandId = 20;
	stale.type = TrackCommandType::SetEncodingParameters;
	stale.applied = true;
	stale.configGeneration = 0;
	stale.actualBitrateBps = 250000;

	EXPECT_FALSE(applyCommandAck(stale, pending, trackState, statsState));
	EXPECT_TRUE(pending.pending);
	EXPECT_EQ(trackState.encBitrate, 500000);

	CommandAck current;
	current.commandId = 21;
	current.type = TrackCommandType::SetEncodingParameters;
	current.applied = true;
	current.configGeneration = 0;
	current.actualBitrateBps = 300000;
	current.actualFps = 15;
	current.actualScale = 2.0;

	EXPECT_TRUE(applyCommandAck(current, pending, trackState, statsState));
	EXPECT_FALSE(pending.pending);
	EXPECT_EQ(trackState.encBitrate, 300000);
	EXPECT_EQ(trackState.configuredVideoFps, 15);
	EXPECT_DOUBLE_EQ(trackState.scaleResolutionDownBy, 2.0);
}

TEST(ThreadedControlHelpers, ShouldSampleTrackRequiresFreshOrServerStats) {
	EXPECT_TRUE(shouldSampleTrack(true, false));
	EXPECT_TRUE(shouldSampleTrack(false, true));
	EXPECT_FALSE(shouldSampleTrack(false, false));
}

// ═══════════════════════════════════════════════════════════
// RtcpHandler unit tests
// ═══════════════════════════════════════════════════════════

#include "../client/RtcpHandler.h"

TEST(RtcpHandler, BuildSenderReportProducesValidPacket) {
	uint8_t buf[64];
	size_t len = buildSenderReport(buf, 0x12345678, 90000, 100, 50000);
	EXPECT_EQ(len, 28u);
	// V=2, RC=0
	EXPECT_EQ(buf[0], 0x80);
	// PT=200 (SR)
	EXPECT_EQ(buf[1], 200);
	// SSRC
	EXPECT_EQ(buf[4], 0x12); EXPECT_EQ(buf[5], 0x34);
	EXPECT_EQ(buf[6], 0x56); EXPECT_EQ(buf[7], 0x78);
	// RTP timestamp = 90000 = 0x00015F90
	EXPECT_EQ(buf[16], 0x00); EXPECT_EQ(buf[17], 0x01);
	EXPECT_EQ(buf[18], 0x5F); EXPECT_EQ(buf[19], 0x90);
	// Packet count = 100 = 0x00000064
	EXPECT_EQ(buf[23], 0x64);
	// Octet count = 50000 = 0x0000C350
	EXPECT_EQ(buf[24], 0x00); EXPECT_EQ(buf[25], 0x00);
	EXPECT_EQ(buf[26], 0xC3); EXPECT_EQ(buf[27], 0x50);
}

TEST(RtcpHandler, NackRetransmitsStoredPackets) {
	RtpPacketStore store;
	// Store packets seq=100..105
	for (uint16_t seq = 100; seq <= 105; ++seq) {
		uint8_t pkt[20];
		memset(pkt, 0, sizeof(pkt));
		pkt[0] = 0x80; pkt[1] = 96;
		pkt[2] = seq >> 8; pkt[3] = seq & 0xFF;
		store.store(seq, pkt, sizeof(pkt));
	}

	// Build NACK for seq=101 + BLP=0x0005 (bits 0,2 → seq 102, 104)
	// RTPFB header: 12 bytes + 4 bytes NACK item
	uint8_t nack[16];
	nack[0] = 0x81; nack[1] = 205; // RTPFB, fmt=1
	nack[2] = 0; nack[3] = 3; // length=3 words
	memset(nack + 4, 0, 8); // sender + media SSRC
	// PID=101, BLP=0x0005
	nack[12] = 0; nack[13] = 101;
	nack[14] = 0x00; nack[15] = 0x05;

	// Use a socketpair to capture retransmitted packets
	int sv[2];
	ASSERT_EQ(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	int retransmitted = handleNack(nack, sizeof(nack), store, sv[0]);
	// Should retransmit: 101, 102 (bit 0 = 101+1+0=102), 104 (bit 2 = 101+1+2=104)
	EXPECT_EQ(retransmitted, 3);

	// Verify we can read 3 packets from the socket
	uint8_t buf[64];
	for (int i = 0; i < 3; ++i) {
		int n = recv(sv[1], buf, sizeof(buf), MSG_DONTWAIT);
		EXPECT_EQ(n, 20);
	}
	// No more
	EXPECT_EQ(recv(sv[1], buf, sizeof(buf), MSG_DONTWAIT), -1);

	close(sv[0]); close(sv[1]);
}

TEST(RtcpHandler, PliRequestsFreshKeyframeCallbackWhenAvailable) {
	int sv[2];
	ASSERT_EQ(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	RtcpContext rtcp;
	uint32_t ssrc = 0x12345678;
	uint16_t seq = 3456;
	int requestCount = 0;
	int resendCount = 0;

	rtcp.registerVideoStream(ssrc, 96, &seq);
	rtcp.requestKeyframeFn = [&](uint32_t requestedSsrc) {
		++requestCount;
		EXPECT_EQ(requestedSsrc, ssrc);
	};
	rtcp.sendH264Fn = [&](int, const uint8_t*, int, uint8_t, uint32_t, uint32_t, uint16_t&) {
		++resendCount;
	};

	uint8_t pli[12]{};
	pli[0] = 0x81;
	pli[1] = RTCP_PT_PSFB;
	pli[2] = 0x00;
	pli[3] = 0x02;
	pli[8] = static_cast<uint8_t>(ssrc >> 24);
	pli[9] = static_cast<uint8_t>(ssrc >> 16);
	pli[10] = static_cast<uint8_t>(ssrc >> 8);
	pli[11] = static_cast<uint8_t>(ssrc & 0xFF);
	ASSERT_EQ(::send(sv[1], pli, sizeof(pli), 0), static_cast<ssize_t>(sizeof(pli)));

	rtcp.processIncomingRtcp(sv[0]);

	EXPECT_EQ(requestCount, 1);
	EXPECT_EQ(resendCount, 0);
	EXPECT_EQ(rtcp.pliResponded, 1);

	close(sv[0]);
	close(sv[1]);
}

TEST(RtcpHandler, PliFallbackUsesLatestRtpTimestampForCachedKeyframe) {
	int sv[2];
	ASSERT_EQ(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	RtcpContext rtcp;
	uint32_t ssrc = 0x87654321;
	uint16_t seq = 2222;
	uint32_t resentTs = 0;

	rtcp.registerVideoStream(ssrc, 97, &seq);
	uint8_t keyframe[] = {0x00, 0x00, 0x01, 0x65, 0xAA};
	rtcp.cacheKeyframe(ssrc, keyframe, sizeof(keyframe), 90000);

	uint8_t rtp[16]{};
	rtp[0] = 0x80;
	rtp[1] = 97;
	rtp[2] = static_cast<uint8_t>(seq >> 8);
	rtp[3] = static_cast<uint8_t>(seq & 0xFF);
	rtp[4] = 0x00;
	rtp[5] = 0x02;
	rtp[6] = 0xBF;
	rtp[7] = 0x20; // 180000
	rtp[8] = static_cast<uint8_t>(ssrc >> 24);
	rtp[9] = static_cast<uint8_t>(ssrc >> 16);
	rtp[10] = static_cast<uint8_t>(ssrc >> 8);
	rtp[11] = static_cast<uint8_t>(ssrc & 0xFF);
	rtcp.onVideoRtpSent(rtp, sizeof(rtp));
	rtcp.sendH264Fn = [&](int, const uint8_t*, int, uint8_t, uint32_t ts, uint32_t, uint16_t&) {
		resentTs = ts;
	};

	uint8_t pli[12]{};
	pli[0] = 0x81;
	pli[1] = RTCP_PT_PSFB;
	pli[2] = 0x00;
	pli[3] = 0x02;
	pli[8] = static_cast<uint8_t>(ssrc >> 24);
	pli[9] = static_cast<uint8_t>(ssrc >> 16);
	pli[10] = static_cast<uint8_t>(ssrc >> 8);
	pli[11] = static_cast<uint8_t>(ssrc & 0xFF);
	ASSERT_EQ(::send(sv[1], pli, sizeof(pli), 0), static_cast<ssize_t>(sizeof(pli)));

	rtcp.processIncomingRtcp(sv[0]);

	EXPECT_EQ(resentTs, 180000u);

	close(sv[0]);
	close(sv[1]);
}

TEST(RtcpHandler, ReceiverReportClampsNegativeCumulativeLostToZero) {
	int sv[2];
	ASSERT_EQ(socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	RtcpContext rtcp;
	uint32_t ssrc = 0x01020304;
	uint16_t seq = 100;
	rtcp.registerVideoStream(ssrc, 96, &seq);

	uint8_t rr[32]{};
	rr[0] = 0x81;
	rr[1] = RTCP_PT_RR;
	rr[2] = 0x00;
	rr[3] = 0x07;
	rr[8] = static_cast<uint8_t>(ssrc >> 24);
	rr[9] = static_cast<uint8_t>(ssrc >> 16);
	rr[10] = static_cast<uint8_t>(ssrc >> 8);
	rr[11] = static_cast<uint8_t>(ssrc & 0xFF);
	rr[13] = 0x80;
	rr[14] = 0x00;
	rr[15] = 0x01; // mediasoup worker signed 24-bit encoding for -1
	ASSERT_EQ(::send(sv[1], rr, sizeof(rr), 0), static_cast<ssize_t>(sizeof(rr)));

	rtcp.processIncomingRtcp(sv[0]);

	const auto* stream = rtcp.getVideoStream(ssrc);
	ASSERT_NE(stream, nullptr);
	EXPECT_EQ(stream->rrCumulativeLost, 0u);

	close(sv[0]);
	close(sv[1]);
}

TEST(RtcpHandler, RtpPacketStoreEvictsOldEntries) {
	RtpPacketStore store;
	uint8_t pkt[20] = {};
	// Fill beyond capacity
	for (uint16_t i = 0; i < RtpPacketStore::kMaxPackets + 100; ++i) {
		pkt[2] = i >> 8; pkt[3] = i & 0xFF;
		store.store(i, pkt, sizeof(pkt));
	}
	// Oldest entries should be evicted
	const uint8_t* data; size_t len;
	EXPECT_FALSE(store.get(0, data, len)) << "seq 0 should be evicted";
	EXPECT_TRUE(store.get(RtpPacketStore::kMaxPackets + 99, data, len)) << "newest should exist";
}

// ═══════════════════════════════════════════════════════════
// SpscQueue wraparound
// ═══════════════════════════════════════════════════════════

TEST(SpscQueue, Wraparound) {
	SpscQueue<int, 4> q;
	// Fill to capacity
	for (int i = 0; i < 4; ++i) { int v = i; EXPECT_TRUE(q.tryPush(std::move(v))); }
	EXPECT_FALSE(q.tryPush(std::move(*(new int(99))))); // full

	// Drain all
	for (int i = 0; i < 4; ++i) {
		int v = -1;
		EXPECT_TRUE(q.tryPop(v));
		EXPECT_EQ(v, i);
	}
	EXPECT_TRUE(q.empty());

	// Fill again — this exercises head/tail wraparound
	for (int i = 10; i < 14; ++i) { int v = i; EXPECT_TRUE(q.tryPush(std::move(v))); }
	for (int i = 10; i < 14; ++i) {
		int v = -1;
		EXPECT_TRUE(q.tryPop(v));
		EXPECT_EQ(v, i);
	}
	EXPECT_TRUE(q.empty());

	// Partial fill/drain cycles to stress wraparound
	for (int cycle = 0; cycle < 100; ++cycle) {
		int v = cycle;
		EXPECT_TRUE(q.tryPush(std::move(v)));
		int out = -1;
		EXPECT_TRUE(q.tryPop(out));
		EXPECT_EQ(out, cycle);
	}
}
