#pragma once
#include "Logger.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <sstream>
#include <mutex>

extern "C" {
#include "xdb_api.h"
}

namespace mediasoup {

// Parsed ip2region result: "国家|区域|省|市|运营商"
struct GeoInfo {
	std::string country;   // 中国, 美国, ...
	std::string province;  // 北京, 广东, ...
	std::string city;      // 北京, 广州, ...
	std::string isp;       // 联通, 电信, 移动, ...
	double lat = 0;
	double lng = 0;
	bool valid = false;
};

class GeoRouter {
public:
	GeoRouter() : logger_(Logger::Get("GeoRouter")) {}

	bool init(const std::string& xdbPath) {
		xdb_init_winsock();
		content_ = xdb_load_content_from_file(xdbPath.c_str());
		if (!content_) {
			MS_ERROR(logger_, "Failed to load xdb from {}", xdbPath);
			return false;
		}
		int err = xdb_new_with_buffer(XDB_IPv4, &searcher_, content_);
		if (err != 0) {
			MS_ERROR(logger_, "Failed to create xdb searcher: {}", err);
			xdb_free_content(content_);
			content_ = nullptr;
			return false;
		}
		initCityCoords();
		MS_DEBUG(logger_, "GeoRouter initialized with {} city coords", cityCoords_.size());
		return true;
	}

	~GeoRouter() {
		if (content_) {
			xdb_close(&searcher_);
			xdb_free_content(content_);
		}
		xdb_clean_winsock();
	}

	// Lookup IP → GeoInfo
	GeoInfo lookup(const std::string& ip) {
		GeoInfo info;
		if (!content_) return info;

		xdb_region_buffer_t region;
		xdb_region_buffer_init(&region, nullptr, 0);

		int err;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			err = xdb_search_by_string(&searcher_, ip.c_str(), &region);
		}
		if (err != 0 || !region.value) {
			xdb_region_buffer_free(&region);
			return info;
		}

		// Parse "国家|区域|省|市|运营商"
		std::string val(region.value);
		xdb_region_buffer_free(&region);

		std::vector<std::string> parts;
		std::istringstream ss(val);
		std::string token;
		while (std::getline(ss, token, '|')) parts.push_back(token);

		if (parts.size() >= 5) {
			info.country = parts[0];
			info.province = parts[1];
			info.city = parts[2];
			info.isp = parts[3];
			// parts[4] is country code (CN, US, etc.)
			info.valid = true;

			// Resolve coordinates: try city first, then province
			std::string key = (info.city != "0" && !info.city.empty()) ? info.city : info.province;
			// Strip trailing "市"/"省" for matching
			if (key.size() >= 3) {
				std::string suffix3 = key.substr(key.size() - 3);
				if (suffix3 == "市" || suffix3 == "省") key = key.substr(0, key.size() - 3);
			}
			auto it = cityCoords_.find(key);
			if (it != cityCoords_.end()) {
				info.lat = it->second.first;
				info.lng = it->second.second;
			}
		}
		return info;
	}

	bool isChina(const GeoInfo& info) const {
		return info.valid && info.country == "中国";
	}

	// Score a candidate node. Lower = better.
	// For China: prioritize ISP match, then distance.
	// For overseas: pure distance.
	double score(const GeoInfo& client, double nodeLat, double nodeLng,
		const std::string& nodeIsp) const
	{
		double dist = haversine(client.lat, client.lng, nodeLat, nodeLng);

		if (isChina(client) && !nodeIsp.empty() && !client.isp.empty()) {
			bool ispMatch = (client.isp == nodeIsp);
			// ISP mismatch adds a 1000km penalty (roughly the cost of cross-ISP routing)
			if (!ispMatch) dist += 1000.0;
		}

		return dist;
	}

	// Haversine distance in km
	static double haversine(double lat1, double lng1, double lat2, double lng2) {
		constexpr double R = 6371.0;
		double dLat = toRad(lat2 - lat1);
		double dLng = toRad(lng2 - lng1);
		double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
			std::cos(toRad(lat1)) * std::cos(toRad(lat2)) *
			std::sin(dLng / 2) * std::sin(dLng / 2);
		return R * 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
	}

private:
	static double toRad(double deg) { return deg * M_PI / 180.0; }

