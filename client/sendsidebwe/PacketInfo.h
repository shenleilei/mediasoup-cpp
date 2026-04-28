#pragma once

#include "../ccutils/ProbeTypes.h"

#include <cstddef>
#include <cstdint>

namespace mediasoup::plainclient::sendsidebwe {

struct PacketInfo {
	uint64_t sequenceNumber{ 0 };
	int64_t sendTimeUs{ 0 };
	int64_t recvTimeUs{ 0 };
	mediasoup::ccutils::ProbeClusterId probeClusterId{ mediasoup::ccutils::ProbeClusterIdInvalid };
	uint16_t size{ 0 };
	bool isRtx{ false };
	bool isProbe{ false };
};

} // namespace mediasoup::plainclient::sendsidebwe
