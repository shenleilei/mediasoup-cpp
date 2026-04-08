// Unit tests for GeoRouter: IP lookup, scoring, ISP handling
#include <gtest/gtest.h>
#include "GeoRouter.h"

using namespace mediasoup;

class GeoRouterTest : public ::testing::Test {
protected:
	static GeoRouter geo_;
	static bool inited_;

	static void SetUpTestSuite() {
		Logger::Init("", "warn");
		inited_ = geo_.init("ip2region.xdb");
	}

	void SetUp() override {
		if (!inited_) GTEST_SKIP() << "ip2region.xdb not found";
	}
};
GeoRouter GeoRouterTest::geo_;
bool GeoRouterTest::inited_ = false;

// ── IP Lookup ──

TEST_F(GeoRouterTest, LookupBeijingTelecom) {
	auto info = geo_.lookup("36.110.147.0");
	EXPECT_TRUE(info.valid);
	EXPECT_EQ(info.country, "中国");
	EXPECT_EQ(info.isp, "电信");
	EXPECT_NEAR(info.lat, 39.90, 0.5);
	EXPECT_NEAR(info.lng, 116.40, 0.5);
}

TEST_F(GeoRouterTest, LookupGuangzhouMobile) {
	auto info = geo_.lookup("183.232.231.172");
	EXPECT_TRUE(info.valid);
	EXPECT_EQ(info.isp, "移动");
	EXPECT_NEAR(info.lat, 23.13, 0.5);
}

TEST_F(GeoRouterTest, LookupCloudIP) {
	auto info = geo_.lookup("47.99.237.234"); // Alibaba Cloud
	EXPECT_TRUE(info.valid);
	EXPECT_TRUE(GeoRouter::isCloudIsp(info.isp));
}

TEST_F(GeoRouterTest, LookupOverseasIP) {
	auto info = geo_.lookup("13.112.0.0"); // AWS Tokyo
	EXPECT_TRUE(info.valid);
	EXPECT_NE(info.lat, 0);
}

TEST_F(GeoRouterTest, LookupInvalidIP) {
	auto info = geo_.lookup("999.999.999.999");
	EXPECT_FALSE(info.valid);
}

TEST_F(GeoRouterTest, LookupPrivateIP) {
	auto info = geo_.lookup("192.168.1.1");
	// Private IPs may return valid=true but with "0" or "内网IP" — coords should be 0
	EXPECT_EQ(info.lat, 0);
	EXPECT_EQ(info.lng, 0);
}

// ── Haversine Distance ──

TEST_F(GeoRouterTest, HaversineBeijingToShanghai) {
	double d = GeoRouter::haversine(39.90, 116.40, 31.23, 121.47);
	EXPECT_NEAR(d, 1068, 50); // ~1068km
}

TEST_F(GeoRouterTest, HaversineSamePoint) {
	EXPECT_DOUBLE_EQ(GeoRouter::haversine(30.0, 120.0, 30.0, 120.0), 0);
}

// ── ISP Classification ──

TEST_F(GeoRouterTest, CloudIspDetection) {
	EXPECT_TRUE(GeoRouter::isCloudIsp("阿里"));
	EXPECT_TRUE(GeoRouter::isCloudIsp("腾讯"));
	EXPECT_TRUE(GeoRouter::isCloudIsp("华为"));
	EXPECT_TRUE(GeoRouter::isCloudIsp("百度"));
	EXPECT_TRUE(GeoRouter::isCloudIsp("天翼云"));
	EXPECT_FALSE(GeoRouter::isCloudIsp("电信"));
	EXPECT_FALSE(GeoRouter::isCloudIsp("联通"));
	EXPECT_FALSE(GeoRouter::isCloudIsp("移动"));
	EXPECT_FALSE(GeoRouter::isCloudIsp(""));
}

// ── Scoring ──

TEST_F(GeoRouterTest, SameIspNopenalty) {
	auto bj = geo_.lookup("36.110.147.0"); // 北京电信
	double dist = GeoRouter::haversine(bj.lat, bj.lng, 30.27, 120.15);
	double score = geo_.score(bj, 30.27, 120.15, "电信");
	EXPECT_DOUBLE_EQ(score, dist); // no penalty
}

TEST_F(GeoRouterTest, CrossIspMultiplier) {
	auto bj = geo_.lookup("36.110.147.0"); // 北京电信
	double dist = GeoRouter::haversine(bj.lat, bj.lng, 30.27, 120.15);
	double score = geo_.score(bj, 30.27, 120.15, "联通");
	// max(dist, 50) * 2.0
	EXPECT_NEAR(score, std::max(dist, 50.0) * 2.0, 1.0);
}

TEST_F(GeoRouterTest, CloudIspNoPenalty) {
	auto bj = geo_.lookup("36.110.147.0"); // 北京电信
	double dist = GeoRouter::haversine(bj.lat, bj.lng, 30.27, 120.15);
	double score = geo_.score(bj, 30.27, 120.15, "阿里");
	EXPECT_DOUBLE_EQ(score, dist); // cloud = no penalty
}

TEST_F(GeoRouterTest, SameCityCrossIspHasFloor) {
	auto bj = geo_.lookup("36.110.147.0"); // 北京电信
	double score = geo_.score(bj, 39.90, 116.40, "联通"); // same city, diff ISP
	EXPECT_NEAR(score, 100, 5); // max(~0, 50) * 2.0 = 100
	EXPECT_GT(score, 0); // must not be zero
}

TEST_F(GeoRouterTest, OverseasNoPenalty) {
	auto us = geo_.lookup("8.8.8.8");
	double dist = GeoRouter::haversine(us.lat, us.lng, 35.68, 139.69);
	double score = geo_.score(us, 35.68, 139.69, "电信");
	// Overseas: no ISP penalty regardless
	EXPECT_DOUBLE_EQ(score, dist);
}

// ── Ranking Order ──

TEST_F(GeoRouterTest, RankingOrder) {
	auto bj = geo_.lookup("36.110.147.0"); // 北京电信

	double sameIspNear = geo_.score(bj, 39.13, 117.20, "电信");    // 天津电信
	double crossIspNear = geo_.score(bj, 39.13, 117.20, "联通");   // 天津联通
	double cloudNear = geo_.score(bj, 39.13, 117.20, "阿里");      // 天津阿里云
	double sameIspFar = geo_.score(bj, 23.13, 113.26, "电信");     // 广州电信
	double crossIspFar = geo_.score(bj, 23.13, 113.26, "联通");    // 广州联通

	// Same ISP near ≈ Cloud near < Cross ISP near
	EXPECT_DOUBLE_EQ(sameIspNear, cloudNear);
	EXPECT_LT(sameIspNear, crossIspNear);

	// Near < Far (same ISP)
	EXPECT_LT(sameIspNear, sameIspFar);

	// Same ISP far < Cross ISP far
	EXPECT_LT(sameIspFar, crossIspFar);

	// Cross ISP near < Same ISP far (nearby wins even with ISP mismatch)
	EXPECT_LT(crossIspNear, sameIspFar);
}
