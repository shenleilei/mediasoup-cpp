#pragma once

#include "UdpSendHelpers.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <limits>
#include <optional>
#include <unordered_map>
#include <vector>

namespace mediasoup::plainclient {

class SenderTransportController {
public:
	struct Config {
		int64_t pacingIntervalMs{ 2 };
		int64_t audioQueueDeadlineMs{ 40 };
		int64_t maxBudgetWindowMs{ 250 };
		size_t maxFreshVideoPacketsTotal{ 256 };
		size_t maxRetransmissionPackets{ 256 };
	};

	struct Metrics {
		std::array<uint64_t, 4> sentByClass{};
		std::array<uint64_t, 4> wouldBlockByClass{};
		std::array<uint64_t, 4> hardErrorByClass{};
		std::array<int, 4> lastHardErrorByClass{};
		uint64_t audioDeadlineDrops{ 0 };
		uint64_t queuedVideoRetentions{ 0 };
		uint64_t freshVideoQueueDrops{ 0 };
		uint64_t queuedVideoDiscards{ 0 };
		uint64_t retransmissionQueueDrops{ 0 };
		uint64_t retransmissionDrops{ 0 };
		uint64_t retransmissionSent{ 0 };
	};

	SenderTransportController()
		: SenderTransportController(Config{})
	{
	}

	explicit SenderTransportController(Config config)
		: config_(config)
	{
	}

	void SetSendFn(DatagramSendFn fn)
	{
		sendFn_ = std::move(fn);
	}

	void SetNowFn(std::function<int64_t()> fn)
	{
		nowFn_ = std::move(fn);
	}

	void SetOnVideoMediaSent(std::function<void(const uint8_t*, size_t)> fn)
	{
		onVideoMediaSent_ = std::move(fn);
	}

	void SetOnAudioSent(std::function<void(size_t, uint32_t)> fn)
	{
		onAudioSent_ = std::move(fn);
	}

	void SetOnVideoRetransmissionSent(std::function<void(uint32_t)> fn)
	{
		onVideoRetransmissionSent_ = std::move(fn);
	}

	void RegisterVideoTrack(uint32_t trackIndex, uint32_t ssrc, uint32_t targetBitrateBps = 0)
	{
		if (trackIndex >= trackStates_.size()) {
			trackStates_.resize(trackIndex + 1);
		}
		auto& trackState = trackStates_[trackIndex];
		trackState.trackIndex = trackIndex;
		trackState.ssrc = ssrc;
		trackState.targetBitrateBps =
			targetBitrateBps > 0 ? targetBitrateBps : kDefaultTrackTargetBitrateBps;
		ssrcToTrackIndex_[ssrc] = trackIndex;
		RefreshAggregateTargetBitrate();
	}

	void UpdateTrackTransportHint(uint32_t trackIndex, uint32_t ssrc, uint32_t targetBitrateBps, bool paused)
	{
		if (trackIndex >= trackStates_.size()) {
			trackStates_.resize(trackIndex + 1);
		}
		auto& trackState = trackStates_[trackIndex];
		trackState.trackIndex = trackIndex;
		trackState.ssrc = ssrc;
		trackState.targetBitrateBps = targetBitrateBps;
		trackState.paused = paused;
		ssrcToTrackIndex_[ssrc] = trackIndex;
		if (paused) {
			DropQueuedFreshVideoForTrack(ssrc);
			DropQueuedRetransmissionForTrack(ssrc);
		}
		RefreshAggregateTargetBitrate();
	}

	void SetTransportEstimatedBitrateBps(uint32_t estimatedBitrateBps)
	{
		transportEstimatedBitrateBps_ = estimatedBitrateBps;
		RefreshEffectivePacingBitrate();
	}

	void SetApplicationBitrateCapBps(uint32_t appBitrateCapBps)
	{
		applicationBitrateCapBps_ = appBitrateCapBps;
		RefreshEffectivePacingBitrate();
	}

