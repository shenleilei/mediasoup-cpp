#include <gtest/gtest.h>
#include "qos/QosRegistry.h"
#include "qos/QosValidator.h"
#include <fstream>

using namespace mediasoup;
using json = nlohmann::json;

namespace {

json LoadFixture(const std::string& name) {
	std::ifstream f("tests/fixtures/qos_protocol/" + name + ".json");
	EXPECT_TRUE(f.is_open()) << "fixture not found: " << name;
	json data;
	f >> data;
	return data;
}

qos::ClientQosSnapshot ValidSnapshot() {
	auto parsed = qos::QosValidator::ParseClientSnapshot(LoadFixture("valid_client_v1"));
	EXPECT_TRUE(parsed.ok) << parsed.error;
	return parsed.value;
}

} // namespace

TEST(QosRegistryTest, UpsertStoresLatestSnapshot) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();

	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 42u);
	EXPECT_EQ(entry->receivedAtMs, 1000);
}

TEST(QosRegistryTest, UpsertRejectsStaleOrDuplicateSeq) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();
	std::string rejectReason;

	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));
	EXPECT_FALSE(registry.Upsert("room1", "alice", snapshot, 2000, &rejectReason));
	EXPECT_EQ(rejectReason, "stale-seq");

	auto newer = snapshot;
	newer.seq = 43u;
	EXPECT_TRUE(registry.Upsert("room1", "alice", newer, 3000));

	auto older = snapshot;
	older.seq = 41u;
	EXPECT_FALSE(registry.Upsert("room1", "alice", older, 4000, &rejectReason));
	EXPECT_EQ(rejectReason, "stale-seq");
}

TEST(QosRegistryTest, SeqResetBoundary1000Rejected) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();
	snapshot.seq = 2000;
	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));

	// Jump back by exactly 1000 → still within threshold → rejected as stale
	auto reset = snapshot;
	reset.seq = 1000;
	std::string reason;
	EXPECT_FALSE(registry.Upsert("room1", "alice", reset, 2000, &reason));
	EXPECT_EQ(reason, "stale-seq");
}

TEST(QosRegistryTest, SeqResetBoundary1001Accepted) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();
	snapshot.seq = 2001;
	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));

	// Jump back by 1001 → exceeds threshold → accepted as client reset
	auto reset = snapshot;
	reset.seq = 1000;
	EXPECT_TRUE(registry.Upsert("room1", "alice", reset, 2000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 1000u);
}

TEST(QosRegistryTest, SeqResetFromZeroAccepted) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();
	snapshot.seq = 5000;
	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));

	// Client reloaded, seq restarts from 0
	auto reset = snapshot;
	reset.seq = 0;
	EXPECT_TRUE(registry.Upsert("room1", "alice", reset, 2000));

	auto* entry = registry.Get("room1", "alice");
	ASSERT_NE(entry, nullptr);
	EXPECT_EQ(entry->snapshot.seq, 0u);
}

TEST(QosRegistryTest, SeqContinuesAfterReset) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();
	snapshot.seq = 5000;
	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));

	// Reset to 0
	auto reset = snapshot;
	reset.seq = 0;
	EXPECT_TRUE(registry.Upsert("room1", "alice", reset, 2000));

	// Subsequent increments work normally after reset
	auto next = snapshot;
	next.seq = 1;
	EXPECT_TRUE(registry.Upsert("room1", "alice", next, 3000));

	// Stale relative to new baseline is still rejected
	auto stale = snapshot;
	stale.seq = 0;
	std::string reason;
	EXPECT_FALSE(registry.Upsert("room1", "alice", stale, 4000, &reason));
	EXPECT_EQ(reason, "stale-seq");
}

TEST(QosRegistryTest, ErasePeerAndRoomWork) {
	qos::QosRegistry registry;
	auto snapshot = ValidSnapshot();

	EXPECT_TRUE(registry.Upsert("room1", "alice", snapshot, 1000));
	EXPECT_TRUE(registry.Upsert("room1", "bob", snapshot, 1000));
	EXPECT_TRUE(registry.Upsert("room2", "carol", snapshot, 1000));

	registry.ErasePeer("room1", "alice");
	EXPECT_EQ(registry.Get("room1", "alice"), nullptr);
	EXPECT_NE(registry.Get("room1", "bob"), nullptr);

	registry.EraseRoom("room1");
	EXPECT_EQ(registry.Get("room1", "bob"), nullptr);
	EXPECT_NE(registry.Get("room2", "carol"), nullptr);
}
