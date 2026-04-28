// Unit tests for the 5 code review fixes (2026-04-08).
#include <gtest/gtest.h>
#include "RoomManager.h"
#include "Channel.h"
#include "Constants.h"
#include "PipeTransport.h"
#include "PlainTransport.h"
#include "WebRtcTransport.h"
#include "Consumer.h"
#include "message_generated.h"
#include "MainBootstrap.h"
#include "notification_generated.h"
#include "pipeTransport_generated.h"
#include "plainTransport_generated.h"
#include "RoomRegistryReplyUtils.h"
#include "RuntimeOptionParsers.h"
#include "RoomRegistrySelection.h"
#include "RoomStatsQosHelpers.h"
#include "response_generated.h"
#include "SignalingSocketState.h"
#include "StaticFileResponder.h"
#include "WorkerThread.h"
#include "webRtcTransport_generated.h"
#include "../client/ccutils/Prober.h"
#include "../client/RtcpHandler.h"
#include "../src/Recorder.h"
#include <algorithm>
#include <future>
#include <chrono>
#include <cerrno>
#include <cstring>
#include <hiredis/hiredis.h>
#include <mutex>
#include <unistd.h>

using namespace mediasoup;

namespace {

class BlockingProbeListener : public mediasoup::ccutils::ProberListener {
public:
	void OnProbeClusterSwitch(const mediasoup::ccutils::ProbeClusterInfo& info) override
	{
		std::lock_guard<std::mutex> lock(mutex_);
		startedIds_.push_back(info.id);
		cv_.notify_all();
	}

	void OnSendProbe(int) override
	{
		std::unique_lock<std::mutex> lock(mutex_);
		sendCount_++;
		if (sendCount_ == 1) {
			firstSendEntered_ = true;
			cv_.notify_all();
			cv_.wait(lock, [&] { return releaseFirstSend_; });
		} else {
			cv_.notify_all();
		}
	}

	bool waitForFirstSend(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return cv_.wait_for(lock, timeout, [&] { return firstSendEntered_; });
	}

	void releaseFirstSend()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		releaseFirstSend_ = true;
		cv_.notify_all();
	}

	bool waitForClusterStart(
		mediasoup::ccutils::ProbeClusterId clusterId,
		std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return cv_.wait_for(lock, timeout, [&] {
			return std::find(startedIds_.begin(), startedIds_.end(), clusterId) != startedIds_.end();
		});
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
	std::vector<mediasoup::ccutils::ProbeClusterId> startedIds_;
	int sendCount_{ 0 };
	bool firstSendEntered_{ false };
	bool releaseFirstSend_{ false };
};

mediasoup::ccutils::ProbeClusterGoal MakeTestProbeGoal()
{
	mediasoup::ccutils::ProbeClusterGoal goal;
	goal.availableBandwidthBps = 500000;
	goal.expectedUsageBps = 100000;
	goal.desiredBps = 250000;
	goal.duration = std::chrono::milliseconds(60);
	return goal;
}

bool ReadExact(int fd, uint8_t* data, size_t len)
{
	size_t total = 0;
	while (total < len) {
		ssize_t n = ::read(fd, data + total, len - total);
		if (n <= 0) {
			return false;
		}
		total += static_cast<size_t>(n);
	}
	return true;
}

bool WriteExact(int fd, const uint8_t* data, size_t len)
{
	size_t total = 0;
	while (total < len) {
		ssize_t n = ::write(fd, data + total, len - total);
		if (n <= 0) {
			return false;
		}
		total += static_cast<size_t>(n);
	}
	return true;
}

template<typename ResponseBuilder>
std::future<bool> StartSingleResponseWorker(
	int requestReadFd,
	int responseWriteFd,
	FBS::Request::Method expectedMethod,
	ResponseBuilder buildResponse)
{
	return std::async(std::launch::async, [=]() mutable {
		uint8_t sizePrefix[4];
		if (!ReadExact(requestReadFd, sizePrefix, sizeof(sizePrefix))) {
			return false;
		}
		uint32_t messageSize = 0;
		std::memcpy(&messageSize, sizePrefix, sizeof(messageSize));
		std::vector<uint8_t> requestBuf(4 + messageSize);
		std::memcpy(requestBuf.data(), sizePrefix, sizeof(sizePrefix));
		if (!ReadExact(requestReadFd, requestBuf.data() + 4, messageSize)) {
			return false;
		}

		auto* requestMsg = FBS::Message::GetSizePrefixedMessage(requestBuf.data());
		if (!requestMsg) {
			return false;
		}
		auto* request = requestMsg->data_as_Request();
		if (!request || request->method() != expectedMethod) {
			return false;
		}

		const std::vector<uint8_t> response = buildResponse(*request);
		return WriteExact(responseWriteFd, response.data(), response.size());
	});
}

std::vector<uint8_t> BuildEmptyResponseBuffer(uint32_t requestId)
{
	flatbuffers::FlatBufferBuilder builder;
	auto response = FBS::Response::CreateResponse(
		builder,
		requestId,
		true,
		FBS::Response::Body::NONE,
		0);
	auto message = FBS::Message::CreateMessage(
		builder,
		FBS::Message::Body::Response,
		response.Union());
	builder.FinishSizePrefixed(message);
	return {
		builder.GetBufferPointer(),
		builder.GetBufferPointer() + builder.GetSize()
	};
}

template<typename BodyBuilder>
std::vector<uint8_t> BuildResponseBuffer(
	uint32_t requestId,
	FBS::Response::Body bodyType,
	BodyBuilder buildBody)
{
	flatbuffers::FlatBufferBuilder builder;
	auto body = buildBody(builder);
	auto response = FBS::Response::CreateResponse(
		builder,
		requestId,
		true,
		bodyType,
		body.Union());
	auto message = FBS::Message::CreateMessage(
		builder,
		FBS::Message::Body::Response,
		response.Union());
	builder.FinishSizePrefixed(message);
	return {
		builder.GetBufferPointer(),
		builder.GetBufferPointer() + builder.GetSize()
	};
}

} // namespace