	void initCityCoords() {
		// Major Chinese cities + some international cities
		// Format: name → {lat, lng}
		auto& c = cityCoords_;
		// 直辖市
		c["北京"] = {39.90, 116.40};
		c["上海"] = {31.23, 121.47};
		c["天津"] = {39.13, 117.20};
		c["重庆"] = {29.56, 106.55};
		// 省会 & 主要城市
		c["广州"] = {23.13, 113.26};
		c["深圳"] = {22.54, 114.06};
		c["杭州"] = {30.27, 120.15};
		c["南京"] = {32.06, 118.80};
		c["成都"] = {30.57, 104.07};
		c["武汉"] = {30.59, 114.30};
		c["西安"] = {34.26, 108.94};
		c["长沙"] = {28.23, 112.94};
		c["郑州"] = {34.75, 113.65};
		c["济南"] = {36.65, 116.98};
		c["青岛"] = {36.07, 120.38};
		c["大连"] = {38.91, 121.60};
		c["沈阳"] = {41.80, 123.43};
		c["哈尔滨"] = {45.75, 126.65};
		c["长春"] = {43.88, 125.32};
		c["福州"] = {26.07, 119.30};
		c["厦门"] = {24.48, 118.09};
		c["合肥"] = {31.82, 117.23};
		c["南昌"] = {28.68, 115.86};
		c["昆明"] = {25.04, 102.71};
		c["贵阳"] = {26.65, 106.63};
		c["南宁"] = {22.82, 108.37};
		c["海口"] = {20.02, 110.35};
		c["石家庄"] = {38.04, 114.51};
		c["太原"] = {37.87, 112.55};
		c["呼和浩特"] = {40.84, 111.75};
		c["兰州"] = {36.06, 103.83};
		c["银川"] = {38.49, 106.23};
		c["西宁"] = {36.62, 101.78};
		c["乌鲁木齐"] = {43.83, 87.62};
		c["拉萨"] = {29.65, 91.13};
		c["苏州"] = {31.30, 120.62};
		c["无锡"] = {31.49, 120.31};
		c["宁波"] = {29.87, 121.55};
		c["东莞"] = {23.04, 113.75};
		c["佛山"] = {23.02, 113.12};
		c["珠海"] = {22.27, 113.58};
		c["温州"] = {28.00, 120.67};
		// 省份 (fallback when city is "0")
		c["广东"] = {23.13, 113.26};
		c["浙江"] = {30.27, 120.15};
		c["江苏"] = {32.06, 118.80};
		c["山东"] = {36.65, 116.98};
		c["河南"] = {34.75, 113.65};
		c["四川"] = {30.57, 104.07};
		c["湖北"] = {30.59, 114.30};
		c["湖南"] = {28.23, 112.94};
		c["福建"] = {26.07, 119.30};
		c["安徽"] = {31.82, 117.23};
		c["河北"] = {38.04, 114.51};
		c["陕西"] = {34.26, 108.94};
		c["辽宁"] = {41.80, 123.43};
		c["江西"] = {28.68, 115.86};
		c["云南"] = {25.04, 102.71};
		c["贵州"] = {26.65, 106.63};
		c["广西"] = {22.82, 108.37};
		c["山西"] = {37.87, 112.55};
		c["吉林"] = {43.88, 125.32};
		c["黑龙江"] = {45.75, 126.65};
		c["甘肃"] = {36.06, 103.83};
		c["内蒙古"] = {40.84, 111.75};
		c["海南"] = {20.02, 110.35};
		c["宁夏"] = {38.49, 106.23};
		c["青海"] = {36.62, 101.78};
		c["新疆"] = {43.83, 87.62};
		c["西藏"] = {29.65, 91.13};
		c["香港"] = {22.32, 114.17};
		c["澳门"] = {22.20, 113.55};
		c["台湾"] = {25.03, 121.57};
		// International
		c["东京"] = {35.68, 139.69};
		c["首尔"] = {37.57, 126.98};
		c["新加坡"] = {1.35, 103.82};
		c["孟买"] = {19.08, 72.88};
		c["悉尼"] = {-33.87, 151.21};
		c["伦敦"] = {51.51, -0.13};
		c["法兰克福"] = {50.11, 8.68};
		c["弗吉尼亚"] = {38.95, -77.45};
		c["硅谷"] = {37.39, -122.08};
		c["圣保罗"] = {-23.55, -46.63};
	}

	xdb_content_t* content_ = nullptr;
	xdb_searcher_t searcher_{};
	std::mutex mutex_;
	std::unordered_map<std::string, std::pair<double, double>> cityCoords_;
	std::shared_ptr<spdlog::logger> logger_;
};

} // namespace mediasoup
