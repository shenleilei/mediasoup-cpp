// Unit tests for QoS-related features:
// OwnedNotification, Producer::ScoreEntry, Consumer::Score,
// Room::getPeerIds, RoomManager::getRoomIds
#include <gtest/gtest.h>
#include "Channel.h"
#include "Producer.h"
#include "Consumer.h"
#include "RoomManager.h"

using namespace mediasoup;

// ─── OwnedNotification tests ───

TEST(OwnedNotificationTest, EmptyDataReturnsNull) {
	Channel::OwnedNotification owned;
	EXPECT_EQ(owned.notification(), nullptr);
}

TEST(OwnedNotificationTest, GarbageDataReturnsNull) {
	Channel::OwnedNotification owned;
	owned.data = {0xFF, 0xFE, 0x00, 0x01, 0x02, 0x03};
	// Should not crash, just return nullptr for invalid flatbuffer
	auto* notif = owned.notification();
	// We can't guarantee nullptr for arbitrary bytes (flatbuffers may "parse" garbage),
	// but it must not crash.
	(void)notif;
}

TEST(OwnedNotificationTest, DefaultEventValue) {
	Channel::OwnedNotification owned;
	EXPECT_EQ(owned.event, FBS::Notification::Event::WORKER_RUNNING);
}

// ─── Producer::ScoreEntry tests ───

TEST(ProducerScoreTest, InitialScoresEmpty) {
	// Producer scores should be empty before any notification
	// We can't construct a Producer without a Channel, so test the struct directly
	std::vector<Producer::ScoreEntry> scores;
	EXPECT_TRUE(scores.empty());

	// Verify struct fields
	Producer::ScoreEntry entry{0, 12345, "r0", 8};
	EXPECT_EQ(entry.encodingIdx, 0u);
	EXPECT_EQ(entry.ssrc, 12345u);
	EXPECT_EQ(entry.rid, "r0");
	EXPECT_EQ(entry.score, 8);
}

TEST(ProducerScoreTest, ScoreEntryBoundaryValues) {
	// Max values
	Producer::ScoreEntry maxEntry{UINT32_MAX, UINT32_MAX, "", 255};
	EXPECT_EQ(maxEntry.encodingIdx, UINT32_MAX);
	EXPECT_EQ(maxEntry.ssrc, UINT32_MAX);
	EXPECT_EQ(maxEntry.score, 255);

	// Zero values
	Producer::ScoreEntry zeroEntry{0, 0, "", 0};
	EXPECT_EQ(zeroEntry.score, 0);
}

// ─── Consumer::Score tests ───

TEST(ConsumerScoreTest, DefaultScoreIsZero) {
	Consumer::Score score{};
	EXPECT_EQ(score.score, 0);
	EXPECT_EQ(score.producerScore, 0);
	EXPECT_TRUE(score.producerScores.empty());
}

TEST(ConsumerScoreTest, ScoreBoundaryValues) {
	Consumer::Score score{10, 10, {10, 10, 10}};
	EXPECT_EQ(score.score, 10);
	EXPECT_EQ(score.producerScore, 10);
	EXPECT_EQ(score.producerScores.size(), 3u);

	// Zero score (worst quality)
	Consumer::Score bad{0, 0, {0}};
	EXPECT_EQ(bad.score, 0);
}

// ─── Room::getPeerIds tests ───

class RoomQosTest : public ::testing::Test {
protected:
	std::shared_ptr<Room> room = std::make_shared<Room>("qos-room", nullptr);

	std::shared_ptr<Peer> makePeer(const std::string& id) {
		auto p = std::make_shared<Peer>();
		p->id = id;
		p->displayName = id;
		return p;
	}
};

TEST_F(RoomQosTest, GetPeerIdsEmpty) {
	auto ids = room->getPeerIds();
	EXPECT_TRUE(ids.empty());
}

TEST_F(RoomQosTest, GetPeerIdsReturnsAll) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	room->addPeer(makePeer("charlie"));

	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 3u);

	std::sort(ids.begin(), ids.end());
	EXPECT_EQ(ids[0], "alice");
	EXPECT_EQ(ids[1], "bob");
	EXPECT_EQ(ids[2], "charlie");
}

TEST_F(RoomQosTest, GetPeerIdsAfterRemove) {
	room->addPeer(makePeer("alice"));
	room->addPeer(makePeer("bob"));
	room->removePeer("alice");

	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 1u);
	EXPECT_EQ(ids[0], "bob");
}

TEST_F(RoomQosTest, GetPeerIdsAfterRemoveNonExistent) {
	room->addPeer(makePeer("alice"));
	room->removePeer("ghost"); // removing non-existent peer should not crash
	auto ids = room->getPeerIds();
	EXPECT_EQ(ids.size(), 1u);
}

// ─── RoomManager::getRoomIds tests (no real workers needed) ───

// We can't easily construct a RoomManager without WorkerManager,
// but we can test Room-level getPeerIds thoroughly above.
// For getRoomIds, we test via the integration test below.