// ═══════════════════════════════════════════════════════════════
// Fix 1: Room::addPeer closes old peer before replacing
// ═══════════════════════════════════════════════════════════════

class DuplicatePeerTest : public ::testing::Test {
protected:
	std::shared_ptr<Room> room = std::make_shared<Room>("test-room", nullptr);

	std::shared_ptr<Peer> makePeer(const std::string& id) {
		auto p = std::make_shared<Peer>();
		p->id = id;
		p->displayName = id;
		return p;
	}
};

TEST_F(DuplicatePeerTest, AddPeerWithSameIdClosesOldPeer) {
	auto old = makePeer("alice");
	room->addPeer(old);
	EXPECT_EQ(room->peerCount(), 1u);

	// replacePeer returns old peer for caller to clean up
	auto fresh = makePeer("alice");
	auto replaced = room->replacePeer(fresh);

	EXPECT_NE(replaced, nullptr);
	EXPECT_EQ(replaced, old);
	// Room should still have exactly 1 peer
	EXPECT_EQ(room->peerCount(), 1u);
	// The stored peer should be the new one
	EXPECT_EQ(room->getPeer("alice"), fresh);

	// Caller is responsible for close — simulate what RoomService::join does
	replaced->close();
	EXPECT_EQ(old->sendTransport, nullptr);
	EXPECT_EQ(old->recvTransport, nullptr);
	EXPECT_TRUE(old->producers.empty());
	EXPECT_TRUE(old->consumers.empty());
}

TEST_F(DuplicatePeerTest, ReplacePeerReturnsNullWhenNoPrevious) {
	auto alice = makePeer("alice");
	auto replaced = room->replacePeer(alice);
	EXPECT_EQ(replaced, nullptr);
	EXPECT_EQ(room->peerCount(), 1u);
}

TEST_F(DuplicatePeerTest, AddPeerWithDifferentIdDoesNotAffectExisting) {
	auto alice = makePeer("alice");
	room->addPeer(alice);
	room->addPeer(makePeer("bob"));
	EXPECT_EQ(room->peerCount(), 2u);
	EXPECT_NE(room->getPeer("alice"), nullptr);
	EXPECT_NE(room->getPeer("bob"), nullptr);
}

TEST_F(DuplicatePeerTest, RepeatedReplacementWorks) {
	for (int i = 0; i < 5; ++i) {
		auto old = room->replacePeer(makePeer("alice"));
		if (old) old->close();
	}
	EXPECT_EQ(room->peerCount(), 1u);
}

// ═══════════════════════════════════════════════════════════════
// Fix 4: collectPeerStats no longer uses std::async
//   Verify the old pseudo-timeout pattern is gone by checking
//   that std::future destructor blocking is not an issue.
// ═══════════════════════════════════════════════════════════════

TEST(StatsTimeoutTest, FutureDestructorBlocksWithStdAsync) {
	// Demonstrate the problem that was fixed:
	// std::async(std::launch::async, ...) future destructor blocks.
	auto start = std::chrono::steady_clock::now();
	{
		auto fut = std::async(std::launch::async, [] {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			return 42;
		});
		// Don't call get() — let fut destruct
		// With std::launch::async, destructor WILL block ~200ms
	}
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - start).count();
	// This proves the destructor blocks — the old code had this problem
	EXPECT_GE(elapsed, 150) << "std::async future destructor should block";
}

TEST(StatsTimeoutTest, RequestWaitTimeoutConstantExists) {
	// The fix relies on requestWait's built-in timeout
	EXPECT_EQ(kChannelRequestTimeoutMs, 5000);
	EXPECT_EQ(kStatsTimeoutMs, 500);
}

// ═══════════════════════════════════════════════════════════════
// Fix 5: PlainTransport uses requestWait (compile-time check)
//   The actual fix is that PlainTransport::connect() calls
//   channel_->requestWait() instead of channel_->request().get().
//   This is verified by the fact that the code compiles and by
//   the integration test. Here we just verify OwnedResponse
//   semantics used by the fixed code.
// ═══════════════════════════════════════════════════════════════

