#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace mediasoup::media::h264 {

void AnnexBToAvcc(const uint8_t* annexB, size_t annexBSize, std::vector<uint8_t>* avcc);

inline void AnnexBToAvcc(const std::vector<uint8_t>& annexB, std::vector<uint8_t>* avcc)
{
	AnnexBToAvcc(annexB.data(), annexB.size(), avcc);
}

} // namespace mediasoup::media::h264