	void SetTrackPaused(uint32_t ssrc, bool paused)
	{
		auto it = ssrcToTrackIndex_.find(ssrc);
		if (it == ssrcToTrackIndex_.end()) {
			return;
		}
		auto& trackState = trackStates_[it->second];
		trackState.paused = paused;
		if (paused) {
			DropQueuedFreshVideoForTrack(ssrc);
			DropQueuedRetransmissionForTrack(ssrc);
		}
		RefreshAggregateTargetBitrate();
	}

	bool EnqueueVideoMediaPacket(uint32_t ssrc, const uint8_t* data, size_t len)
	{
		auto* trackState = GetTrackStateBySsrc(ssrc);
		if (!trackState || trackState->paused) {
			return false;
		}
		if (queuedFreshVideoPackets_ >= config_.maxFreshVideoPacketsTotal) {
			metrics_.freshVideoQueueDrops++;
			return false;
		}
		if (!EnqueuePacket(trackState->videoQueue, ssrc, data, len)) {
			return false;
		}
		queuedFreshVideoPackets_++;
		return true;
	}

	bool EnqueueVideoRetransmissionPacket(uint32_t ssrc, const uint8_t* data, size_t len)
	{
		if (videoRetransmissionQueue_.size() >= config_.maxRetransmissionPackets) {
			metrics_.retransmissionQueueDrops++;
			return false;
		}
		return EnqueuePacket(videoRetransmissionQueue_, ssrc, data, len);
	}

	bool EnqueueAudioPacket(uint32_t ssrc, uint32_t rtpTimestamp, const uint8_t* data, size_t len)
	{
		if (len > kMaxPacketBytes || !data) {
			return false;
		}
		AudioPacket packet;
		packet.ssrc = ssrc;
		packet.rtpTimestamp = rtpTimestamp;
		packet.deadlineMs = NowMs() + config_.audioQueueDeadlineMs;
		std::memcpy(packet.data, data, len);
		packet.len = len;
		packet.transportMetadata.retryable = true;
		audioQueue_.push_back(std::move(packet));
		return true;
	}

	SendResult SendControlPacket(const uint8_t* data, size_t len)
	{
		if (!sendFn_) {
			return {SendStatus::HardError, EINVAL, 0};
		}
		PacketTransportMetadata transportMetadata;
		const auto result = sendFn_(PacketClass::Control, &transportMetadata, data, len);
		RecordSendResult(PacketClass::Control, result);
		return result;
	}

	void OnPacingTick(int64_t nowMs)
	{
		AdvanceBudget(nowMs);
		DropExpiredAudioPackets(nowMs);
		FlushAudioQueue();
		FlushRetransmissionQueue();
		FlushFreshVideoQueues();
	}

	void FlushForShutdown(int64_t nowMs)
	{
		// Try to drain pending packets on shutdown before dropping leftovers.
		// Use an effectively unbounded budget so we are not limited by pacing target.
		constexpr size_t kMaxDrainPasses = 64;
		for (size_t pass = 0; pass < kMaxDrainPasses; ++pass) {
			const size_t pendingBefore = PendingPacketCount();
			if (pendingBefore == 0) {
				break;
			}
			DropExpiredAudioPackets(nowMs);
			mediaBudgetBytes_ = std::numeric_limits<int64_t>::max() / 4;
			FlushAudioQueue();
			FlushRetransmissionQueue();
			FlushFreshVideoQueues();
			if (PendingPacketCount() >= pendingBefore) {
				break;
			}
		}

		DropExpiredAudioPackets(nowMs);
		ClearQueues();
	}

	void DropQueuedFreshVideoForTrack(uint32_t ssrc)
	{
		auto* trackState = GetTrackStateBySsrc(ssrc);
		if (!trackState) {
			return;
		}
		const size_t dropped = trackState->videoQueue.size();
		metrics_.queuedVideoDiscards += dropped;
		trackState->videoQueue.clear();
		queuedFreshVideoPackets_ = dropped > queuedFreshVideoPackets_
			? 0
			: (queuedFreshVideoPackets_ - dropped);
	}