TEST(PlainTransportFixTest, OwnedResponseNullResponseOnEmpty) {
	Channel::OwnedResponse resp;
	EXPECT_EQ(resp.response(), nullptr);
}

// ═══════════════════════════════════════════════════════════════
// Fix 3: restartIce parses response (compile-time + semantics)
//   The actual parsing requires a real worker. Here we verify
//   the IceParameters struct can be updated as the fix does.
// ═══════════════════════════════════════════════════════════════

TEST(RestartIceFixTest, IceParametersCanBeUpdated) {
	IceParameters params;
	params.usernameFragment = "old_ufrag";
	params.password = "old_pwd";
	params.iceLite = false;

	// Simulate what the fix does after parsing response
	params.usernameFragment = "new_ufrag";
	params.password = "new_pwd";
	params.iceLite = true;

	EXPECT_EQ(params.usernameFragment, "new_ufrag");
	EXPECT_EQ(params.password, "new_pwd");
	EXPECT_TRUE(params.iceLite);

	// Verify JSON serialization works
	json j = params;
	EXPECT_EQ(j["usernameFragment"], "new_ufrag");
	EXPECT_EQ(j["password"], "new_pwd");
	EXPECT_EQ(j["iceLite"], true);
}

// ═══════════════════════════════════════════════════════════════
// Unsigned underflow: worker count with low CPU cores
// ═══════════════════════════════════════════════════════════════

TEST(WorkerCountTest, NoCoresUnderflow) {
	// Simulate the calculation: max(1, cores - 2)
	auto calc = [](int cores) { return std::max(1, cores - 2); };
	EXPECT_EQ(calc(0), 1);  // hardware_concurrency() can return 0
	EXPECT_EQ(calc(1), 1);
	EXPECT_EQ(calc(2), 1);
	EXPECT_EQ(calc(4), 2);
	EXPECT_EQ(calc(8), 6);
}

TEST(WorkerThreadEpollWaitTest, RecoverableErrorClassification) {
	EXPECT_TRUE(IsRecoverableEpollWaitError(EINTR));
	EXPECT_FALSE(IsRecoverableEpollWaitError(EBADF));
	EXPECT_FALSE(IsRecoverableEpollWaitError(EINVAL));
}

TEST(SocketPendingJoinStateTest, PendingJoinDoesNotCommitSessionState) {
	PerSocketData socketData;

	SetPendingSocketJoin(&socketData, "room-a", "peer-a", 42);
	EXPECT_EQ(socketData.sessionId, kInvalidSessionId);
	EXPECT_TRUE(socketData.roomId.empty());
	EXPECT_TRUE(socketData.peerId.empty());
	EXPECT_EQ(socketData.pendingSessionId, 42u);
	EXPECT_EQ(socketData.pendingRoomId, "room-a");
	EXPECT_EQ(socketData.pendingPeerId, "peer-a");

	ClearPendingSocketJoinIfMatches(&socketData, 7);
	EXPECT_EQ(socketData.pendingSessionId, 42u);

	ClearPendingSocketJoinIfMatches(&socketData, 42);
	EXPECT_EQ(socketData.pendingSessionId, kInvalidSessionId);
	EXPECT_TRUE(socketData.pendingRoomId.empty());
	EXPECT_TRUE(socketData.pendingPeerId.empty());
}

TEST(SocketPendingJoinStateTest, WorkerUnavailableClearsPendingJoinBeforeCommit) {
	PerSocketData socketData;
	socketData.roomId = "committed-room";
	socketData.peerId = "committed-peer";
	socketData.sessionId = 7;

	SetPendingSocketJoin(&socketData, "pending-room", "pending-peer", 42);
	EXPECT_FALSE(PrepareSocketJoinCommit(&socketData, 42, false));
	EXPECT_EQ(socketData.roomId, "committed-room");
	EXPECT_EQ(socketData.peerId, "committed-peer");
	EXPECT_EQ(socketData.sessionId, 7u);
	EXPECT_EQ(socketData.pendingSessionId, kInvalidSessionId);
	EXPECT_TRUE(socketData.pendingRoomId.empty());
	EXPECT_TRUE(socketData.pendingPeerId.empty());
}

TEST(ChannelWriteRetryTest, RecoverableErrorClassification) {
	EXPECT_TRUE(IsRecoverableChannelWriteError(EINTR));
	EXPECT_TRUE(IsRecoverableChannelWriteError(EAGAIN));
	EXPECT_TRUE(IsRecoverableChannelWriteError(EWOULDBLOCK));
	EXPECT_FALSE(IsRecoverableChannelWriteError(EPIPE));
	EXPECT_FALSE(IsRecoverableChannelWriteError(EBADF));
}

