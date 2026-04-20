#include "media/h264/AnnexB.h"

namespace mediasoup::media::h264 {
namespace {

size_t FindStartCode(const uint8_t* data, size_t size, size_t offset, size_t* startCodeSize)
{
	for (size_t i = offset; i < size; ++i) {
		if (i + 3 <= size &&
			data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
			if (startCodeSize) *startCodeSize = 3;
			return i;
		}
		if (i + 4 <= size &&
			data[i] == 0 && data[i + 1] == 0 &&
			data[i + 2] == 0 && data[i + 3] == 1) {
			if (startCodeSize) *startCodeSize = 4;
			return i;
		}
	}

	if (startCodeSize) *startCodeSize = 0;
	return size;
}

} // namespace

AnnexBReader::AnnexBReader(const uint8_t* data, size_t size)
	: data_(data)
	, size_(size)
{
}

bool AnnexBReader::Next(NaluView* nalu)
{
	if (!nalu || !data_ || size_ == 0) return false;

	while (offset_ < size_) {
		size_t startCodeSize = 0;
		size_t start = FindStartCode(data_, size_, offset_, &startCodeSize);
		if (start == size_) {
			offset_ = size_;
			return false;
		}

		size_t nalStart = start + startCodeSize;
		while (nalStart < size_) {
			size_t extraStartCodeSize = 0;
			const size_t extraStart = FindStartCode(data_, size_, nalStart, &extraStartCodeSize);
			if (extraStart != nalStart) break;
			start = extraStart;
			nalStart = start + extraStartCodeSize;
		}
		if (nalStart >= size_) {
			offset_ = size_;
			return false;
		}

		size_t nextStartCodeSize = 0;
		const size_t nextStart = FindStartCode(data_, size_, nalStart, &nextStartCodeSize);
		(void)nextStartCodeSize;
		offset_ = nextStart;

		if (nextStart > nalStart) {
			nalu->data = data_ + nalStart;
			nalu->size = nextStart - nalStart;
			return true;
		}

		offset_ = nalStart + 1;
	}

	return false;
}

} // namespace mediasoup::media::h264
