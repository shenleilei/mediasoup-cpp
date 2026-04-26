// NetworkThread.h — Dedicated network thread owning UDP socket, RTP/RTCP state
// Phase 2 of linux-client-multi-source-thread-model migration.
#pragma once

#include "RtcpHandler.h"
#include "SenderTransportController.h"
#include "TransportCcHelpers.h"
#include "ThreadTypes.h"
#include "ccutils/Prober.h"
#include "sendsidebwe/SendSideBwe.h"
#include "sendsidebwe/ProbeObserver.h"
#include "media/rtp/H264Packetizer.h"

#include <arpa/inet.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>

#include <atomic>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

// ═══════════════════════════════════════════════════════════
// Shared H264 packetization now lives in common/media; this file still owns
// the local RTP header helper used by non-H264 send paths.
// ═══════════════════════════════════════════════════════════

inline void rtpHeader(uint8_t* buf, uint8_t pt, uint16_t seq, uint32_t ts, uint32_t ssrc, bool marker) {
	buf[0] = 0x80;
	buf[1] = (marker ? 0x80 : 0) | (pt & 0x7F);
	buf[2] = seq >> 8; buf[3] = seq & 0xFF;
	buf[4] = ts >> 24; buf[5] = ts >> 16; buf[6] = ts >> 8; buf[7] = ts;
	buf[8] = ssrc >> 24; buf[9] = ssrc >> 16; buf[10] = ssrc >> 8; buf[11] = ssrc;
}

// ═══════════════════════════════════════════════════════════
// Per-track network state (owned exclusively by network thread)
// ═══════════════════════════════════════════════════════════
struct TrackNetState {
	uint32_t trackIndex = 0;
	uint32_t ssrc = 0;
	uint8_t payloadType = 0;
	uint8_t transportCcExtensionId = 0;
	uint16_t seq = 0;
	bool paused = false;
	int64_t lastPliForwardedMs = 0;
	uint64_t configGeneration = 0;
	static constexpr int64_t kPliDebounceMs = 1000;
	VideoStreamRtcpState rtcpState;
};