	void DropQueuedRetransmissionForTrack(uint32_t ssrc)
	{
		const auto previousSize = videoRetransmissionQueue_.size();
		videoRetransmissionQueue_.erase(
			std::remove_if(
				videoRetransmissionQueue_.begin(),
				videoRetransmissionQueue_.end(),
				[ssrc](const auto& packet) { return packet.ssrc == ssrc; }),
			videoRetransmissionQueue_.end());
		const size_t dropped = previousSize - videoRetransmissionQueue_.size();
		metrics_.retransmissionDrops += dropped;
	}

	void ClearQueuedProbeClusterAssociation(uint32_t probeClusterId)
	{
		if (probeClusterId == 0) {
			return;
		}

		for (auto& packet : audioQueue_) {
			ClearProbeClusterAssociation(packet.transportMetadata, probeClusterId);
		}
		for (auto& packet : videoRetransmissionQueue_) {
			ClearProbeClusterAssociation(packet.transportMetadata, probeClusterId);
		}
		for (auto& trackState : trackStates_) {
			for (auto& packet : trackState.videoQueue) {
				ClearProbeClusterAssociation(packet.transportMetadata, probeClusterId);
			}
		}
	}

	size_t QueuedFreshVideoPackets() const
	{
		return queuedFreshVideoPackets_;
	}

	size_t QueuedAudioPackets() const
	{
		return audioQueue_.size();
	}

	size_t QueuedRetransmissionPackets() const
	{
		return videoRetransmissionQueue_.size();
	}

	int64_t MediaBudgetBytes() const
	{
		return mediaBudgetBytes_;
	}

	uint32_t AggregateTargetBitrateBps() const
	{
		return aggregateTargetBitrateBps_;
	}

	uint32_t EffectivePacingBitrateBps() const
	{
		return effectivePacingBitrateBps_;
	}

	const Metrics& GetMetrics() const
	{
		return metrics_;
	}

private:
	static constexpr size_t kMaxPacketBytes = 1500;
	static constexpr uint32_t kDefaultTrackTargetBitrateBps = 900000;

	struct Packet {
		uint32_t ssrc{ 0 };
		uint8_t data[kMaxPacketBytes]{};
		size_t len{ 0 };
		PacketTransportMetadata transportMetadata{};
	};

	struct AudioPacket : public Packet {
		uint32_t rtpTimestamp{ 0 };
		int64_t deadlineMs{ 0 };
	};

	struct TrackState {
		uint32_t trackIndex{ 0 };
		uint32_t ssrc{ 0 };
		uint32_t targetBitrateBps{ 0 };
		bool paused{ false };
		std::deque<Packet> videoQueue;
	};

	bool EnqueuePacket(std::deque<Packet>& queue, uint32_t ssrc, const uint8_t* data, size_t len)
	{
		if (len > kMaxPacketBytes || !data) {
			return false;
		}
		Packet packet;
		packet.ssrc = ssrc;
		std::memcpy(packet.data, data, len);
		packet.len = len;
		packet.transportMetadata.retryable = true;
		queue.push_back(std::move(packet));
		return true;
	}

	void AdvanceBudget(int64_t nowMs)
	{
		if (lastBudgetUpdateMs_ == 0) {
			lastBudgetUpdateMs_ = nowMs;
			return;
		}
		const int64_t deltaMs = std::max<int64_t>(0, nowMs - lastBudgetUpdateMs_);
		lastBudgetUpdateMs_ = nowMs;
		if (deltaMs == 0 || effectivePacingBitrateBps_ == 0) {
			return;
		}
		const int64_t addedBytes =
			static_cast<int64_t>(effectivePacingBitrateBps_) * deltaMs / 8000;
		const int64_t maxBudget = std::max<int64_t>(
			static_cast<int64_t>(kMaxPacketBytes),
			static_cast<int64_t>(effectivePacingBitrateBps_) * config_.maxBudgetWindowMs / 8000);
		mediaBudgetBytes_ = std::min(maxBudget, mediaBudgetBytes_ + addedBytes);
	}

