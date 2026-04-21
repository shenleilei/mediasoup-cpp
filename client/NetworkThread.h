// NetworkThread.h — Dedicated network thread owning UDP socket, RTP/RTCP state
// Phase 2 of linux-client-multi-source-thread-model migration.
#pragma once

#include "RtcpHandler.h"
#include "SenderTransportController.h"
#include "TransportCcHelpers.h"
#include "ThreadTypes.h"
#include "media/rtp/H264Packetizer.h"

#include <arpa/inet.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <poll.h>

#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
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
class NetworkThread {
public:
	struct Config {
		int udpFd = -1;                // pre-created connected UDP socket
		uint32_t audioSsrc = 0;
		uint8_t audioPt = 0;
		uint8_t audioTransportCcExtensionId = 0;
		uint32_t applicationBitrateCapBps = 0;
		bool enableTransportController = true;
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
	{
		rtcp_.audioSsrc = cfg.audioSsrc;
		transportController_.SetSendFn([this](
			mediasoup::plainclient::PacketClass packetClass,
			const uint8_t* data,
			size_t len) {
			return sendMediaPacketWithTransportCc(packetClass, data, len);
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
			if (transportEstimatedBitrateBps_ == 0) {
				transportEstimatedBitrateBps_ = transportController_.AggregateTargetBitrateBps();
				transportController_.SetTransportEstimatedBitrateBps(transportEstimatedBitrateBps_);
			}
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
		statsQueue->tryPush(std::move(snap));
	}

private:
	static int64_t steadyNowMs() {
		return std::chrono::duration_cast<std::chrono::milliseconds>(
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
		const uint8_t* data,
		size_t len)
	{
		if (!isTransportCcPacketClass(packetClass) || !data || len < 12) {
			return mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, data, len);
		}

		const uint32_t ssrc =
			(static_cast<uint32_t>(data[8]) << 24) |
			(static_cast<uint32_t>(data[9]) << 16) |
			(static_cast<uint32_t>(data[10]) << 8) |
			static_cast<uint32_t>(data[11]);
		const uint8_t extensionId = transportCcExtensionIdForSsrc(ssrc);
		if (extensionId == 0) {
			return mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, data, len);
		}

		uint8_t rewrittenPacket[1600];
		size_t rewrittenLen = sizeof(rewrittenPacket);
		if (!mediasoup::plainclient::RewriteRtpWithTransportCcSequence(
				data,
				len,
				extensionId,
				nextTransportWideCcSequence_,
				rewrittenPacket,
				&rewrittenLen)) {
			transportCcRewriteFailures_++;
			return mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, data, len);
		}

		const auto result = mediasoup::plainclient::SendUdpDatagram(cfg_.udpFd, rewrittenPacket, rewrittenLen);
		if (result.status == mediasoup::plainclient::SendStatus::Sent) {
			nextTransportWideCcSequence_++;
			transportCcRewrittenPacketsSent_++;
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
					mediasoup::plainclient::TransportCcFeedbackSummary summary;
					if (mediasoup::plainclient::ParseTransportCcFeedbackSummary(
							buf + offset,
							pktLen,
							&summary)) {
						transportCcFeedbackReports_++;
						transportCcFeedbackPacketCount_ += summary.packetStatusCount;
						transportCcFeedbackLostCount_ += summary.lostPacketCount;
						UpdateTransportEstimateFromFeedback(summary);
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
		// Control commands first — pause/resume must take effect before draining new AUs.
		if (controlQueue) {
			mt::NetworkControlCommand cmd;
			while (controlQueue->tryPop(cmd)) {
				switch (cmd.type) {
				case mt::NetworkControlCommand::TrackTransportConfig:
					if (useTransportController_) {
						transportController_.UpdateTrackTransportHint(
							cmd.trackIndex, cmd.ssrc, cmd.targetBitrateBps, cmd.paused);
						if (transportEstimatedBitrateBps_ == 0) {
							transportEstimatedBitrateBps_ = transportController_.AggregateTargetBitrateBps();
							transportController_.SetTransportEstimatedBitrateBps(transportEstimatedBitrateBps_);
						}
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

	void loop() {
		constexpr int kMaxEvents = 8;
		struct epoll_event events[kMaxEvents];

		while (running_.load()) {
			int n = epoll_wait(epollFd_, events, kMaxEvents, 50 /*ms timeout as safety net*/);

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
				e.data,
				e.len);
			if (result.status == mediasoup::plainclient::SendStatus::Sent && e.len >= 12) {
				rtcp_.onVideoRtpSent(e.data, e.len);
			}
			pacingQueue_.pop_front();
		}
	}

	void UpdateTransportEstimateFromFeedback(
		const mediasoup::plainclient::TransportCcFeedbackSummary& summary)
	{
		if (!useTransportController_ || summary.packetStatusCount == 0) {
			return;
		}

		uint32_t estimateBps = transportEstimatedBitrateBps_;
		if (estimateBps == 0) {
			estimateBps = transportController_.AggregateTargetBitrateBps();
		}
		if (estimateBps == 0) {
			return;
		}

		const double lossRatio =
			static_cast<double>(summary.lostPacketCount) /
			static_cast<double>(summary.packetStatusCount);
		if (lossRatio >= 0.10) {
			estimateBps = static_cast<uint32_t>(
				static_cast<uint64_t>(estimateBps) * 80u / 100u);
		} else if (lossRatio >= 0.02) {
			estimateBps = static_cast<uint32_t>(
				static_cast<uint64_t>(estimateBps) * 92u / 100u);
		} else {
			const uint32_t additiveStep = std::max<uint32_t>(20000u, estimateBps / 20u);
			estimateBps = estimateBps > kMaxTransportEstimateBps - additiveStep
				? kMaxTransportEstimateBps
				: (estimateBps + additiveStep);
		}

		estimateBps = std::clamp<uint32_t>(
			estimateBps,
			kMinTransportEstimateBps,
			kMaxTransportEstimateBps);
		transportEstimatedBitrateBps_ = estimateBps;
		lastTransportFeedbackAtMs_ = steadyNowMs();
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

	uint32_t effectivePacingBitrateBps() const {
		return useTransportController_ ? transportController_.EffectivePacingBitrateBps() : 0;
	}

private:
	Config cfg_;
	bool useTransportController_{ true };
	RtcpContext rtcp_;
	mediasoup::plainclient::SenderTransportController transportController_;
	std::vector<TrackNetState> tracks_;
	std::vector<SourceInput> sourceInputs_;
	std::atomic<bool> running_{false};
	std::thread thread_;
	uint16_t audioSeq_ = 0;
	uint16_t nextTransportWideCcSequence_{ 0 };
	uint32_t transportEstimatedBitrateBps_{ 0 };
	int64_t lastTransportFeedbackAtMs_{ 0 };
	uint64_t transportCcRewrittenPacketsSent_{ 0 };
	uint64_t transportCcRewriteFailures_{ 0 };
	uint64_t transportCcFeedbackReports_{ 0 };
	uint64_t transportCcFeedbackPacketCount_{ 0 };
	uint64_t transportCcFeedbackLostCount_{ 0 };
	uint64_t transportCcMalformedFeedbackCount_{ 0 };
	static constexpr uint32_t kMinTransportEstimateBps = 30000u;
	static constexpr uint32_t kMaxTransportEstimateBps = 8000000u;
	uint64_t statsGeneration_ = 0;
	int epollFd_ = -1;
	int wakeupFd_ = -1;
	int srTimerFd_ = -1;
	int statsTimerFd_ = -1;
};