// ═══════════════════════════════════════════════════════════
// NetworkThread: owns UDP socket, all RTP/RTCP state
// ═══════════════════════════════════════════════════════════
class NetworkThread
	: public mediasoup::ccutils::ProberListener
	, public mediasoup::plainclient::sendsidebwe::ProbeObserverListener {
public:
	static constexpr uint32_t kDefaultMinTransportEstimateBps = 100000u;
	static constexpr uint32_t kDefaultMaxTransportEstimateBps = std::numeric_limits<uint32_t>::max();
	static constexpr int64_t kDefaultProbeOveragePct = 120;
	static constexpr int64_t kDefaultProbeMinBps = 200000;

	struct Config {
		int udpFd = -1;                // pre-created connected UDP socket
		uint32_t audioSsrc = 0;
		uint8_t audioPt = 0;
		uint8_t audioTransportCcExtensionId = 0;
		uint32_t applicationBitrateCapBps = 0;
		bool enableTransportController = true;
		bool enableTransportEstimate = true;
		uint32_t transportEstimateMinBps = kDefaultMinTransportEstimateBps;
		uint32_t transportEstimateMaxBps = kDefaultMaxTransportEstimateBps;
		int64_t probeOveragePct = kDefaultProbeOveragePct;
		int64_t probeMinBps = kDefaultProbeMinBps;
		std::optional<mediasoup::plainclient::sendsidebwe::CongestionDetectorConfig> sendSideBweConfig;
	};

	// Per-source input queue (one per source worker)
	struct SourceInput {
		mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity>* auQueue = nullptr;
		mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity>* keyframeQueue = nullptr;
	};

	// Stats output queue (network -> control)
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity>* statsQueue = nullptr;

	// Control -> network command queue (§10.6)
	mt::SpscQueue<mt::NetworkControlCommand, mt::kControlCommandQueueCapacity>* controlQueue = nullptr;

	// Network -> control command ack queue
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity>* commandAckQueue = nullptr;

	// Audio AU input queue (optional, for threaded audio path)
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity>* audioAuQueue = nullptr;

	explicit NetworkThread(const Config& cfg)
		: cfg_(cfg)
		, useTransportController_(cfg.enableTransportController)
		, useTransportEstimate_(cfg.enableTransportEstimate)
		, sendSideBwe_(cfg.sendSideBweConfig.has_value()
			? mediasoup::plainclient::sendsidebwe::SendSideBwe(*cfg.sendSideBweConfig)
			: mediasoup::plainclient::sendsidebwe::SendSideBwe())
	{
		rtcp_.audioSsrc = cfg.audioSsrc;
		transportController_.SetSendFn([this](
			mediasoup::plainclient::PacketClass packetClass,
			mediasoup::plainclient::PacketTransportMetadata* transportMetadata,
			const uint8_t* data,
			size_t len) {
			return sendMediaPacketWithTransportCc(packetClass, transportMetadata, data, len);
		});
		transportController_.SetNowFn([this] { return steadyNowMs(); });
		transportController_.SetOnVideoMediaSent([this](const uint8_t* packet, size_t packetLen) {
			rtcp_.onVideoRtpSent(packet, packetLen);
		});
		transportController_.SetOnAudioSent([this](size_t payloadLen, uint32_t rtpTs) {
			rtcp_.onAudioRtpSent(payloadLen, rtpTs);
		});
		transportController_.SetOnVideoRetransmissionSent([this](uint32_t ssrc) {
			rtcp_.nackRetransmitted++;
			if (auto* stream = rtcp_.getVideoStream(ssrc)) {
				stream->nackRetransmitted++;
			}
		});
		transportController_.SetApplicationBitrateCapBps(cfg.applicationBitrateCapBps);
		prober_.SetListener(this);
		probeObserver_.SetListener(this);
	}

	~NetworkThread() { stop(); }

	void registerVideoTrack(
		uint32_t trackIndex,
		uint32_t ssrc,
		uint8_t pt,
		uint8_t transportCcExtensionId = 0) {
		TrackNetState t;
		t.trackIndex = trackIndex;
		t.ssrc = ssrc;
		t.payloadType = pt;
		t.transportCcExtensionId = transportCcExtensionId;
		t.rtcpState.ssrc = ssrc;
		t.rtcpState.payloadType = pt;
		tracks_.push_back(std::move(t));
		// Note: rtcp_ registration deferred to start() to avoid dangling pointers from vector realloc
		if (useTransportController_) {
			transportController_.RegisterVideoTrack(trackIndex, ssrc);
			RefreshPublishedTransportEstimate();
		}
	}

	void addSourceInput(SourceInput input) { sourceInputs_.push_back(input); }

	void sendComediaProbe() {
		for (auto& t : tracks_) {
			for (int i = 0; i < 5; ++i) {
				uint8_t dummy[13];
				rtpHeader(dummy, t.payloadType, t.seq++, 0, t.ssrc, false);
				dummy[12] = 0;
				if (useTransportController_) {
					(void)transportController_.SendControlPacket(dummy, 13);
				} else {
					(void)mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, dummy, 13);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
			}
		}
	}

	void start() {
		if (running_.load()) return;
		for (auto& t : tracks_)
			rtcp_.registerVideoStream(t.ssrc, t.payloadType, &t.seq);
		// Create epoll, eventfd, timerfds
		epollFd_ = epoll_create1(0);
		wakeupFd_ = eventfd(0, EFD_NONBLOCK);
		srTimerFd_ = createTimerFd(1000);   // SR every 1s
		statsTimerFd_ = createTimerFd(500); // stats every 500ms
		pacingTimerFd_ = createTimerFd(kPacingIntervalMs); // pacing tick
		epollAdd(cfg_.udpFd, EPOLLIN);
		epollAdd(wakeupFd_, EPOLLIN);
		epollAdd(srTimerFd_, EPOLLIN);
		epollAdd(statsTimerFd_, EPOLLIN);
		epollAdd(pacingTimerFd_, EPOLLIN);
		running_ = true;
		thread_ = std::thread([this]() { loop(); });
	}

	void stop() {
		running_ = false;
		if (wakeupFd_ >= 0) { uint64_t v = 1; ::write(wakeupFd_, &v, sizeof(v)); }
		if (thread_.joinable()) thread_.join();
		prober_.Reset();
		if (useTransportController_) {
			transportController_.FlushForShutdown(steadyNowMs());
		} else {
			// Flush remaining paced packets before closing
			while (!pacingQueue_.empty()) pacingFlush();
		}
		if (epollFd_ >= 0) { ::close(epollFd_); epollFd_ = -1; }
		if (wakeupFd_ >= 0) { ::close(wakeupFd_); wakeupFd_ = -1; }
		if (srTimerFd_ >= 0) { ::close(srTimerFd_); srTimerFd_ = -1; }
		if (statsTimerFd_ >= 0) { ::close(statsTimerFd_); statsTimerFd_ = -1; }
		if (pacingTimerFd_ >= 0) { ::close(pacingTimerFd_); pacingTimerFd_ = -1; }
	}

	// Call from producer threads after pushing AU to wake network thread
	void wakeup() {
		if (wakeupFd_ >= 0) { uint64_t v = 1; ::write(wakeupFd_, &v, sizeof(v)); }
	}

	void OnProbeClusterSwitch(const mediasoup::ccutils::ProbeClusterInfo& info) override
	{
		{
			std::lock_guard<std::mutex> lock(probeEventMutex_);
			probeEvents_.push_back(ProbeEvent{ProbeEventType::StartCluster, info, mediasoup::ccutils::ProbeClusterIdInvalid});
		}
		wakeup();
	}

	void OnSendProbe(int bytesToSend) override
	{
		pendingProbeBytes_.fetch_add(bytesToSend);
		wakeup();
	}

	void OnProbeObserverClusterComplete(mediasoup::ccutils::ProbeClusterId clusterId) override
	{
		{
			std::lock_guard<std::mutex> lock(probeEventMutex_);
			probeEvents_.push_back(ProbeEvent{ProbeEventType::CompleteCluster, mediasoup::ccutils::ProbeClusterInfoInvalid, clusterId});
		}
		wakeup();
	}

	bool running() const { return running_.load(); }

	// Query track stats (called from network thread internally, pushed to statsQueue)
	void pushStatsSnapshot(const TrackNetState& t) {
		if (!statsQueue) return;
		auto* stream = rtcp_.getVideoStream(t.ssrc);
		if (!stream) return;
		mt::SenderStatsSnapshot snap;
		snap.timestampMs = steadyNowMs();
		snap.trackIndex = t.trackIndex;
		snap.ssrc = t.ssrc;
		snap.generation = ++statsGeneration_;
		snap.packetCount = stream->packetCount;
		snap.octetCount = stream->octetCount;
		snap.rrCumulativeLost = stream->rrCumulativeLost;
		snap.rrLossFraction = stream->rrLossFraction;
		snap.rrRttMs = stream->rrRttMs;
		snap.rrJitterMs = stream->rrJitterMs;
		snap.lastRrReceivedMs = stream->lastRrReceivedMs;
		snap.probePacketCount = stream->probePacketCount;
		snap.probeActive =
			probeObserver_.ActiveClusterId() != mediasoup::ccutils::ProbeClusterIdInvalid;
		snap.senderUsageBitrateBps = CurrentSenderUsageBitrateBps();
		snap.transportEstimatedBitrateBps = transportEstimatedBitrateBps_;
		snap.effectivePacingBitrateBps = effectivePacingBitrateBps();
		snap.transportCcFeedbackReports = transportCcFeedbackReports_;
		const auto& transportMetrics = transportController_.GetMetrics();
		snap.transportWouldBlockTotal =
			std::accumulate(
				transportMetrics.wouldBlockByClass.begin(),
				transportMetrics.wouldBlockByClass.end(),
				uint64_t{0});
		snap.queuedVideoRetentions = transportMetrics.queuedVideoRetentions;
		snap.audioDeadlineDrops = transportMetrics.audioDeadlineDrops;
		snap.retransmissionDrops = transportMetrics.retransmissionDrops;
		snap.retransmissionSent = transportMetrics.retransmissionSent;
		snap.probeClusterStartCount = probeClusterStartCount_;
		snap.probeClusterCompleteCount = probeClusterCompleteCount_;
		snap.probeClusterEarlyStopCount = probeClusterEarlyStopCount_;
		snap.probeBytesSent = probeBytesSent_;
		snap.queuedFreshVideoPackets = static_cast<uint32_t>(queuedFreshVideoPackets());
		snap.queuedAudioPackets = static_cast<uint32_t>(queuedAudioPackets());
		snap.queuedRetransmissionPackets = static_cast<uint32_t>(queuedRetransmissionPackets());
		statsQueue->tryPush(std::move(snap));
	}

private:
	static int64_t steadyNowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	static int64_t steadyNowUs() {
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	static bool isTransportCcPacketClass(mediasoup::plainclient::PacketClass packetClass) {
		return
			packetClass == mediasoup::plainclient::PacketClass::AudioRtp ||
			packetClass == mediasoup::plainclient::PacketClass::VideoMedia ||
			packetClass == mediasoup::plainclient::PacketClass::VideoRetransmission;
	}

	uint8_t transportCcExtensionIdForSsrc(uint32_t ssrc) const
	{
		if (ssrc == cfg_.audioSsrc) {
			return cfg_.audioTransportCcExtensionId;
		}

		for (const auto& track : tracks_) {
			if (track.ssrc == ssrc) {
				return track.transportCcExtensionId;
			}
		}

		return 0;
	}

	mediasoup::plainclient::SendResult sendMediaPacketWithTransportCc(
		mediasoup::plainclient::PacketClass packetClass,
		mediasoup::plainclient::PacketTransportMetadata* transportMetadata,
		const uint8_t* data,
		size_t len)
	{
		if (!isTransportCcPacketClass(packetClass) || !data || len < 12) {
			return SendUdpPacket(data, len);
		}

		const uint32_t ssrc =
			(static_cast<uint32_t>(data[8]) << 24) |
			(static_cast<uint32_t>(data[9]) << 16) |
			(static_cast<uint32_t>(data[10]) << 8) |
			static_cast<uint32_t>(data[11]);
		const uint8_t extensionId = transportCcExtensionIdForSsrc(ssrc);
		if (extensionId == 0) {
			return SendUdpPacket(data, len);
		}

		uint8_t rewrittenPacket[1600];
		size_t rewrittenLen = sizeof(rewrittenPacket);
		const bool isRtx =
			packetClass == mediasoup::plainclient::PacketClass::VideoRetransmission;
		const bool isProbe = transportMetadata && transportMetadata->isProbe;
		const auto activeProbeClusterId = probeObserver_.ActiveClusterId();
		mediasoup::ccutils::ProbeClusterId probeClusterId =
			transportMetadata
				? static_cast<mediasoup::ccutils::ProbeClusterId>(
					transportMetadata->probeClusterId)
				: mediasoup::ccutils::ProbeClusterIdInvalid;
		uint16_t transportSequence = 0;
		if (transportMetadata && transportMetadata->retryable &&
			transportMetadata->hasTransportSequence) {
			transportSequence = transportMetadata->transportSequence;
		} else {
			if (transportMetadata) {
				if (isProbe) {
					probeClusterId = static_cast<mediasoup::ccutils::ProbeClusterId>(
						transportMetadata->probeClusterId);
				} else if (
					packetClass != mediasoup::plainclient::PacketClass::AudioRtp &&
					activeProbeClusterId != mediasoup::ccutils::ProbeClusterIdInvalid) {
					probeClusterId = activeProbeClusterId;
					transportMetadata->probeClusterId =
						static_cast<uint32_t>(activeProbeClusterId);
				} else {
					probeClusterId = mediasoup::ccutils::ProbeClusterIdInvalid;
					transportMetadata->probeClusterId = 0;
				}
			}
			transportSequence = sendSideBwe_.GetNextSequenceNumber();
			if (transportMetadata && transportMetadata->retryable) {
				transportMetadata->hasTransportSequence = true;
				transportMetadata->transportSequence = transportSequence;
			}
		}
		if (!mediasoup::plainclient::RewriteRtpWithTransportCcSequence(
				data,
				len,
				extensionId,
				transportSequence,
				rewrittenPacket,
					&rewrittenLen)) {
			transportCcRewriteFailures_++;
			return SendUdpPacket(data, len);
		}

		const auto result = SendUdpPacket(rewrittenPacket, rewrittenLen);
		if (result.status == mediasoup::plainclient::SendStatus::Sent) {
			sendSideBwe_.RecordPacketSent(
				transportSequence,
				steadyNowUs(),
				rewrittenLen,
				isRtx,
				probeClusterId,
				isProbe);
			transportCcRewrittenPacketsSent_++;
			if (!isProbe) {
				RecordSenderUsageSample(steadyNowUs(), rewrittenLen);
			}
			if (probeClusterId != mediasoup::ccutils::ProbeClusterIdInvalid) {
				probeObserver_.RecordPacket(
					rewrittenLen,
					isRtx,
					probeClusterId,
					isProbe);
			}
		}
		return result;
	}

	void sendH264(const uint8_t* data, int size, uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq) {
		class NetworkPacingSink final : public mediasoup::media::rtp::H264PacketSink {
		public:
			NetworkPacingSink(NetworkThread* owner, uint32_t sinkSsrc)
				: owner_(owner)
				, ssrc_(sinkSsrc)
			{
			}

			void OnPacket(const uint8_t* packet, size_t packetLen) override {
				if (owner_->useTransportController_) {
					(void)owner_->transportController_.EnqueueVideoMediaPacket(ssrc_, packet, packetLen);
				} else {
					owner_->pacingEnqueue(packet, packetLen, ssrc_);
				}
			}

		private:
			NetworkThread* owner_{nullptr};
			uint32_t ssrc_{0};
		};

		NetworkPacingSink sink(this, ssrc);
		mediasoup::media::rtp::H264Packetizer::PacketizeAnnexB(
			data,
			static_cast<size_t>(std::max(0, size)),
			pt,
			ts,
			ssrc,
			&seq,
			&sink);
	}

	void sendOpus(const uint8_t* data, int size, uint8_t pt, uint32_t ts, uint32_t ssrc, uint16_t& seq) {
		if (size <= 0 || size > 4096) return;
		uint8_t pkt[12 + 4096];
		rtpHeader(pkt, pt, seq++, ts, ssrc, true);
		memcpy(pkt + 12, data, size);
			if (useTransportController_) {
				(void)transportController_.EnqueueAudioPacket(ssrc, ts, pkt, 12 + size);
		} else {
			const auto result = sendMediaPacketWithTransportCc(
				mediasoup::plainclient::PacketClass::AudioRtp,
				nullptr,
				pkt,
				12 + size);
			if (result.status == mediasoup::plainclient::SendStatus::Sent) {
				rtcp_.onAudioRtpSent(size, ts);
			}
		}
	}

	void processEncodedAU(mt::EncodedAccessUnit& au) {
		TrackNetState* track = findTrack(au.ssrc);
		if (!track || track->paused) return;

		// Reject AU from old config generation
		if (au.configGeneration < track->configGeneration) return;
		if (au.configGeneration > track->configGeneration) {
			track->configGeneration = au.configGeneration;
			// New generation — invalidate old keyframe cache and retransmission store
			auto* stream = rtcp_.getVideoStream(au.ssrc);
			if (stream) {
				stream->lastKeyframe.clear();
				stream->store = RtpPacketStore{};
			}
			if (useTransportController_) {
				transportController_.DropQueuedFreshVideoForTrack(au.ssrc);
				transportController_.DropQueuedRetransmissionForTrack(au.ssrc);
			}
		}

		if (au.isKeyframe) {
			rtcp_.cacheKeyframe(au.ssrc, au.data.get(), au.size, au.rtpTimestamp);
		}
		sendH264(au.data.get(), static_cast<int>(au.size),
			au.payloadType, au.rtpTimestamp, au.ssrc, track->seq);
	}

	void processIncomingRtcp() {
		uint8_t buf[1500];
		while (true) {
			int n = recv(cfg_.udpFd, buf, sizeof(buf), MSG_DONTWAIT);
			if (n <= 0) break;

			size_t offset = 0;
			while (offset + 4 <= (size_t)n) {
				uint8_t version = (buf[offset] >> 6) & 0x3;
				if (version != 2) break;
				uint8_t fmt = buf[offset] & 0x1F;
				uint8_t pt = buf[offset + 1];
				uint16_t lengthWords = (buf[offset + 2] << 8) | buf[offset + 3];
				size_t pktLen = (lengthWords + 1) * 4;
				if (offset + pktLen > (size_t)n) break;

				uint32_t mediaSsrc = 0;
				if (pktLen >= 12) {
					mediaSsrc = ((uint32_t)buf[offset + 8] << 24) | ((uint32_t)buf[offset + 9] << 16)
						| ((uint32_t)buf[offset + 10] << 8) | buf[offset + 11];
				}

				if (pt == RTCP_PT_PSFB) {
					// PLI / FIR
					uint32_t targetSsrc = mediaSsrc;
					if (fmt == 4 && targetSsrc == 0 && pktLen >= 20) {
						targetSsrc = ((uint32_t)buf[offset + 12] << 24) | ((uint32_t)buf[offset + 13] << 16)
							| ((uint32_t)buf[offset + 14] << 8) | buf[offset + 15];
					}
					auto* track = findTrackBySsrc(targetSsrc);
					if (track && track->paused) {
						offset += pktLen;
						continue;
					}
					auto* stream = targetSsrc != 0 ? rtcp_.getVideoStream(targetSsrc) : rtcp_.getFirstVideoStream();
					if ((fmt == 1 || fmt == 4) && track && stream) {
						// Forward ForceKeyframe to source worker (with debounce §12.2).
						int64_t now = steadyNowMs();
						if (now - track->lastPliForwardedMs >= TrackNetState::kPliDebounceMs) {
							track->lastPliForwardedMs = now;
							dispatchForceKeyframe(track->trackIndex);
							rtcp_.pliResponded++;
							stream->pliResponded++;
						}
					}
				} else if (pt == RTCP_PT_RTPFB && fmt == mediasoup::plainclient::RTCP_RTPFB_FMT_TRANSPORT_CC) {
						mediasoup::plainclient::TransportCcFeedback feedback;
						if (mediasoup::plainclient::ParseTransportCcFeedback(
								buf + offset,
								pktLen,
								&feedback)) {
							transportCcFeedbackReports_++;
							transportCcFeedbackPacketCount_ += feedback.packetStatusCount;
							transportCcFeedbackLostCount_ += feedback.lostPacketCount;
							UpdateTransportEstimateFromFeedback(feedback);
						} else {
							transportCcMalformedFeedbackCount_++;
						}
				} else if (pt == RTCP_PT_RTPFB && fmt == 1) {
					// NACK — handle locally in network thread
					auto* track = findTrackBySsrc(mediaSsrc);
					if (track && track->paused) {
						offset += pktLen;
						continue;
					}
					auto* stream = mediaSsrc != 0 ? rtcp_.getVideoStream(mediaSsrc) : rtcp_.getFirstVideoStream();
					if (stream) {
						if (useTransportController_) {
							(void)enqueueNackPackets(
								buf + offset,
								pktLen,
								stream->store,
								[this, mediaSsrc](const uint8_t* packet, size_t packetLen) {
									return transportController_.EnqueueVideoRetransmissionPacket(
										mediaSsrc, packet, packetLen);
								});
						} else {
							int ret = handleNack(buf + offset, pktLen, stream->store, cfg_.udpFd);
							rtcp_.nackRetransmitted += ret;
							stream->nackRetransmitted += ret;
						}
					}
				} else if (pt == RTCP_PT_RR && pktLen >= 32) {
					// RR — parse into per-track stats (reuse RtcpContext logic)
					parseRR(buf + offset, pktLen);
				}

				offset += pktLen;
			}
		}
	}

	void parseRR(const uint8_t* buf, size_t pktLen) {
		size_t rbOff = 8;
		while (rbOff + 24 <= pktLen) {
			uint32_t reportSsrc = ((uint32_t)buf[rbOff] << 24) | ((uint32_t)buf[rbOff+1] << 16)
				| ((uint32_t)buf[rbOff+2] << 8) | buf[rbOff+3];
			auto* stream = rtcp_.getVideoStream(reportSsrc);
			if (!stream) { rbOff += 24; continue; }

			stream->rrLossFraction = buf[rbOff + 4] / 256.0;
			stream->rrCumulativeLost = parseClampedRtcpTotalLost(buf + rbOff + 5);
			uint32_t jitter = ((uint32_t)buf[rbOff+12] << 24) | ((uint32_t)buf[rbOff+13] << 16)
				| ((uint32_t)buf[rbOff+14] << 8) | buf[rbOff+15];
			uint32_t lsr = ((uint32_t)buf[rbOff+16] << 24) | ((uint32_t)buf[rbOff+17] << 16)
				| ((uint32_t)buf[rbOff+18] << 8) | buf[rbOff+19];
			uint32_t dlsr = ((uint32_t)buf[rbOff+20] << 24) | ((uint32_t)buf[rbOff+21] << 16)
				| ((uint32_t)buf[rbOff+22] << 8) | buf[rbOff+23];

			stream->rrJitterMs = (double)jitter / 90.0;
			if (lsr != 0) {
				auto it = stream->srNtpToSendTime.find(lsr);
				if (it != stream->srNtpToSendTime.end()) {
					double dlsrMs = (double)dlsr / 65536.0 * 1000.0;
					double elapsed = (double)(steadyNowMs() - it->second);
					stream->rrRttMs = std::max(0.0, elapsed - dlsrMs);
				}
			}
			if (stream->rrRttMs > 0.0) {
				sendSideBwe_.UpdateRtt(stream->rrRttMs / 1000.0);
			}
			stream->lastRrReceivedMs = steadyNowMs();
			rbOff += 24;
		}
	}

	void dispatchForceKeyframe(uint32_t trackIndex) {
		// Dispatch to the matching source worker (trackIndex == sourceInputs_ index)
		if (trackIndex < sourceInputs_.size() && sourceInputs_[trackIndex].keyframeQueue) {
			mt::NetworkToSourceCommand cmd;
			cmd.type = mt::NetworkToSourceCommand::ForceKeyframe;
			cmd.trackIndex = trackIndex;
			sourceInputs_[trackIndex].keyframeQueue->tryPush(std::move(cmd));
		}
	}

	void dispatchSourceControl(uint32_t trackIndex, mt::NetworkToSourceCommand::Type type) {
		if (trackIndex < sourceInputs_.size() && sourceInputs_[trackIndex].keyframeQueue) {
			mt::NetworkToSourceCommand cmd;
			cmd.type = type;
			cmd.trackIndex = trackIndex;
			sourceInputs_[trackIndex].keyframeQueue->tryPush(std::move(cmd));
		}
	}

	TrackNetState* findTrack(uint32_t ssrc) {
		for (auto& t : tracks_) if (t.ssrc == ssrc) return &t;
		return nullptr;
	}

	TrackNetState* findTrackBySsrc(uint32_t ssrc) { return findTrack(ssrc); }

	TrackNetState* findTrackByIndex(uint32_t trackIndex) {
		for (auto& t : tracks_) if (t.trackIndex == trackIndex) return &t;
		return nullptr;
	}

	void sendCommandAck(uint32_t trackIndex, mt::TrackCommandType type, uint64_t commandId,
		bool applied, const char* reason)
	{
		if (!commandAckQueue) return;
		mt::CommandAck ack;
		ack.trackIndex = trackIndex;
		ack.type = type;
		ack.commandId = commandId;
		ack.applied = applied;
		ack.reason = reason ? reason : "";
		ack.appliedAtMs = steadyNowMs();
		commandAckQueue->tryPush(std::move(ack));
	}

	static int createTimerFd(int intervalMs) {
		int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
		if (fd < 0) return -1;
		struct itimerspec ts{};
		ts.it_interval.tv_sec = intervalMs / 1000;
		ts.it_interval.tv_nsec = (intervalMs % 1000) * 1000000L;
		ts.it_value = ts.it_interval;
		timerfd_settime(fd, 0, &ts, nullptr);
		return fd;
	}

	void epollAdd(int fd, uint32_t events) {
		struct epoll_event ev{};
		ev.events = events;
		ev.data.fd = fd;
		epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
	}

	void drainQueues() {
		DrainProbeEvents();

		// Control commands first — pause/resume must take effect before draining new AUs.
		if (controlQueue) {
			mt::NetworkControlCommand cmd;
			while (controlQueue->tryPop(cmd)) {
				switch (cmd.type) {
				case mt::NetworkControlCommand::TrackTransportConfig:
					if (useTransportController_) {
						transportController_.UpdateTrackTransportHint(
							cmd.trackIndex, cmd.ssrc, cmd.targetBitrateBps, cmd.paused);
						RefreshPublishedTransportEstimate();
					}
					break;
				case mt::NetworkControlCommand::PauseTrack:
					if (auto* t = findTrackByIndex(cmd.trackIndex)) {
						t->paused = true;
						if (useTransportController_) {
							transportController_.SetTrackPaused(t->ssrc, true);
						} else {
							dropQueuedPacketsForTrack(t->ssrc);
						}
						dispatchSourceControl(cmd.trackIndex, mt::NetworkToSourceCommand::PauseTrack);
						sendCommandAck(cmd.trackIndex, mt::TrackCommandType::PauseTrack, cmd.commandId, true, "");
					}
					break;
				case mt::NetworkControlCommand::ResumeTrack:
					if (auto* t = findTrackByIndex(cmd.trackIndex)) {
						t->paused = false;
						if (useTransportController_) {
							transportController_.SetTrackPaused(t->ssrc, false);
						}
						dispatchSourceControl(cmd.trackIndex, mt::NetworkToSourceCommand::ResumeTrack);
						sendCommandAck(cmd.trackIndex, mt::TrackCommandType::ResumeTrack, cmd.commandId, true, "");
					}
					break;
				case mt::NetworkControlCommand::Shutdown:
					running_ = false;
					break;
				default:
					break;
				}
			}
		}

		// Video AUs
		for (auto& si : sourceInputs_) {
			if (!si.auQueue) continue;
			mt::EncodedAccessUnit au;
			while (si.auQueue->tryPop(au)) processEncodedAU(au);
		}
		// Audio AUs
		if (audioAuQueue) {
			mt::EncodedAccessUnit au;
			while (audioAuQueue->tryPop(au))
				sendOpus(au.data.get(), static_cast<int>(au.size),
					au.payloadType, au.rtpTimestamp, cfg_.audioSsrc, audioSeq_);
		}
	}

	void DrainProbeEvents()
	{
		std::deque<ProbeEvent> events;
		{
			std::lock_guard<std::mutex> lock(probeEventMutex_);
			events.swap(probeEvents_);
		}

		for (const auto& event : events) {
			switch (event.type) {
				case ProbeEventType::StartCluster:
					probeObserver_.StartProbeCluster(event.probeClusterInfo);
					sendSideBwe_.ProbeClusterStarting(event.probeClusterInfo);
					probeClusterStartCount_++;
					break;
				case ProbeEventType::CompleteCluster:
					CompleteProbeCluster(event.clusterId);
					break;
			}
		}
	}

	void loop() {
		constexpr int kMaxEvents = 8;
		struct epoll_event events[kMaxEvents];

		while (running_.load()) {
			int n = epoll_wait(epollFd_, events, kMaxEvents, 50 /*ms timeout as safety net*/);
			if (n < 0) {
				if (errno == EINTR) {
					continue;
				}
				std::fprintf(stderr, "NetworkThread epoll_wait failed: %d\n", errno);
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				continue;
			}

			for (int i = 0; i < n; ++i) {
				int fd = events[i].data.fd;
				if (fd == wakeupFd_) {
					// Drain eventfd counter
					uint64_t v; ::read(wakeupFd_, &v, sizeof(v));
				} else if (fd == srTimerFd_) {
					uint64_t v; ::read(srTimerFd_, &v, sizeof(v));
					rtcp_.maybeSendSR(cfg_.udpFd);
				} else if (fd == statsTimerFd_) {
					uint64_t v; ::read(statsTimerFd_, &v, sizeof(v));
					for (auto& t : tracks_) pushStatsSnapshot(t);
				} else if (fd == pacingTimerFd_) {
					uint64_t v; ::read(pacingTimerFd_, &v, sizeof(v));
					MaybeStartProbeCluster();
					FlushPendingProbePackets();
					MaybeStopProbeClusterOnGoalReached();
					MaybeFinalizeProbeCluster();
					if (useTransportController_) {
						transportController_.OnPacingTick(steadyNowMs());
					} else {
						pacingFlush();
					}
				}
				// fd == cfg_.udpFd is handled below
			}

			// Always drain queues (wakeup or timeout)
			drainQueues();

			// Always check for incoming RTCP
			processIncomingRtcp();
		}
	}

	// ─── Pacing buffer ───────────────────────────────────
	struct PacingEntry {
		uint32_t ssrc = 0;
		uint8_t data[1500];
		size_t len = 0;
	};
	std::deque<PacingEntry> pacingQueue_;
	static constexpr size_t kMaxPacingQueue = 256;
	static constexpr int kPacingIntervalMs = 2; // ~500 pps budget
	int pacingTimerFd_ = -1;

	void pacingEnqueue(const uint8_t* pkt, size_t len, uint32_t ssrc) {
		if (len > 1500 || pacingQueue_.size() >= kMaxPacingQueue) return;
		PacingEntry e;
		e.ssrc = ssrc;
		std::memcpy(e.data, pkt, len);
		e.len = len;
		pacingQueue_.push_back(std::move(e));
	}

	void dropQueuedPacketsForTrack(uint32_t ssrc) {
		pacingQueue_.erase(
			std::remove_if(pacingQueue_.begin(), pacingQueue_.end(),
				[ssrc](const PacingEntry& entry) { return entry.ssrc == ssrc; }),
			pacingQueue_.end());
	}

	void pacingFlush() {
		// Send up to a burst of packets per tick
		constexpr int kBurstLimit = 8;
		for (int i = 0; i < kBurstLimit && !pacingQueue_.empty(); ++i) {
			auto& e = pacingQueue_.front();
			const auto result = sendMediaPacketWithTransportCc(
				mediasoup::plainclient::PacketClass::VideoMedia,
				nullptr,
				e.data,
				e.len);
			if (result.status == mediasoup::plainclient::SendStatus::Sent && e.len >= 12) {
				rtcp_.onVideoRtpSent(e.data, e.len);
			}
			pacingQueue_.pop_front();
		}
	}

	void UpdateTransportEstimateFromFeedback(
		const mediasoup::plainclient::TransportCcFeedback& feedback)
	{
		if (!useTransportController_ || feedback.packetStatusCount == 0) {
			return;
		}
		if (!useTransportEstimate_) {
			return;
		}
		sendSideBwe_.HandleTransportFeedback(feedback, steadyNowUs());
		RefreshPublishedTransportEstimate();
		lastTransportFeedbackAtMs_ = steadyNowMs();
	}

	void RecordSenderUsageSample(int64_t nowUs, size_t bytes)
	{
		senderUsageSamples_.push_back({nowUs, bytes});
		senderUsageBytesWindow_ += bytes;
		PruneSenderUsageSamples(nowUs);
	}

	void PruneSenderUsageSamples(int64_t nowUs)
	{
		while (!senderUsageSamples_.empty() &&
			nowUs - senderUsageSamples_.front().atUs > kSenderUsageWindowUs) {
			senderUsageBytesWindow_ -= senderUsageSamples_.front().bytes;
			senderUsageSamples_.pop_front();
		}
	}

	uint32_t CurrentSenderUsageBitrateBps()
	{
		const int64_t nowUs = steadyNowUs();
		PruneSenderUsageSamples(nowUs);
		if (senderUsageBytesWindow_ == 0) {
			return 0;
		}
		const uint64_t bitrate =
			static_cast<uint64_t>(senderUsageBytesWindow_) * 8u * 1000000u /
			static_cast<uint64_t>(kSenderUsageWindowUs);
		return bitrate > std::numeric_limits<uint32_t>::max()
			? std::numeric_limits<uint32_t>::max()
			: static_cast<uint32_t>(bitrate);
	}

	void MaybeStartProbeCluster()
	{
		if (!useTransportController_ || !useTransportEstimate_) {
			return;
		}
		if (prober_.IsRunning() || !sendSideBwe_.CanProbe()) {
			return;
		}

		const uint32_t aggregateTargetBps = transportController_.AggregateTargetBitrateBps();
		const uint32_t availableBps = transportEstimatedBitrateBps_;
		if (!FindProbeTrack()) {
			return;
		}

		mediasoup::ccutils::ProbeClusterGoal goal;
		if (!BuildProbeClusterGoal(
				aggregateTargetBps,
				availableBps,
				CurrentSenderUsageBitrateBps(),
				cfg_.probeOveragePct,
				cfg_.probeMinBps,
				sendSideBwe_.ProbeDuration(),
				&goal)) {
			return;
		}
		(void)prober_.AddCluster(mediasoup::ccutils::ProbeClusterMode::Uniform, goal);
	}

	uint32_t ProbeRtpTimestampForTrack(uint32_t ssrc) const
	{
		if (const auto* stream = rtcp_.getVideoStream(ssrc)) {
			if (stream->lastRtpTs != 0) {
				return stream->lastRtpTs;
			}
			if (stream->lastKeyframeTs != 0) {
				return stream->lastKeyframeTs;
			}
		}

		const uint64_t now90k = static_cast<uint64_t>(steadyNowMs()) * 90u;
		return static_cast<uint32_t>(now90k & 0xFFFFFFFFu);
	}

	static bool BuildProbeClusterGoal(
		uint32_t aggregateTargetBps,
		uint32_t availableBps,
		uint32_t expectedUsageBps,
		int64_t probeOveragePct,
		int64_t probeMinBps,
		std::chrono::microseconds duration,
		mediasoup::ccutils::ProbeClusterGoal* outGoal)
	{
		if (!outGoal || aggregateTargetBps == 0 || availableBps == 0) {
			return false;
		}
		if (expectedUsageBps == 0) {
			expectedUsageBps = std::min(aggregateTargetBps, availableBps);
		}
		if (aggregateTargetBps <= expectedUsageBps || aggregateTargetBps <= availableBps) {
			return false;
		}

		const uint64_t transitionDeltaBps =
			static_cast<uint64_t>(aggregateTargetBps - expectedUsageBps);
		const uint64_t overagePct = probeOveragePct > 0
			? static_cast<uint64_t>(probeOveragePct)
			: static_cast<uint64_t>(kDefaultProbeOveragePct);
		uint64_t desiredIncreaseBps =
			(transitionDeltaBps * overagePct + 99u) / 100u;
		const uint64_t probeMinBpsSafe = probeMinBps > 0
			? static_cast<uint64_t>(probeMinBps)
			: static_cast<uint64_t>(kDefaultProbeMinBps);
		desiredIncreaseBps = std::max<uint64_t>(desiredIncreaseBps, probeMinBpsSafe);
		const uint64_t desiredBps = std::min<uint64_t>(
			std::numeric_limits<int>::max(),
			static_cast<uint64_t>(expectedUsageBps) + desiredIncreaseBps);

		outGoal->availableBandwidthBps = static_cast<int>(availableBps);
		outGoal->expectedUsageBps = static_cast<int>(expectedUsageBps);
		outGoal->desiredBps = static_cast<int>(desiredBps);
		outGoal->duration = duration;
		return true;
	}

	const TrackNetState* FindProbeTrack() const
	{
		for (const auto& track : tracks_) {
			if (!track.paused && track.transportCcExtensionId != 0) {
				return &track;
			}
		}
		return nullptr;
	}

	void FlushPendingProbePackets()
	{
		const TrackNetState* probeTrack = FindProbeTrack();
		if (!probeTrack) {
			pendingProbeBytes_.store(0);
			return;
		}
		const auto activeProbeClusterId = probeObserver_.ActiveClusterId();
		if (activeProbeClusterId == mediasoup::ccutils::ProbeClusterIdInvalid) {
			pendingProbeBytes_.store(0);
			return;
		}

		int64_t remainingBytes = pendingProbeBytes_.exchange(0);
		while (remainingBytes > 0) {
			const size_t packetBytes = static_cast<size_t>(std::min<int64_t>(
				remainingBytes,
				mediasoup::ccutils::Prober::kBytesPerProbe));
			if (packetBytes <= 12) {
				break;
			}
			const auto result = SendProbePacket(*probeTrack, packetBytes, activeProbeClusterId);
			if (result.status != mediasoup::plainclient::SendStatus::Sent) {
				pendingProbeBytes_.fetch_add(remainingBytes);
				break;
			}
			remainingBytes -= static_cast<int64_t>(packetBytes);
		}
	}

	mediasoup::plainclient::SendResult SendProbePacket(
		const TrackNetState& track,
		size_t packetBytes,
		mediasoup::ccutils::ProbeClusterId probeClusterId)
	{
		const size_t totalBytes = std::clamp<size_t>(packetBytes, 64u, 1200u);
		const size_t paddingBytes = totalBytes > 12 ? (totalBytes - 12) : 1u;
		uint8_t packet[1600]{};
		const uint32_t probeRtpTs = ProbeRtpTimestampForTrack(track.ssrc);
		rtpHeader(packet, track.payloadType, const_cast<TrackNetState&>(track).seq++,
			probeRtpTs, track.ssrc, false);
		packet[0] |= 0x20; // P bit.
		std::memset(packet + 12, 0, paddingBytes);
		packet[12 + paddingBytes - 1] = static_cast<uint8_t>(paddingBytes);
		size_t packetLen = 12 + paddingBytes;
		mediasoup::plainclient::PacketTransportMetadata transportMetadata;
		transportMetadata.isProbe = true;
		transportMetadata.probeClusterId = probeClusterId;
		const auto result = sendMediaPacketWithTransportCc(
			mediasoup::plainclient::PacketClass::VideoMedia,
			&transportMetadata,
			packet,
			packetLen);
		if (result.status == mediasoup::plainclient::SendStatus::Sent) {
			rtcp_.onVideoProbeRtpSent(packet, packetLen);
			prober_.ProbesSent(static_cast<int>(result.bytesSent));
			probeBytesSent_ += result.bytesSent;
		}
		return result;
	}

	void MaybeStopProbeClusterOnGoalReached()
	{
		const auto probeClusterInfo = probeObserver_.CurrentProbeClusterInfo();
		if (!probeClusterInfo.IsValid()) {
			return;
		}
		if (!sendSideBwe_.ProbeClusterIsGoalReached(probeClusterInfo)) {
			return;
		}
		probeClusterEarlyStopCount_++;
		CompleteProbeCluster(probeClusterInfo.id);
	}

	void MaybeFinalizeProbeCluster()
	{
		const auto [probeSignal, estimatedCapacity, finalized] = sendSideBwe_.ProbeClusterFinalize();
		if (!finalized) {
			return;
		}
		(void)probeSignal;
		(void)estimatedCapacity;
		RefreshPublishedTransportEstimate();
	}

	void CompleteProbeCluster(mediasoup::ccutils::ProbeClusterId clusterId)
	{
		transportController_.ClearQueuedProbeClusterAssociation(static_cast<uint32_t>(clusterId));
		auto probeClusterInfo = probeObserver_.EndProbeCluster(clusterId);
		if (!probeClusterInfo.IsValid()) {
			return;
		}
		prober_.ClusterDone(probeClusterInfo);
		sendSideBwe_.ProbeClusterDone(probeClusterInfo);
		probeClusterCompleteCount_++;
	}

	uint32_t ClampConfiguredTransportEstimate(int64_t rawEstimateBps) const
	{
		if (rawEstimateBps <= 0) {
			return 0;
		}
		const uint32_t minTransportEstimateBps = std::min(
			cfg_.transportEstimateMinBps,
			cfg_.transportEstimateMaxBps);
		const uint32_t maxTransportEstimateBps = std::max(
			cfg_.transportEstimateMinBps,
			cfg_.transportEstimateMaxBps);
		const uint64_t capped = static_cast<uint64_t>(std::min<int64_t>(
			rawEstimateBps,
			static_cast<int64_t>(std::numeric_limits<uint32_t>::max())));
		return std::clamp<uint32_t>(
			static_cast<uint32_t>(capped),
			minTransportEstimateBps,
			maxTransportEstimateBps);
	}

	void RefreshPublishedTransportEstimate()
	{
		if (!useTransportController_) {
			return;
		}
		if (!useTransportEstimate_) {
			transportEstimatedBitrateBps_ = 0;
			transportController_.SetTransportEstimatedBitrateBps(0);
			return;
		}
		transportEstimatedBitrateBps_ = ClampConfiguredTransportEstimate(
			sendSideBwe_.EstimatedAvailableChannelCapacityBps());
		transportController_.SetTransportEstimatedBitrateBps(transportEstimatedBitrateBps_);
	}
public:
	const mediasoup::plainclient::SenderTransportController::Metrics& transportMetrics() const {
		return transportController_.GetMetrics();
	}

	size_t queuedFreshVideoPackets() const {
		return useTransportController_
			? transportController_.QueuedFreshVideoPackets()
			: pacingQueue_.size();
	}

	size_t queuedAudioPackets() const {
		return useTransportController_ ? transportController_.QueuedAudioPackets() : 0;
	}

	size_t queuedRetransmissionPackets() const {
		return useTransportController_ ? transportController_.QueuedRetransmissionPackets() : 0;
	}

	int64_t mediaBudgetBytes() const {
		return useTransportController_ ? transportController_.MediaBudgetBytes() : 0;
	}

	uint32_t transportEstimatedBitrateBps() const {
		return transportEstimatedBitrateBps_;
	}

	uint64_t transportCcFeedbackReports() const {
		return transportCcFeedbackReports_;
	}

	uint64_t transportCcMalformedFeedbackCount() const {
		return transportCcMalformedFeedbackCount_;
	}

	uint32_t effectivePacingBitrateBps() const {
		return useTransportController_ ? transportController_.EffectivePacingBitrateBps() : 0;
	}

	void recordTransportCcPacketForTest(
		uint16_t sequenceNumber,
		size_t sizeBytes,
		int64_t sendTimeUs)
	{
		sendSideBwe_.SeedSentPacketForTest(sequenceNumber, sendTimeUs, sizeBytes);
	}

	void seedTransportEstimateForTest(int64_t capacityBps)
	{
		sendSideBwe_.SeedEstimatedAvailableChannelCapacityForTest(capacityBps);
		RefreshPublishedTransportEstimate();
	}

	static bool BuildProbeClusterGoalForTest(
		uint32_t aggregateTargetBps,
		uint32_t availableBps,
		uint32_t expectedUsageBps,
		int64_t probeOveragePct,
		int64_t probeMinBps,
		std::chrono::microseconds duration,
		mediasoup::ccutils::ProbeClusterGoal* outGoal)
	{
		return BuildProbeClusterGoal(
			aggregateTargetBps,
			availableBps,
			expectedUsageBps,
			probeOveragePct,
			probeMinBps,
			duration,
			outGoal);
	}

	void SetUdpSendFnForTest(std::function<mediasoup::plainclient::SendResult(
		int,
		const uint8_t*,
		size_t)> fn)
	{
		udpSendFn_ = std::move(fn);
	}

	private:
		struct SenderUsageSample {
			int64_t atUs{ 0 };
			size_t bytes{ 0 };
		};
		static constexpr int64_t kSenderUsageWindowUs = 500000;

	mediasoup::plainclient::SendResult SendUdpPacket(const uint8_t* data, size_t len)
	{
		if (udpSendFn_) {
			return udpSendFn_(cfg_.udpFd, data, len);
		}
		return mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, data, len);
	}

	Config cfg_;
	bool useTransportController_{ true };
	bool useTransportEstimate_{ true };
	RtcpContext rtcp_;
	mediasoup::plainclient::sendsidebwe::SendSideBwe sendSideBwe_;
	mediasoup::plainclient::SenderTransportController transportController_;
	std::vector<TrackNetState> tracks_;
	std::vector<SourceInput> sourceInputs_;
	std::atomic<bool> running_{false};
	std::thread thread_;
	uint16_t audioSeq_ = 0;
	uint32_t transportEstimatedBitrateBps_{ 0 };
	int64_t lastTransportFeedbackAtMs_{ 0 };
	uint64_t transportCcRewrittenPacketsSent_{ 0 };
	uint64_t transportCcRewriteFailures_{ 0 };
	uint64_t transportCcFeedbackReports_{ 0 };
	uint64_t transportCcFeedbackPacketCount_{ 0 };
	uint64_t transportCcFeedbackLostCount_{ 0 };
	uint64_t transportCcMalformedFeedbackCount_{ 0 };
	uint64_t probeClusterStartCount_{ 0 };
	uint64_t probeClusterCompleteCount_{ 0 };
	uint64_t probeClusterEarlyStopCount_{ 0 };
	uint64_t probeBytesSent_{ 0 };
	std::deque<SenderUsageSample> senderUsageSamples_;
	size_t senderUsageBytesWindow_{ 0 };
	uint64_t statsGeneration_ = 0;
	std::function<mediasoup::plainclient::SendResult(int, const uint8_t*, size_t)> udpSendFn_;
	enum class ProbeEventType {
		StartCluster,
		CompleteCluster
	};
	struct ProbeEvent {
		ProbeEventType type;
		mediasoup::ccutils::ProbeClusterInfo probeClusterInfo;
		mediasoup::ccutils::ProbeClusterId clusterId;
	};
	mediasoup::ccutils::Prober prober_{this};
	mediasoup::plainclient::sendsidebwe::ProbeObserver probeObserver_;
	std::mutex probeEventMutex_;
	std::deque<ProbeEvent> probeEvents_;
	std::atomic<int64_t> pendingProbeBytes_{ 0 };
	int epollFd_ = -1;
	int wakeupFd_ = -1;
	int srTimerFd_ = -1;
	int statsTimerFd_ = -1;
};