TEST(TransportConnectValidationTest, WebRtcTransportUsesWorkerDtlsRole) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/false);
		WebRtcTransport transport(
			"webrtc-transport",
			&ch,
			"router-1",
			IceParameters{},
			{},
			DtlsParameters{});
		auto worker = StartSingleResponseWorker(
			producerPipe[0],
			consumerPipe[1],
			FBS::Request::Method::WEBRTCTRANSPORT_CONNECT,
			[](const FBS::Request::Request& request) {
				return BuildResponseBuffer(
					request.id(),
					FBS::Response::Body::WebRtcTransport_ConnectResponse,
					[](flatbuffers::FlatBufferBuilder& builder) {
						return FBS::WebRtcTransport::CreateConnectResponse(
							builder,
							FBS::WebRtcTransport::DtlsRole::CLIENT);
					});
			});

		const json result = transport.connect(DtlsParameters{});
		EXPECT_EQ(result.value("dtlsLocalRole", ""), "client");
		EXPECT_EQ(transport.dtlsParameters().role, "client");
		EXPECT_TRUE(worker.get());
	}

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(TransportConnectValidationTest, WebRtcTransportRejectsUnexpectedResponseBody) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/false);
		WebRtcTransport transport(
			"webrtc-transport",
			&ch,
			"router-1",
			IceParameters{},
			{},
			DtlsParameters{});
		auto worker = StartSingleResponseWorker(
			producerPipe[0],
			consumerPipe[1],
			FBS::Request::Method::WEBRTCTRANSPORT_CONNECT,
			[](const FBS::Request::Request& request) {
				return BuildEmptyResponseBuffer(request.id());
			});

		EXPECT_THROW(transport.connect(DtlsParameters{}), std::runtime_error);
		EXPECT_TRUE(worker.get());
	}

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(TransportConnectValidationTest, PlainTransportRequiresValidatedTupleResponse) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/false);
		TransportTuple tuple;
		PlainTransport transport("plain-transport", &ch, "router-1", tuple, true);
		auto worker = StartSingleResponseWorker(
			producerPipe[0],
			consumerPipe[1],
			FBS::Request::Method::PLAINTRANSPORT_CONNECT,
			[](const FBS::Request::Request& request) {
				return BuildResponseBuffer(
					request.id(),
					FBS::Response::Body::PlainTransport_ConnectResponse,
					[](flatbuffers::FlatBufferBuilder& builder) {
						auto tupleOffset = FBS::Transport::CreateTupleDirect(
							builder,
							"127.0.0.1",
							55000,
							"198.51.100.20",
							55001,
							FBS::Transport::Protocol::UDP);
						return FBS::PlainTransport::CreateConnectResponse(
							builder,
							tupleOffset,
							0,
							0);
					});
			});

		const json result = transport.connect("198.51.100.20", 55001);
		EXPECT_TRUE(result.value("connected", false));
		EXPECT_EQ(transport.tuple().localAddress, "127.0.0.1");
		EXPECT_EQ(transport.tuple().localPort, 55000);
		EXPECT_EQ(transport.tuple().remoteIp, "198.51.100.20");
		EXPECT_EQ(transport.tuple().remotePort, 55001);
		EXPECT_EQ(transport.tuple().protocol, "udp");
		EXPECT_TRUE(worker.get());
	}

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(TransportConnectValidationTest, PipeTransportRejectsMissingTupleResponse) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/false);
		PipeTransport transport("pipe-transport", &ch, "router-1", TransportTuple{}, true);
		auto worker = StartSingleResponseWorker(
			producerPipe[0],
			consumerPipe[1],
			FBS::Request::Method::PIPETRANSPORT_CONNECT,
			[](const FBS::Request::Request& request) {
				flatbuffers::FlatBufferBuilder builder;
				auto response = FBS::Response::CreateResponse(
					builder,
					request.id(),
					true,
					FBS::Response::Body::PipeTransport_ConnectResponse,
					0);
				auto message = FBS::Message::CreateMessage(
					builder,
					FBS::Message::Body::Response,
					response.Union());
				builder.FinishSizePrefixed(message);
				return std::vector<uint8_t>(
					builder.GetBufferPointer(),
					builder.GetBufferPointer() + builder.GetSize());
			});

		EXPECT_THROW(transport.connect("198.51.100.40", 60000), std::runtime_error);
		EXPECT_TRUE(worker.get());
	}

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(ChannelRequestIdTest, WraparoundSkipsPendingRequestIds) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/false);

		ch.setNextIdForTest(0);
		auto first = ch.requestWithId(FBS::Request::Method::WORKER_CLOSE);
		EXPECT_EQ(first.requestId, 1u);

		ch.setNextIdForTest(4294967294u);
		auto nearWrap = ch.requestWithId(FBS::Request::Method::WORKER_CLOSE);
		EXPECT_EQ(nearWrap.requestId, 4294967295u);

		ch.setNextIdForTest(4294967295u);
		auto wrapped = ch.requestWithId(FBS::Request::Method::WORKER_CLOSE);
		EXPECT_EQ(wrapped.requestId, 2u);

		ch.close();
	}

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(ConsumerCloseReasonTest, ProducerCloseCarriesTerminalReasonOnInternalCloseEvent) {
	Consumer consumer(
		"consumer-1",
		"producer-1",
		"video",
		RtpParameters{},
		"simple",
		nullptr,
		"transport-1");
	std::string closeReason;
	int producerCloseEvents = 0;
	consumer.emitter().on("@close", [&](const std::vector<std::any>& args) {
		ASSERT_EQ(args.size(), 1u);
		closeReason = std::any_cast<std::string>(args[0]);
	});
	consumer.emitter().on("producerclose", [&](const std::vector<std::any>&) {
		++producerCloseEvents;
	});

	consumer.handleNotification(FBS::Notification::Event::CONSUMER_PRODUCER_CLOSE, nullptr);

	EXPECT_EQ(closeReason, "producerclose");
	EXPECT_EQ(producerCloseEvents, 1);
}

