#include "media/h264/Avcc.h"

#include "media/h264/AnnexB.h"

namespace mediasoup::media::h264 {

void AnnexBToAvcc(const uint8_t* annexB, size_t annexBSize, std::vector<uint8_t>* avcc)
{
	if (!avcc) return;

	avcc->clear();
	if (!annexB || annexBSize == 0) return;

	AnnexBReader reader(annexB, annexBSize);
	NaluView nalu;
	while (reader.Next(&nalu)) {
		const uint32_t nalLen = static_cast<uint32_t>(nalu.size);
		avcc->push_back(static_cast<uint8_t>((nalLen >> 24) & 0xFF));
		avcc->push_back(static_cast<uint8_t>((nalLen >> 16) & 0xFF));
		avcc->push_back(static_cast<uint8_t>((nalLen >> 8) & 0xFF));
		avcc->push_back(static_cast<uint8_t>(nalLen & 0xFF));
		avcc->insert(avcc->end(), nalu.data, nalu.data + nalu.size);
	}
}

} // namespace mediasoup::media::h264
