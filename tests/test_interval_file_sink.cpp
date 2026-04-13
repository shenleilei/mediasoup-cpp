#include <gtest/gtest.h>
#include "IntervalFileSink.h"
#include <filesystem>

using namespace mediasoup::logging;

// --- bucket_tm ---

TEST(IntervalFileSinkTest, BucketTmZerosMinutesAndSeconds) {
	auto now = log_clock::now();
	auto tm = bucket_tm(now, 3);
	EXPECT_EQ(tm.tm_min, 0);
	EXPECT_EQ(tm.tm_sec, 0);
}

TEST(IntervalFileSinkTest, BucketTmBucketsHour) {
	// Create a time_point for hour 5
	std::tm input{};
	input.tm_year = 126;  // 2026
	input.tm_mon = 0;     // January
	input.tm_mday = 1;
	input.tm_hour = 5;
	input.tm_min = 30;
	input.tm_sec = 45;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto result = bucket_tm(tp, 3);
	EXPECT_EQ(result.tm_hour, 3);  // 5 / 3 * 3 = 3
	EXPECT_EQ(result.tm_min, 0);
	EXPECT_EQ(result.tm_sec, 0);
}

TEST(IntervalFileSinkTest, BucketTmWithRotateHoursOne) {
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 5;
	input.tm_mday = 15;
	input.tm_hour = 7;
	input.tm_min = 45;
	input.tm_sec = 30;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto result = bucket_tm(tp, 1);
	EXPECT_EQ(result.tm_hour, 7);  // 7 / 1 * 1 = 7
}

TEST(IntervalFileSinkTest, BucketTmWithRotateHoursZeroPreservesHour) {
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 0;
	input.tm_mday = 1;
	input.tm_hour = 5;
	input.tm_min = 30;
	input.tm_sec = 45;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto result = bucket_tm(tp, 0);
	EXPECT_EQ(result.tm_min, 0);
	EXPECT_EQ(result.tm_sec, 0);
	// With rotateHours=0, the if branch is skipped, hour is unchanged
	EXPECT_EQ(result.tm_hour, 5);
}

TEST(IntervalFileSinkTest, BucketTmSixHourBuckets) {
	// Hour 14 with 6-hour buckets should become hour 12
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 3;
	input.tm_mday = 10;
	input.tm_hour = 14;
	input.tm_min = 20;
	input.tm_sec = 10;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto result = bucket_tm(tp, 6);
	EXPECT_EQ(result.tm_hour, 12);  // 14 / 6 * 6 = 12
}

// --- build_bucketed_log_filename ---

TEST(IntervalFileSinkTest, BuildFilenameHasCorrectFormat) {
	std::tm input{};
	input.tm_year = 126;  // 2026
	input.tm_mon = 0;     // January
	input.tm_mday = 15;
	input.tm_hour = 9;
	input.tm_min = 30;
	input.tm_sec = 0;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto filename = build_bucketed_log_filename("", "sfu", 12345, tp, 3);
	// Hour 9 with 3-hour buckets → bucket at hour 9
	EXPECT_EQ(filename, "sfu_2026011509_12345.log");
}

TEST(IntervalFileSinkTest, BuildFilenameWithLogDir) {
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 0;
	input.tm_mday = 15;
	input.tm_hour = 9;
	input.tm_min = 30;
	input.tm_sec = 0;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto filename = build_bucketed_log_filename("/var/log", "sfu", 12345, tp, 3);
	auto expected = (std::filesystem::path("/var/log") / "sfu_2026011509_12345.log").string();
	EXPECT_EQ(filename, expected);
}

TEST(IntervalFileSinkTest, BuildFilenameEmptyLogDir) {
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 5;   // June
	input.tm_mday = 1;
	input.tm_hour = 0;
	input.tm_min = 0;
	input.tm_sec = 0;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto filename = build_bucketed_log_filename("", "app", 1, tp, 3);
	EXPECT_EQ(filename, "app_2026060100_1.log");
}

TEST(IntervalFileSinkTest, BuildFilenameWithDifferentPids) {
	std::tm input{};
	input.tm_year = 126;
	input.tm_mon = 0;
	input.tm_mday = 1;
	input.tm_hour = 0;
	input.tm_min = 0;
	input.tm_sec = 0;
	input.tm_isdst = -1;
	auto timeT = std::mktime(&input);
	auto tp = log_clock::from_time_t(timeT);

	auto fn1 = build_bucketed_log_filename("", "sfu", 100, tp, 3);
	auto fn2 = build_bucketed_log_filename("", "sfu", 200, tp, 3);
	EXPECT_NE(fn1, fn2);
	EXPECT_TRUE(fn1.find("100") != std::string::npos);
	EXPECT_TRUE(fn2.find("200") != std::string::npos);
}

// --- interval_file_sink creation ---

TEST(IntervalFileSinkTest, SinkCreatesFileInTmpDir) {
	auto tmpDir = std::filesystem::temp_directory_path() / "mediasoup_test_sink";
	std::filesystem::create_directories(tmpDir);

	{
		interval_file_sink_st sink(tmpDir.string(), "test", 99999, 3, true);
		auto filename = sink.filename();
		EXPECT_FALSE(filename.empty());
		EXPECT_TRUE(std::filesystem::exists(filename));
	}

	// Cleanup
	std::filesystem::remove_all(tmpDir);
}