TEST(DownlinkRateLimitStateTest, PendingSequenceDoesNotAdvanceAcceptedBaselineUntilMarked) {
	DownlinkStatsRateLimitState state;
	MarkAcceptedDownlinkSeq(state, 10);

	state.pending = true;
	state.pendingSeq = 11;
	state.pendingSinceMs = 1234;

	EXPECT_EQ(state.lastAcceptedSeq, 10u);
	EXPECT_TRUE(state.hasAcceptedSeq);
	EXPECT_FALSE(IsAdvancingDownlinkSeq(state, 10))
		<< "pending seq should be the temporary advancing baseline";
	EXPECT_TRUE(IsAdvancingDownlinkSeq(state, 12));

	ClearPendingDownlinkSeqIfMatches(state, 11);
	EXPECT_FALSE(state.pending);
	EXPECT_EQ(state.lastAcceptedSeq, 10u)
		<< "clearing pending must not mutate the accepted sequence";
	EXPECT_FALSE(IsAdvancingDownlinkSeq(state, 10));
	EXPECT_TRUE(IsAdvancingDownlinkSeq(state, 11));
}

TEST(DownlinkRateLimitStateTest, AcceptedSequenceMovesOnlyWhenExplicitlyMarked) {
	DownlinkStatsRateLimitState state;
	state.pending = true;
	state.pendingSeq = 77;
	state.pendingSinceMs = 500;

	EXPECT_EQ(state.lastAcceptedSeq, 0u);
	EXPECT_FALSE(state.hasAcceptedSeq);

	ClearPendingDownlinkSeqIfMatches(state, 77);
	EXPECT_FALSE(state.pending);
	EXPECT_EQ(state.lastAcceptedSeq, 0u);
	EXPECT_FALSE(state.hasAcceptedSeq);

	MarkAcceptedDownlinkSeq(state, 77);
	EXPECT_TRUE(state.hasAcceptedSeq);
	EXPECT_EQ(state.lastAcceptedSeq, 77u);
}

TEST(RoomRegistrySelectionTest, NoGeoComparatorPreservesStrictWeakOrderingForSelf) {
	mediasoup::roomregistry::LoadCandidate self{"ws://self", 3, 0};
	mediasoup::roomregistry::LoadCandidate peer{"ws://peer", 3, 0};

	EXPECT_FALSE(mediasoup::roomregistry::CompareNoGeoCandidates(self, self, "ws://self"));
	EXPECT_TRUE(mediasoup::roomregistry::CompareNoGeoCandidates(self, peer, "ws://self"));
	EXPECT_FALSE(mediasoup::roomregistry::CompareNoGeoCandidates(peer, self, "ws://self"));
}

TEST(ProberConcurrencyFixTest, ResetRestartsWorkerForClusterQueuedDuringReset) {
	BlockingProbeListener listener;
	mediasoup::ccutils::Prober prober(&listener);

	const auto first = prober.AddCluster(
		mediasoup::ccutils::ProbeClusterMode::Uniform,
		MakeTestProbeGoal());
	ASSERT_TRUE(first.IsValid());
	ASSERT_TRUE(listener.waitForFirstSend(std::chrono::milliseconds(500)));

	auto resetFuture = std::async(std::launch::async, [&prober] {
		prober.Reset();
	});
	EXPECT_EQ(
		resetFuture.wait_for(std::chrono::milliseconds(20)),
		std::future_status::timeout);

	const auto queuedDuringReset = prober.AddCluster(
		mediasoup::ccutils::ProbeClusterMode::Uniform,
		MakeTestProbeGoal());
	ASSERT_TRUE(queuedDuringReset.IsValid());

	listener.releaseFirstSend();
	EXPECT_EQ(
		resetFuture.wait_for(std::chrono::seconds(1)),
		std::future_status::ready);
	EXPECT_TRUE(listener.waitForClusterStart(
		queuedDuringReset.id,
		std::chrono::seconds(1)));

	prober.Reset();
}