	void RefreshAggregateTargetBitrate()
	{
		uint64_t total = 0;
		for (const auto& trackState : trackStates_) {
			if (!trackState.paused) {
				total += trackState.targetBitrateBps;
			}
		}
		aggregateTargetBitrateBps_ = static_cast<uint32_t>(
			std::min<uint64_t>(total, std::numeric_limits<uint32_t>::max()));
		RefreshEffectivePacingBitrate();
	}

	void RefreshEffectivePacingBitrate()
	{
		uint32_t effective = aggregateTargetBitrateBps_;
		if (transportEstimatedBitrateBps_ > 0) {
			effective = std::min(effective, transportEstimatedBitrateBps_);
		}
		if (applicationBitrateCapBps_ > 0) {
			effective = std::min(effective, applicationBitrateCapBps_);
		}
		effectivePacingBitrateBps_ = effective;
	}

	void DropExpiredAudioPackets(int64_t nowMs)
	{
		while (!audioQueue_.empty() && audioQueue_.front().deadlineMs <= nowMs) {
			audioQueue_.pop_front();
			metrics_.audioDeadlineDrops++;
		}
	}

	void FlushAudioQueue()
	{
			while (!audioQueue_.empty()) {
				auto& packet = audioQueue_.front();
				const auto result = SendPacket(PacketClass::AudioRtp, packet);
				if (result.status == SendStatus::Sent) {
					mediaBudgetBytes_ -= static_cast<int64_t>(packet.len);
					if (onAudioSent_) {
					onAudioSent_(packet.len > 12 ? packet.len - 12 : 0, packet.rtpTimestamp);
				}
				audioQueue_.pop_front();
				continue;
			}
			if (result.status == SendStatus::HardError) {
				audioQueue_.pop_front();
			}
			break;
		}
	}

	void FlushRetransmissionQueue()
	{
			while (!videoRetransmissionQueue_.empty()) {
				auto& packet = videoRetransmissionQueue_.front();
				const auto result = SendPacket(PacketClass::VideoRetransmission, packet);
				if (result.status == SendStatus::Sent) {
					mediaBudgetBytes_ -= static_cast<int64_t>(packet.len);
					metrics_.retransmissionSent++;
				if (onVideoRetransmissionSent_) {
					onVideoRetransmissionSent_(packet.ssrc);
				}
				videoRetransmissionQueue_.pop_front();
				continue;
			}
			if (result.status == SendStatus::HardError) {
				videoRetransmissionQueue_.pop_front();
				metrics_.retransmissionDrops++;
			}
			break;
		}
	}

	void FlushFreshVideoQueues()
	{
		if (trackStates_.empty()) {
			return;
		}

		while (mediaBudgetBytes_ > 0) {
			bool sentPacket = false;
			const size_t trackCount = trackStates_.size();
			for (size_t attempt = 0; attempt < trackCount; ++attempt) {
				const size_t index = (nextVideoTrackIndex_ + attempt) % trackCount;
				auto& trackState = trackStates_[index];
				if (trackState.paused || trackState.videoQueue.empty()) {
					continue;
				}
				auto& packet = trackState.videoQueue.front();
					if (static_cast<int64_t>(packet.len) > mediaBudgetBytes_) {
						continue;
					}
					const auto result = SendPacket(PacketClass::VideoMedia, packet);
					if (result.status == SendStatus::Sent) {
						mediaBudgetBytes_ -= static_cast<int64_t>(packet.len);
						if (onVideoMediaSent_) {
						onVideoMediaSent_(packet.data, packet.len);
					}
					trackState.videoQueue.pop_front();
					if (queuedFreshVideoPackets_ > 0) {
						queuedFreshVideoPackets_--;
					}
					nextVideoTrackIndex_ = (index + 1) % trackCount;
					sentPacket = true;
					break;
				}
				if (result.status == SendStatus::HardError) {
					trackState.videoQueue.pop_front();
					if (queuedFreshVideoPackets_ > 0) {
						queuedFreshVideoPackets_--;
					}
					metrics_.queuedVideoDiscards++;
					nextVideoTrackIndex_ = (index + 1) % trackCount;
					sentPacket = true;
					break;
				}
				if (result.status == SendStatus::WouldBlock) {
					metrics_.queuedVideoRetentions++;
				}
				return;
			}

			if (!sentPacket) {
				break;
			}
		}
	}