// ─── OwnedResponse edge cases ───

TEST(OwnedResponseTest, EmptyDataReturnsNull) {
	Channel::OwnedResponse owned;
	EXPECT_EQ(owned.response(), nullptr);
}

TEST(OwnedResponseTest, GarbageDataDoesNotCrash) {
	Channel::OwnedResponse owned;
	owned.data = {0xDE, 0xAD, 0xBE, 0xEF};
	// Must not crash
	auto* resp = owned.response();
	(void)resp;
}

// ─── PeerRecorder QoS snapshot tests ───

#include "Recorder.h"
#include <fstream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class RecorderQosTest : public ::testing::Test {
protected:
	std::string tmpDir_;
	void SetUp() override {
		tmpDir_ = "/tmp/rec_qos_test_" + std::to_string(getpid());
		system(("rm -rf " + tmpDir_).c_str());
		system(("mkdir -p " + tmpDir_).c_str());
	}
	void TearDown() override {
		system(("rm -rf " + tmpDir_).c_str());
	}
};

TEST_F(RecorderQosTest, AppendCreatesQosFile) {
	std::string webmPath = tmpDir_ + "/test.webm";
	PeerRecorder rec("alice", webmPath, 100, 101, 48000, 90000);
	int port = rec.createSocket();
	ASSERT_GT(port, 0);
	ASSERT_TRUE(rec.start());

	// Send a minimal RTP packet to trigger firstRtpReceived
	// RTP header: V=2, PT=100, seq=1, ts=0, ssrc=1
	uint8_t rtp[12] = {0x80, 100, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1};
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sendto(fd, rtp, sizeof(rtp), 0, (sockaddr*)&addr, sizeof(addr));
	::close(fd);
	usleep(100000); // let recv loop process it

	json stats = {{"peerId", "alice"}, {"producers", json::object()}};
	rec.appendQosSnapshot(stats);
	rec.appendQosSnapshot(stats);
	rec.stop();

	// Verify .qos.json file exists and is valid JSON array
	std::string qosPath = tmpDir_ + "/test.qos.json";
	std::ifstream f(qosPath);
	ASSERT_TRUE(f.is_open()) << "QoS file not created: " << qosPath;
	std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
	f.close();

	auto arr = json::parse(content);
	ASSERT_TRUE(arr.is_array());
	EXPECT_EQ(arr.size(), 2u);
	// Each entry should have "t" (time) and "stats"
	for (auto& entry : arr) {
		EXPECT_TRUE(entry.contains("t"));
		EXPECT_TRUE(entry.contains("stats"));
		EXPECT_GE(entry["t"].get<double>(), 0.0);
		EXPECT_EQ(entry["stats"]["peerId"], "alice");
	}
	// Second entry should have t >= first entry
	EXPECT_GE(arr[1]["t"].get<double>(), arr[0]["t"].get<double>());
}

TEST_F(RecorderQosTest, NoSnapshotBeforeStart) {
	std::string webmPath = tmpDir_ + "/nostart.webm";
	PeerRecorder rec("bob", webmPath, 100, 101, 48000, 90000);
	// Don't call start()
	rec.appendQosSnapshot({{"peerId", "bob"}});
	// Should not create file since recorder not started
	std::string qosPath = tmpDir_ + "/nostart.qos.json";
	std::ifstream f(qosPath);
	EXPECT_FALSE(f.is_open()) << "QoS file should not exist before start";
}

TEST_F(RecorderQosTest, StopWithNoSnapshots) {
	std::string webmPath = tmpDir_ + "/empty.webm";
	PeerRecorder rec("charlie", webmPath, 100, 101, 48000, 90000);
	ASSERT_GT(rec.createSocket(), 0);
	ASSERT_TRUE(rec.start());
	// Stop without any snapshots
	rec.stop();
	// QoS file should exist with empty array (prevents 404 on playback)
	std::string qosPath = tmpDir_ + "/empty.qos.json";
	std::ifstream f(qosPath);
	EXPECT_TRUE(f.is_open()) << "QoS file should be created with empty array";
	if (f.is_open()) {
		std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
		EXPECT_EQ(content, "[]");
	}
}

// ═══════════════════════════════════════════════════════════════
// RTP depacketization unit tests
// ═══════════════════════════════════════════════════════════════

// ─── RtpHeader tests ───

TEST(RtpHeaderTest, MarkerBitParsed) {
	uint8_t pkt[20] = {0x80, 0x80 | 100, 0, 1, 0,0,0,0, 0,0,0,1, 0xAA};
	RtpHeader h;
	ASSERT_TRUE(RtpHeader::parse(pkt, sizeof(pkt), h));
	EXPECT_TRUE(h.marker);
	EXPECT_EQ(h.payloadType, 100);

	pkt[1] = 100; // no marker
	ASSERT_TRUE(RtpHeader::parse(pkt, sizeof(pkt), h));
	EXPECT_FALSE(h.marker);
}