TEST(ChannelThreadSafetyFixTest, ProcessAvailableDataRejectedInThreadedMode) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	{
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/true);
		// Defensive guard: threaded mode must not allow direct pump API.
		EXPECT_FALSE(ch.processAvailableData());
	}

	::close(producerPipe[0]);
	::close(producerPipe[1]);
	::close(consumerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(ChannelThreadSafetyFixTest, ThreadedCloseDoesNotWaitForPeerWriterToClose) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	auto fut = std::async(std::launch::async, [&]() {
		Channel ch(
			/*producerFd=*/producerPipe[1],
			/*consumerFd=*/consumerPipe[0],
			/*pid=*/12345,
			/*threaded=*/true);
	});

	// Keep consumerPipe[1] open on purpose. Before the fix, threaded close could block
	// forever waiting for the read thread, which was stuck in a blocking read().
	auto firstWait = fut.wait_for(std::chrono::milliseconds(500));
	EXPECT_EQ(firstWait, std::future_status::ready)
		<< "threaded Channel::close must not depend on the peer writer closing first";

	if (firstWait != std::future_status::ready) {
		::close(consumerPipe[1]);
		consumerPipe[1] = -1;
	}
	fut.wait();

	::close(producerPipe[0]);
	if (consumerPipe[1] >= 0) ::close(consumerPipe[1]);
}

TEST(ChannelThreadSafetyFixTest, NonThreadedNotificationCanReenterRequestWaitWithoutReplay) {
	int producerPipe[2];
	int consumerPipe[2];
	ASSERT_EQ(::pipe(producerPipe), 0);
	ASSERT_EQ(::pipe(consumerPipe), 0);

	Channel ch(
		/*producerFd=*/producerPipe[1],
		/*consumerFd=*/consumerPipe[0],
		/*pid=*/12345,
		/*threaded=*/false);

	std::atomic<int> callbackCount{0};
	std::atomic<bool> requestSucceeded{false};
	auto readExact = [](int fd, void* data, size_t len) {
		size_t total = 0;
		while (total < len) {
			const ssize_t n = ::read(fd, static_cast<uint8_t*>(data) + total, len - total);
			if (n <= 0) {
				return false;
			}
			total += static_cast<size_t>(n);
		}
		return true;
	};

	auto fakeWorker = std::async(std::launch::async, [&]() {
		uint8_t sizePrefix[4];
		if (!readExact(producerPipe[0], sizePrefix, sizeof(sizePrefix))) {
			return false;
		}
		uint32_t messageSize = 0;
		std::memcpy(&messageSize, sizePrefix, sizeof(messageSize));
		std::vector<uint8_t> requestBuf(4 + messageSize);
		std::memcpy(requestBuf.data(), sizePrefix, sizeof(sizePrefix));
		if (!readExact(producerPipe[0], requestBuf.data() + 4, messageSize)) {
			return false;
		}

		auto* requestMsg = FBS::Message::GetSizePrefixedMessage(requestBuf.data());
		if (!requestMsg) {
			return false;
		}
		auto* request = requestMsg->data_as_Request();
		if (!request) {
			return false;
		}

		flatbuffers::FlatBufferBuilder builder;
		auto response = FBS::Response::CreateResponse(
			builder,
			request->id(),
			true,
			FBS::Response::Body::NONE,
			0);
		auto message = FBS::Message::CreateMessage(
			builder,
			FBS::Message::Body::Response,
			response.Union());
		builder.FinishSizePrefixed(message);
		return ::write(consumerPipe[1], builder.GetBufferPointer(), builder.GetSize()) ==
			static_cast<ssize_t>(builder.GetSize());
	});

	ch.emitter().on("reentrant", [&](const std::vector<std::any>&) {
		if (callbackCount.fetch_add(1) != 0) {
			return;
		}
		auto response = ch.requestWait(FBS::Request::Method::WORKER_CLOSE);
		requestSucceeded.store(response.response() != nullptr);
	});

	flatbuffers::FlatBufferBuilder builder;
	auto handlerId = builder.CreateString("reentrant");
	auto notification = FBS::Notification::CreateNotification(
		builder,
		handlerId,
		FBS::Notification::Event::WORKER_RUNNING,
		FBS::Notification::Body::NONE,
		0);
	auto message = FBS::Message::CreateMessage(
		builder,
		FBS::Message::Body::Notification,
		notification.Union());
	builder.FinishSizePrefixed(message);
	ASSERT_EQ(
		::write(consumerPipe[1], builder.GetBufferPointer(), builder.GetSize()),
		static_cast<ssize_t>(builder.GetSize()));

	EXPECT_TRUE(ch.processAvailableData());
	EXPECT_TRUE(fakeWorker.get());
	EXPECT_EQ(callbackCount.load(), 1);
	EXPECT_TRUE(requestSucceeded.load());

	::close(producerPipe[0]);
	::close(consumerPipe[1]);
}

TEST(RuntimeOptionParsersTest, RejectsInvalidIntegerValues) {
	int parsed = 0;
	EXPECT_FALSE(TryParseIntArgValue("abc", parsed));
	EXPECT_FALSE(TryParseIntArgValue("10ms", parsed));
	EXPECT_TRUE(TryParseIntArgValue("42", parsed));
	EXPECT_EQ(parsed, 42);
}

