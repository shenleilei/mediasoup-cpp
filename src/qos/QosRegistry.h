#pragma once
#include "qos/QosSnapshot.h"
#include <string>
#include <unordered_map>

namespace mediasoup::qos {

class QosRegistry {
public:
	struct Entry {
		ClientQosSnapshot snapshot;
		int64_t receivedAtMs{ 0 };
	};

	bool Upsert(const std::string& roomId, const std::string& peerId,
		const ClientQosSnapshot& snapshot, int64_t receivedAtMs = NowMs(),
		std::string* rejectReason = nullptr);
	const Entry* Get(const std::string& roomId, const std::string& peerId) const;
	void ErasePeer(const std::string& roomId, const std::string& peerId);
	void EraseRoom(const std::string& roomId);
	void Clear();

private:
	using PeerMap = std::unordered_map<std::string, Entry>;
	std::unordered_map<std::string, PeerMap> rooms_;
};

} // namespace mediasoup::qos