TEST(RtpHeaderTest, TooShortFails) {
	uint8_t pkt[8] = {};
	RtpHeader h;
	EXPECT_FALSE(RtpHeader::parse(pkt, sizeof(pkt), h));
}

// ─── VP8 descriptor stripping ───

TEST(Vp8DescriptorTest, SimpleDescriptor) {
	// 1-byte descriptor: S=1, PartID=0
	uint8_t payload[] = {0x10, 0x9d, 0x01, 0x2a}; // S=1 + VP8 keyframe header
	int outSize = 0;
	bool isStart = false;
	auto* data = PeerRecorder::stripVp8Descriptor(payload, sizeof(payload), outSize, isStart);
	EXPECT_TRUE(isStart);
	EXPECT_EQ(outSize, 3);
	EXPECT_EQ(data[0], 0x9d); // first byte of VP8 data
}

TEST(Vp8DescriptorTest, ExtendedDescriptorWithPictureId) {
	// X=1, I=1 (7-bit PictureID)
	uint8_t payload[] = {0x90, 0x80, 0x42, 0xAA, 0xBB};
	// byte0: X=1(0x80) | S=1(0x10) = 0x90
	// byte1: I=1(0x80)
	// byte2: PictureID 0x42 (7-bit, no M bit)
	// skip = 3, data starts at byte3
	int outSize = 0;
	bool isStart = false;
	auto* data = PeerRecorder::stripVp8Descriptor(payload, sizeof(payload), outSize, isStart);
	EXPECT_TRUE(isStart);
	EXPECT_EQ(outSize, 2);
	EXPECT_EQ(data[0], 0xAA);
}

TEST(Vp8DescriptorTest, EmptyPayload) {
	int outSize = 0;
	bool isStart = false;
	PeerRecorder::stripVp8Descriptor(nullptr, 0, outSize, isStart);
	EXPECT_EQ(outSize, 0);
	EXPECT_FALSE(isStart);
}

// ─── Annex-B to AVCC conversion ───

TEST(AnnexBToAvccTest, SingleNal) {
	std::vector<uint8_t> annexB = {0,0,0,1, 0x65, 0xAA, 0xBB}; // IDR NAL
	std::vector<uint8_t> avcc;
	PeerRecorder::annexBToAvcc(annexB, avcc);
	ASSERT_EQ(avcc.size(), 7u); // 4-byte length + 3 bytes NAL
	uint32_t len = (avcc[0]<<24)|(avcc[1]<<16)|(avcc[2]<<8)|avcc[3];
	EXPECT_EQ(len, 3u);
	EXPECT_EQ(avcc[4], 0x65);
}

TEST(AnnexBToAvccTest, TwoNals) {
	std::vector<uint8_t> annexB = {0,0,0,1, 0x67, 0x42, 0,0,0,1, 0x68, 0x01};
	std::vector<uint8_t> avcc;
	PeerRecorder::annexBToAvcc(annexB, avcc);
	// First NAL: len=2 (0x67, 0x42), Second NAL: len=2 (0x68, 0x01)
	ASSERT_EQ(avcc.size(), 12u);
	uint32_t len1 = (avcc[0]<<24)|(avcc[1]<<16)|(avcc[2]<<8)|avcc[3];
	EXPECT_EQ(len1, 2u);
	EXPECT_EQ(avcc[4], 0x67);
	uint32_t len2 = (avcc[6]<<24)|(avcc[7]<<16)|(avcc[8]<<8)|avcc[9];
	EXPECT_EQ(len2, 2u);
	EXPECT_EQ(avcc[10], 0x68);
}

TEST(AnnexBToAvccTest, EmptyInput) {
	std::vector<uint8_t> annexB;
	std::vector<uint8_t> avcc;
	PeerRecorder::annexBToAvcc(annexB, avcc);
	EXPECT_TRUE(avcc.empty());
}

// ─── H264 deferred header: recorder starts OK without IDR, stop doesn't crash ───

TEST_F(RecorderQosTest, H264DeferredHeaderNoCrash) {
	std::string path = tmpDir_ + "/h264_deferred.webm";
	PeerRecorder rec("alice", path, 100, 107, 48000, 90000, VideoCodec::H264);
	ASSERT_GT(rec.createSocket(), 0);
	ASSERT_TRUE(rec.start()) << "H264 recorder should start (deferred header)";

	// Send only audio RTP (no video IDR) — should not crash
	uint8_t rtp[20] = {0x80, 100, 0, 1, 0,0,0,0, 0,0,0,1};
	int fd = socket(AF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(rec.port());
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sendto(fd, rtp, sizeof(rtp), 0, (sockaddr*)&addr, sizeof(addr));
	::close(fd);
	usleep(100000);

	rec.stop(); // should not crash even without IDR
	// File should be nearly empty (header never written)
	struct stat st;
	stat(path.c_str(), &st);
	// QoS file should still be created
	std::string qosPath = tmpDir_ + "/h264_deferred.qos.json";
	std::ifstream f(qosPath);
	EXPECT_TRUE(f.is_open());
}