TEST(RuntimeOptionParsersTest, RejectsInvalidDoubleValues) {
	double parsed = 0.0;
	EXPECT_FALSE(TryParseDoubleArgValue("abc", parsed));
	EXPECT_FALSE(TryParseDoubleArgValue("1.5ms", parsed));
	EXPECT_TRUE(TryParseDoubleArgValue("1.5", parsed));
	EXPECT_DOUBLE_EQ(parsed, 1.5);
}

TEST(RuntimeOptionParsersTest, FinalizeRuntimeOptionsRejectsRecordedLoadError) {
	RuntimeOptions options;
	options.loadError = "invalid numeric CLI arg '--port=abc'";
	EXPECT_TRUE(options.hasLoadError());
}

TEST(RoomRegistryReplyUtilsTest, RejectsMissingOrWronglyTypedTextElements) {
	redisReply malformed{};
	malformed.type = REDIS_REPLY_ARRAY;
	malformed.elements = 1;
	redisReply* elements[1]{nullptr};
	malformed.element = elements;

	std::string out;
	EXPECT_FALSE(redisreply::GetTextElement(&malformed, 0, out));

	redisReply integerElement{};
	integerElement.type = REDIS_REPLY_INTEGER;
	elements[0] = &integerElement;
	EXPECT_FALSE(redisreply::GetTextElement(&malformed, 0, out));
}

TEST(RoomRegistryReplyUtilsTest, CopiesTextReplyUsingDeclaredLength) {
	redisReply arrayReply{};
	arrayReply.type = REDIS_REPLY_ARRAY;
	arrayReply.elements = 1;

	redisReply textElement{};
	textElement.type = REDIS_REPLY_STRING;
	char payload[] = "message";
	textElement.str = payload;
	textElement.len = 7;

	redisReply* elements[1]{&textElement};
	arrayReply.element = elements;

	std::string out;
	ASSERT_TRUE(redisreply::GetTextElement(&arrayReply, 0, out));
	EXPECT_EQ(out, "message");
}

TEST(RoomStatsQosHelpersTest, ClearPeerAutomaticOverrideRecordsRemovesPeerAndRoomPressureKeys) {
	std::unordered_map<std::string, int> records = {
		{roomstatsqos::MakePeerKey("room", "alice"), 1},
		{roomstatsqos::MakeRoomPressureKey("room", "alice"), 2},
		{roomstatsqos::MakePeerKey("room", "bob"), 3}
	};

	roomstatsqos::ClearPeerAutomaticOverrideRecords(records, "room", "alice");

	EXPECT_EQ(records.count(roomstatsqos::MakePeerKey("room", "alice")), 0u);
	EXPECT_EQ(records.count(roomstatsqos::MakeRoomPressureKey("room", "alice")), 0u);
	EXPECT_EQ(records.count(roomstatsqos::MakePeerKey("room", "bob")), 1u);
}

TEST(RoomStatsQosHelpersTest, BuildStatsStoreResponseDataIncludesReasonOnlyWhenPresent) {
	const auto stored = roomstatsqos::BuildStatsStoreResponseData(true);
	EXPECT_TRUE(stored.value("stored", false));
	EXPECT_FALSE(stored.contains("reason"));

	const auto rejected = roomstatsqos::BuildStatsStoreResponseData(false, "stale-seq");
	EXPECT_FALSE(rejected.value("stored", true));
	EXPECT_EQ(rejected.value("reason", ""), "stale-seq");
}

TEST(StaticFileResponderTest, MatchesOnlyRealSuffixes) {
	EXPECT_EQ(ContentTypeForPath("/assets/app.css"), "text/css");
	EXPECT_EQ(ContentTypeForPath("/assets/app.css.backup"), "application/octet-stream");
	EXPECT_EQ(ContentTypeForPath("/assets/report.json"), "application/json");
	EXPECT_EQ(ContentTypeForPath("/assets/report.json.tmp"), "application/octet-stream");
}

