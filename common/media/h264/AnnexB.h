#pragma once

#include <cstddef>
#include <cstdint>

namespace mediasoup::media::h264 {

struct NaluView {
	const uint8_t* data{nullptr};
	size_t size{0};
};

class AnnexBReader {
public:
	AnnexBReader(const uint8_t* data, size_t size);

	bool Next(NaluView* nalu);

private:
	const uint8_t* data_{nullptr};
	size_t size_{0};
	size_t offset_{0};
};

} // namespace mediasoup::media::h264
