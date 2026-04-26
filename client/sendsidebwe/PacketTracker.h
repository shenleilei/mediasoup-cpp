#pragma once

#include "PacketInfo.h"

#include "../ccutils/ProbeTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>

namespace mediasoup::plainclient::sendsidebwe {

class PacketTracker {
public:
	PacketTracker()
	{
		std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<uint32_t> dist(1u << 15, (1u << 15) + (1u << 14) - 1u);
		sequenceNumber_ = dist(rng);
	}

	uint16_t GetNextSequenceNumber()
	{
		const uint16_t sequence = static_cast<uint16_t>(sequenceNumber_);
		sequenceNumber_++;
		return sequence;
	}

	uint16_t RecordPacketSendAndGetSequenceNumber(
		int64_t atUs,
		size_t size,
		bool isRtx,
		mediasoup::ccutils::ProbeClusterId probeClusterId,
		bool isProbe)
	{
		const uint16_t sequence = GetNextSequenceNumber();
		RecordPacketSent(sequence, atUs, size, isRtx, probeClusterId, isProbe);
		return sequence;
	}

	void RecordPacketSent(
		uint16_t sequenceNumber,
		int64_t atUs,
		size_t size,
		bool isRtx,
		mediasoup::ccutils::ProbeClusterId probeClusterId,
		bool isProbe)
	{
		if (baseSendTimeUs_ == 0) {
			baseSendTimeUs_ = atUs;
		}
		uint64_t absoluteSeq = Unwrap(sequenceNumber);
		auto& packetInfo = packetInfos_[static_cast<size_t>(absoluteSeq & 0x1FFFu)];
		packetInfo = PacketInfo{
			absoluteSeq,
			atUs - baseSendTimeUs_,
			0,
			probeClusterId,
			static_cast<uint16_t>(size),
			isRtx,
			isProbe,
		};
		if (lastReceivedPacket_ == &packetInfo) {
			lastReceivedPacket_ = nullptr;
		}
		if (activeProbeClusterId_ != mediasoup::ccutils::ProbeClusterIdInvalid &&
			activeProbeClusterId_ == packetInfo.probeClusterId &&
			packetInfo.sequenceNumber > probeMaxSequenceNumber_) {
			probeMaxSequenceNumber_ = packetInfo.sequenceNumber;
		}
		if (sequenceNumber_ <= absoluteSeq) {
			sequenceNumber_ = absoluteSeq + 1;
		}
	}

	void SeedPacketForTest(
		uint16_t sequenceNumber,
		int64_t sendTimeUs,
		size_t size,
		bool isRtx = false,
		mediasoup::ccutils::ProbeClusterId probeClusterId = mediasoup::ccutils::ProbeClusterIdInvalid,
		bool isProbe = false)
	{
		if (baseSendTimeUs_ == 0) {
			baseSendTimeUs_ = sendTimeUs;
		}
		uint64_t absoluteSeq = Unwrap(sequenceNumber);
		auto& packetInfo = packetInfos_[static_cast<size_t>(absoluteSeq & 0x1FFFu)];
		packetInfo = PacketInfo{
			absoluteSeq,
			sendTimeUs - baseSendTimeUs_,
			0,
			probeClusterId,
			static_cast<uint16_t>(size),
			isRtx,
			isProbe,
		};
		if (sequenceNumber_ <= absoluteSeq) {
			sequenceNumber_ = absoluteSeq + 1;
		}
	}

	struct RemoteIndication {
		PacketInfo packetInfo;
		int64_t sendDeltaUs{ 0 };
		int64_t recvDeltaUs{ 0 };
		bool valid{ false };
	};

	RemoteIndication RecordPacketIndicationFromRemote(uint16_t sequenceNumber, int64_t recvTimeUs)
	{
		RemoteIndication indication;
		PacketInfo* packetInfo = GetPacketInfoExisting(sequenceNumber);
		if (!packetInfo) {
			return indication;
		}

		if (recvTimeUs == 0) {
			indication.packetInfo = *packetInfo;
			indication.valid = indication.packetInfo.sendTimeUs != 0;
			return indication;
		}

		if (baseRecvTimeUs_ == 0) {
			baseRecvTimeUs_ = recvTimeUs;
			lastReceivedPacket_ = packetInfo;
		}

		packetInfo->recvTimeUs = recvTimeUs - baseRecvTimeUs_;
		indication.packetInfo = *packetInfo;
		if (lastReceivedPacket_) {
			indication.sendDeltaUs = packetInfo->sendTimeUs - lastReceivedPacket_->sendTimeUs;
			indication.recvDeltaUs = packetInfo->recvTimeUs - lastReceivedPacket_->recvTimeUs;
		}
		lastReceivedPacket_ = packetInfo;
		indication.valid = indication.packetInfo.sendTimeUs != 0;
		return indication;
	}

	void ProbeClusterStarting(mediasoup::ccutils::ProbeClusterId probeClusterId)
	{
		activeProbeClusterId_ = probeClusterId;
	}

	void ProbeClusterDone(mediasoup::ccutils::ProbeClusterId probeClusterId)
	{
		if (activeProbeClusterId_ == probeClusterId) {
			activeProbeClusterId_ = mediasoup::ccutils::ProbeClusterIdInvalid;
		}
	}

	uint64_t ProbeMaxSequenceNumber() const
	{
		return probeMaxSequenceNumber_;
	}

	int64_t BaseSendTimeThreshold(int64_t thresholdUs, int64_t nowUs) const
	{
		if (baseSendTimeUs_ == 0) {
			return 0;
		}
		return nowUs - baseSendTimeUs_ - thresholdUs;
	}

private:
	PacketInfo* GetPacketInfoExisting(uint16_t sequenceNumber)
	{
		uint64_t absoluteSeq = Unwrap(sequenceNumber);
		auto& packetInfo = packetInfos_[static_cast<size_t>(absoluteSeq & 0x1FFFu)];
		if (static_cast<uint16_t>(packetInfo.sequenceNumber) == sequenceNumber) {
			return &packetInfo;
		}
		return nullptr;
	}

	uint64_t Unwrap(uint16_t seq)
	{
		if (!lastSeq_) {
			lastSeq_ = seq;
			unwrappedSeq_ = seq;
			return unwrappedSeq_;
		}
		int32_t diff = static_cast<int32_t>(seq) - static_cast<int32_t>(*lastSeq_);
		if (diff == 0) return unwrappedSeq_;
		if (diff > 0x8000) diff -= 0x10000;
		else if (diff < -0x8000) diff += 0x10000;
		
		unwrappedSeq_ += diff;
		lastSeq_ = seq;
		return unwrappedSeq_;
	}

	uint64_t sequenceNumber_{ 0 };
	int64_t baseSendTimeUs_{ 0 };
	std::array<PacketInfo, 8192> packetInfos_{};
	int64_t baseRecvTimeUs_{ 0 };
	PacketInfo* lastReceivedPacket_{ nullptr };
	mediasoup::ccutils::ProbeClusterId activeProbeClusterId_{ mediasoup::ccutils::ProbeClusterIdInvalid };
	uint64_t probeMaxSequenceNumber_{ 0 };
	std::optional<uint16_t> lastSeq_;
	uint64_t unwrappedSeq_{ 0 };
};

} // namespace mediasoup::plainclient::sendsidebwe