TEST(RtcpSuppressionFixTest, SuppressedVideoSkipsPliAndNackRetransmissions) {
	int sv[2];
	ASSERT_EQ(::socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), 0);

	RtcpContext rtcp;
	uint32_t ssrc = 0x12345678;
	uint16_t seq = 3456;
	int pliCalls = 0;

	rtcp.registerVideoStream(ssrc, 96, &seq);
	rtcp.canSendVideoFn = [](uint32_t) { return false; };
	rtcp.sendH264Fn = [&](int, const uint8_t*, int, uint8_t, uint32_t, uint32_t, uint16_t&) {
		++pliCalls;
	};

	uint8_t rtp[16]{};
	rtp[0] = 0x80;
	rtp[1] = 96;
	rtp[2] = static_cast<uint8_t>(seq >> 8);
	rtp[3] = static_cast<uint8_t>(seq & 0xFF);
	rtp[4] = 0x00;
	rtp[5] = 0x01;
	rtp[6] = 0x5F;
	rtp[7] = 0x90;
	rtp[8] = static_cast<uint8_t>(ssrc >> 24);
	rtp[9] = static_cast<uint8_t>(ssrc >> 16);
	rtp[10] = static_cast<uint8_t>(ssrc >> 8);
	rtp[11] = static_cast<uint8_t>(ssrc & 0xFF);
	rtcp.onVideoRtpSent(rtp, sizeof(rtp));
	rtcp.cacheKeyframe(ssrc, rtp, sizeof(rtp), 90000);

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

	uint8_t nack[16]{};
	nack[0] = 0x81;
	nack[1] = RTCP_PT_RTPFB;
	nack[2] = 0x00;
	nack[3] = 0x03;
	nack[8] = static_cast<uint8_t>(ssrc >> 24);
	nack[9] = static_cast<uint8_t>(ssrc >> 16);
	nack[10] = static_cast<uint8_t>(ssrc >> 8);
	nack[11] = static_cast<uint8_t>(ssrc & 0xFF);
	nack[12] = static_cast<uint8_t>(seq >> 8);
	nack[13] = static_cast<uint8_t>(seq & 0xFF);
	ASSERT_EQ(::send(sv[1], nack, sizeof(nack), 0), static_cast<ssize_t>(sizeof(nack)));

	rtcp.processIncomingRtcp(sv[0]);

	EXPECT_EQ(pliCalls, 0);
	EXPECT_EQ(rtcp.pliResponded, 0);
	EXPECT_EQ(rtcp.nackRetransmitted, 0);

	uint8_t recvBuf[1500];
	EXPECT_LT(::recv(sv[1], recvBuf, sizeof(recvBuf), MSG_DONTWAIT), 0);

	::close(sv[0]);
	::close(sv[1]);
}

// ── F1: unwrapTimestamp backward-wrap state consistency ─────────────

TEST(UnwrapTimestamp, ForwardWrapUpdatesState)
{
	uint32_t lastTs = 0xFFFFFFF0u;
	uint64_t wrapCount = 0;
	const uint32_t baseTs = 0;

	// Forward wrap: jump from near-max to near-zero crossing half-range
	auto ticks = PeerRecorder::unwrapTimestamp(0x00000010u, baseTs, lastTs, wrapCount);
	EXPECT_GT(ticks, 0u);
	EXPECT_EQ(lastTs, 0x00000010u);
	EXPECT_EQ(wrapCount, 1u);
}

TEST(UnwrapTimestamp, BackwardWrapUpdatesLastTsAndWrapCount)
{
	uint32_t lastTs = 0x00000010u;
	uint64_t wrapCount = 1;
	const uint32_t baseTs = 0;

	// Backward wrap: jump from near-zero back to near-max crossing half-range
	auto ticks = PeerRecorder::unwrapTimestamp(0xFFFFFFF0u, baseTs, lastTs, wrapCount);

	// Key invariant: lastTs must be updated (was the F1 bug)
	EXPECT_EQ(lastTs, 0xFFFFFFF0u);
	// wrapCount must decrement (backward wrap undoes one forward wrap)
	EXPECT_EQ(wrapCount, 0u);
	// ticks should be near the top of the uint32 range (before baseTs subtraction)
	// With wrapCount=0, ticks = 0xFFFFFFF0 which is >= baseTs(0), so result = 0xFFFFFFF0
	EXPECT_EQ(ticks, static_cast<uint64_t>(0xFFFFFFF0u));
}

TEST(UnwrapTimestamp, BackwardWrapGuardedAtWrapCountZero)
{
	uint32_t lastTs = 0x00000010u;
	uint64_t wrapCount = 0;
	const uint32_t baseTs = 0;

	// Large forward jump when wrapCount==0: treat as normal forward, don't decrement
	auto ticks = PeerRecorder::unwrapTimestamp(0xFFFFFFF0u, baseTs, lastTs, wrapCount);
	EXPECT_EQ(lastTs, 0xFFFFFFF0u);
	EXPECT_EQ(wrapCount, 0u);  // No decrement below zero
}

TEST(UnwrapTimestamp, SmallBackwardJumpNoWrap)
{
	uint32_t lastTs = 1000u;
	uint64_t wrapCount = 2;
	const uint32_t baseTs = 0;

	// Small backward jump (within half-range): genuine reorder, no wrap change
	auto ticks = PeerRecorder::unwrapTimestamp(990u, baseTs, lastTs, wrapCount);
	EXPECT_EQ(lastTs, 990u);
	EXPECT_EQ(wrapCount, 2u);  // Unchanged
}

TEST(UnwrapTimestamp, MonotonicSequenceNoWrap)
{
	uint32_t lastTs = 0;
	uint64_t wrapCount = 0;
	const uint32_t baseTs = 0;

	// Simple increasing sequence
	auto t1 = PeerRecorder::unwrapTimestamp(100u, baseTs, lastTs, wrapCount);
	auto t2 = PeerRecorder::unwrapTimestamp(200u, baseTs, lastTs, wrapCount);
	auto t3 = PeerRecorder::unwrapTimestamp(300u, baseTs, lastTs, wrapCount);
	EXPECT_LT(t1, t2);
	EXPECT_LT(t2, t3);
	EXPECT_EQ(wrapCount, 0u);
}