		SendResult SendPacket(PacketClass packetClass, Packet& packet)
		{
			if (!sendFn_) {
				return {SendStatus::HardError, EINVAL, 0};
			}
			const auto result =
				sendFn_(packetClass, &packet.transportMetadata, packet.data, packet.len);
			RecordSendResult(packetClass, result);
			return result;
		}

	void RecordSendResult(PacketClass packetClass, const SendResult& result)
	{
		const size_t index = PacketClassIndex(packetClass);
		switch (result.status) {
			case SendStatus::Sent:
				metrics_.sentByClass[index]++;
				break;
			case SendStatus::WouldBlock:
				metrics_.wouldBlockByClass[index]++;
				break;
			case SendStatus::HardError:
				metrics_.hardErrorByClass[index]++;
				metrics_.lastHardErrorByClass[index] = result.error;
				break;
		}
	}

	TrackState* GetTrackStateBySsrc(uint32_t ssrc)
	{
		auto it = ssrcToTrackIndex_.find(ssrc);
		if (it == ssrcToTrackIndex_.end() || it->second >= trackStates_.size()) {
			return nullptr;
		}
		return &trackStates_[it->second];
	}

	int64_t NowMs() const
	{
		return nowFn_ ? nowFn_() : 0;
	}

	static void ClearProbeClusterAssociation(
		PacketTransportMetadata& transportMetadata,
		uint32_t probeClusterId)
	{
		if (transportMetadata.isProbe) {
			return;
		}
		if (transportMetadata.probeClusterId == probeClusterId) {
			transportMetadata.probeClusterId = 0;
		}
	}

	size_t PendingPacketCount() const
	{
		return queuedFreshVideoPackets_ + videoRetransmissionQueue_.size() + audioQueue_.size();
	}

	void ClearQueues()
	{
		queuedFreshVideoPackets_ = 0;
		for (auto& trackState : trackStates_) {
			metrics_.queuedVideoDiscards += trackState.videoQueue.size();
			trackState.videoQueue.clear();
		}
		metrics_.audioDeadlineDrops += audioQueue_.size();
		audioQueue_.clear();
		metrics_.retransmissionDrops += videoRetransmissionQueue_.size();
		videoRetransmissionQueue_.clear();
	}

	Config config_;
	DatagramSendFn sendFn_;
	std::function<int64_t()> nowFn_;
	std::function<void(const uint8_t*, size_t)> onVideoMediaSent_;
	std::function<void(size_t, uint32_t)> onAudioSent_;
	std::function<void(uint32_t)> onVideoRetransmissionSent_;
	std::vector<TrackState> trackStates_;
	std::unordered_map<uint32_t, size_t> ssrcToTrackIndex_;
	std::deque<AudioPacket> audioQueue_;
	std::deque<Packet> videoRetransmissionQueue_;
	size_t queuedFreshVideoPackets_{ 0 };
	int64_t mediaBudgetBytes_{ 0 };
	int64_t lastBudgetUpdateMs_{ 0 };
	uint32_t aggregateTargetBitrateBps_{ 0 };
	uint32_t transportEstimatedBitrateBps_{ 0 };
	uint32_t applicationBitrateCapBps_{ 0 };
	uint32_t effectivePacingBitrateBps_{ 0 };
	size_t nextVideoTrackIndex_{ 0 };
	Metrics metrics_;
};

} // namespace mediasoup::plainclient
