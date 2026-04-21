#include "PlainClientApp.h"

#include "NetworkThread.h"
#include "UdpSendHelpers.h"

#include <limits>
#include <map>

namespace msff = mediasoup::ffmpeg;

int PlainClientApp::RunLegacyMode()
{
	if (!inputFormat_) {
		std::fprintf(stderr, "legacy mode requires input media bootstrap\n");
		return 1;
	}

	RtcpContext rtcp;
	rtcp.audioSsrc = audioSsrc_;
	rtcp.sendH264Fn = PlainClientApp::SendH264ViaSharedPacketizer;
	rtcp.canSendVideoFn = [this](uint32_t ssrc) {
		for (const auto& track : videoTracks_) {
			if (track.ssrc == ssrc) return !track.videoSuppressed;
		}
		return true;
	};
	rtcp.requestKeyframeFn = [this, &rtcp](uint32_t ssrc) {
		for (auto& track : videoTracks_) {
			if (track.ssrc != ssrc || track.videoSuppressed) continue;

			if (track.encoder) {
				track.forceNextVideoKeyframe = true;
				track.nextVideoEncodePtsSec = -1.0;
				return;
			}

			const auto* stream = rtcp.getVideoStream(ssrc);
			if (stream && !stream->lastKeyframe.empty() && stream->seqPtr) {
				const uint32_t resendTs =
					stream->lastRtpTs != 0 ? stream->lastRtpTs : stream->lastKeyframeTs;
				SendH264ViaSharedPacketizer(
					udpFd_,
					stream->lastKeyframe.data(),
					static_cast<int>(stream->lastKeyframe.size()),
					stream->payloadType,
					resendTs,
					stream->ssrc,
					*stream->seqPtr);
			}
			return;
		}
	};
	rtcpContext_ = &rtcp;

	for (auto& track : videoTracks_) {
		rtcp.registerVideoStream(track.ssrc, track.payloadType, &track.seq);
	}

	for (auto& track : videoTracks_) {
		for (int i = 0; i < 5; ++i) {
			uint8_t dummy[12 + 1];
			rtpHeader(dummy, track.payloadType, track.seq++, 0, track.ssrc, false);
			dummy[12] = 0;
			(void)mediasoup::plainclient::SendUdpDatagram(udpFd_, dummy, 13);
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
		}
	}
	std::printf("Sent probe packets for comedia across %zu video tracks\n", videoTracks_.size());
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	auto packet = msff::MakePacket();
	auto t0 = std::chrono::steady_clock::now();
	double firstPts = -1;
	int frameCnt = 0;

	while (running_.load() && inputFormat_ && inputFormat_->ReadPacket(packet.get())) {
		auto* stream = inputFormat_->StreamAt(packet->stream_index);
		double pts = packet->pts * av_q2d(stream->time_base);
		if (firstPts < 0) firstPts = pts;
		double elapsed = pts - firstPts;

		auto target = t0 + std::chrono::microseconds((int64_t)(elapsed * 1e6));
		std::this_thread::sleep_until(target);
		ws_.dispatchNotifications();

		if (packet->stream_index == vidIdx_) {
			const uint32_t rtpTs = (uint32_t)(pts * 90000);
			if (!copyMode_ && videoDecoder_.has_value() && !videoTracks_.empty()) {
				videoDecoder_->SendPacket(packet.get());
				while (videoDecoder_->ReceiveFrame(videoFrame_.get())) {
					double framePtsSec = pts;
					if (videoFrame_->best_effort_timestamp != AV_NOPTS_VALUE) {
						framePtsSec = videoFrame_->best_effort_timestamp *
							av_q2d(inputFormat_->StreamAt(vidIdx_)->time_base);
					}

					for (auto& track : videoTracks_) {
						if (!track.encoder.has_value()) continue;
						auto* encoderCtx = track.encoder->get();

						if (track.configuredVideoFps > 0 && !track.forceNextVideoKeyframe) {
							if (track.nextVideoEncodePtsSec < 0)
								track.nextVideoEncodePtsSec = framePtsSec;
							const double minEncodeIntervalSec = 1.0 / track.configuredVideoFps;
							if (framePtsSec + 1e-6 < track.nextVideoEncodePtsSec) continue;
							track.nextVideoEncodePtsSec = framePtsSec + minEncodeIntervalSec;
						}

						if (track.videoSuppressed) continue;

						AVFrame* frameToEncode = videoFrame_.get();
						if (encoderCtx->width != videoFrame_->width ||
							encoderCtx->height != videoFrame_->height ||
							videoFrame_->format != AV_PIX_FMT_YUV420P) {
							msff::FrameMakeWritable(track.scaledFrame.get());
							track.swsCtx = sws_getCachedContext(
								track.swsCtx,
								videoFrame_->width,
								videoFrame_->height,
								static_cast<AVPixelFormat>(videoFrame_->format),
								encoderCtx->width,
								encoderCtx->height,
								AV_PIX_FMT_YUV420P,
								SWS_BILINEAR,
								nullptr,
								nullptr,
								nullptr);
							if (!track.swsCtx) continue;
							sws_scale(
								track.swsCtx,
								videoFrame_->data,
								videoFrame_->linesize,
								0,
								videoFrame_->height,
								track.scaledFrame->data,
								track.scaledFrame->linesize);
							track.scaledFrame->pts = videoFrame_->pts;
							frameToEncode = track.scaledFrame.get();
						}

						if (track.forceNextVideoKeyframe) {
							frameToEncode->pict_type = AV_PICTURE_TYPE_I;
							track.forceNextVideoKeyframe = false;
						} else {
							frameToEncode->pict_type = AV_PICTURE_TYPE_NONE;
						}

						track.encoder->SendFrame(frameToEncode);
						auto encodedPacket = msff::MakePacket();
						while (track.encoder->ReceivePacket(encodedPacket.get())) {
							if (encodedPacket->flags & AV_PKT_FLAG_KEY) {
								rtcp.cacheKeyframe(
									track.ssrc,
									encodedPacket->data,
									encodedPacket->size,
									rtpTs);
							}
							if (!track.videoSuppressed) {
								SendH264ViaSharedPacketizer(
									udpFd_,
									encodedPacket->data,
									encodedPacket->size,
									track.payloadType,
									rtpTs,
									track.ssrc,
									track.seq);
							}
							msff::PacketUnref(encodedPacket.get());
						}
					}
				}
			} else {
				if (h264BitstreamFilter_.has_value()) {
					auto filteredPacket = msff::MakePacket();
					msff::PacketRef(filteredPacket.get(), packet.get());
					h264BitstreamFilter_->SendPacket(filteredPacket.get());
					while (h264BitstreamFilter_->ReceivePacket(filteredPacket.get())) {
						for (auto& track : videoTracks_) {
							if (filteredPacket->flags & AV_PKT_FLAG_KEY)
								rtcp.cacheKeyframe(track.ssrc, filteredPacket->data, filteredPacket->size, rtpTs);
							if (!track.videoSuppressed) {
								SendH264ViaSharedPacketizer(
									udpFd_,
									filteredPacket->data,
									filteredPacket->size,
									track.payloadType,
									rtpTs,
									track.ssrc,
									track.seq);
							}
						}
						msff::PacketUnref(filteredPacket.get());
					}
				} else {
					for (auto& track : videoTracks_) {
						if (packet->flags & AV_PKT_FLAG_KEY)
							rtcp.cacheKeyframe(track.ssrc, packet->data, packet->size, rtpTs);
						if (!track.videoSuppressed) {
							SendH264ViaSharedPacketizer(
								udpFd_,
								packet->data,
								packet->size,
								track.payloadType,
								rtpTs,
								track.ssrc,
								track.seq);
						}
					}
				}
			}

			rtcp.processIncomingRtcp(udpFd_);
			rtcp.maybeSendSR(udpFd_);

			bool hasVideoQos = false;
			int sampleIntervalMs = std::numeric_limits<int>::max();
			for (const auto& track : videoTracks_) {
				if (!track.qosCtrl) continue;
				hasVideoQos = true;
				sampleIntervalMs = std::min(
					sampleIntervalMs, track.qosCtrl->getRuntimeSettings().sampleIntervalMs);
			}

			if (hasVideoQos && sampleIntervalMs < std::numeric_limits<int>::max()) {
				const auto now = steadyNowMs();
				if (now - lastPeerQosSampleMs_ >= sampleIntervalMs) {
					lastPeerQosSampleMs_ = now;
					RequestServerProducerStats();

					std::optional<json> cachedPeerStats;
					{
						std::lock_guard<std::mutex> lock(cachedServerStatsResponse_->mutex);
						const int64_t maxStatsAgeMs =
							std::max<int64_t>(5000, static_cast<int64_t>(sampleIntervalMs) * 4);
						if (cachedServerStatsResponse_->latestPeerStats.has_value() &&
							now - cachedServerStatsResponse_->updatedAtSteadyMs <= maxStatsAgeMs) {
							cachedPeerStats = *cachedServerStatsResponse_->latestPeerStats;
						}
					}

					std::vector<qos::PeerTrackState> peerTrackStates;
					std::map<std::string, qos::DerivedSignals> peerTrackSignals;
					std::map<std::string, qos::PlannedAction> peerLastActions;
					std::map<std::string, qos::RawSenderSnapshot> peerRawSnapshots;
					std::map<std::string, bool> peerActionApplied;
					std::vector<qos::TrackBudgetRequest> trackBudgetRequests;
					uint64_t totalDesiredBudgetBps = 0;
					uint64_t totalObservedBudgetBps = 0;
					bool anyBandwidthLimited = false;
					bool peerSnapshotRequested = false;

					for (auto& track : videoTracks_) {
						if (!track.qosCtrl) continue;
						track.snapshotRequested = false;

						qos::RawSenderSnapshot snap;
						snap.timestampMs = now;
						snap.trackId = track.trackId;
						snap.producerId = track.producerId;
						snap.source = qos::Source::Camera;
						snap.kind = qos::TrackKind::Video;
						snap.targetBitrateBps = track.encBitrate;
						snap.configuredBitrateBps = track.encBitrate;
						LossCounterSource lossCounterSource = LossCounterSource::None;

						if (const auto* rtcpStream = rtcp.getVideoStream(track.ssrc)) {
							snap.bytesSent = rtcpStream->octetCount;
							snap.packetsSent = rtcpStream->packetCount;
							snap.packetsLost = rtcpStream->rrCumulativeLost;
							snap.roundTripTimeMs = rtcpStream->rrRttMs;
							snap.jitterMs = rtcpStream->rrJitterMs;
							lossCounterSource = LossCounterSource::LocalRtcp;
						}

						bool usingServerStats = false;
						bool usingMatrixTestProfile = false;
						double observedBitrateBps = 0.0;
						if (cachedPeerStats.has_value() && !track.producerId.empty()) {
							auto parsed = parseServerProducerStats(*cachedPeerStats, track.producerId, "video");
							if (parsed.has_value()) {
								usingServerStats = true;
								if (!track.lossBaseInitialized) {
									track.lossBase = parsed->packetsLost;
									track.lossBaseInitialized = true;
								}
								if (parsed->packetsLost >= track.lossBase) {
									parsed->packetsLost -= track.lossBase;
								} else {
									track.lossBase = parsed->packetsLost;
									parsed->packetsLost = 0;
								}

								if (parsed->byteCount > 0) snap.bytesSent = parsed->byteCount;
								if (parsed->packetCount > 0) snap.packetsSent = parsed->packetCount;
								snap.packetsLost = parsed->packetsLost;
								if (parsed->roundTripTimeMs >= 0) snap.roundTripTimeMs = parsed->roundTripTimeMs;
								if (parsed->jitterMs >= 0) snap.jitterMs = parsed->jitterMs;
								observedBitrateBps = parsed->bitrateBps;
								lossCounterSource = LossCounterSource::ServerStats;
							}
						}

						if (matrixTestProfile_.has_value() && track.index == 0) {
							if (auto syntheticSendBps = applyMatrixTestProfile(
									snap, track.encBitrate, *matrixTestProfile_, matrixTestRuntime_, now)) {
								observedBitrateBps = *syntheticSendBps;
								usingMatrixTestProfile = true;
							}
						}

						if (track.encoder.has_value()) {
							snap.frameWidth = track.encoder->get()->width;
							snap.frameHeight = track.encoder->get()->height;
							snap.framesPerSecond = track.configuredVideoFps;
						}

						if (observedBitrateBps > 0.0 &&
							snap.targetBitrateBps > 0 &&
							observedBitrateBps < snap.targetBitrateBps * qos::NETWORK_CONGESTED_UTILIZATION) {
							snap.qualityLimitationReason = qos::QualityLimitationReason::Bandwidth;
							anyBandwidthLimited = true;
						}

						if (track.lossCounterSource != LossCounterSource::None &&
							lossCounterSource != LossCounterSource::None &&
							track.lossCounterSource != lossCounterSource) {
							track.qosCtrl->primeSnapshotBaseline(snap);
						}
						if (lossCounterSource != LossCounterSource::None)
							track.lossCounterSource = lossCounterSource;

						track.qosCtrl->onSample(snap);
						peerTrackStates.push_back(track.qosCtrl->getTrackState());
						if (auto derivedSignals = track.qosCtrl->lastSignals())
							peerTrackSignals[track.trackId] = *derivedSignals;
						peerLastActions[track.trackId] = track.qosCtrl->lastAction();
						peerRawSnapshots[track.trackId] = snap;
						peerActionApplied[track.trackId] = track.qosCtrl->lastActionWasApplied();
						peerSnapshotRequested = peerSnapshotRequested || track.snapshotRequested;

						const uint32_t desiredBitrateBps = GetTrackDesiredBitrateBps(track);
						totalDesiredBudgetBps += desiredBitrateBps;
						totalObservedBudgetBps += observedBitrateBps > 0.0
							? static_cast<uint64_t>(std::llround(observedBitrateBps))
							: desiredBitrateBps;
						trackBudgetRequests.push_back({
							track.trackId,
							qos::Source::Camera,
							qos::TrackKind::Video,
							GetTrackMinBitrateBps(track),
							desiredBitrateBps,
							desiredBitrateBps,
							track.weight,
							false
						});

							const double traceLossRate = track.qosCtrl->lastSignals().has_value()
								? track.qosCtrl->lastSignals()->lossRate
								: 0.0;
							std::printf("[QOS_TRACE] tsMs=%lld track=%s state=%s level=%d mode=%s sample=%s bitrateBps=%d sendBps=%.0f lossRate=%.6f packetsLost=%llu rttMs=%.1f jitterMs=%.1f width=%d height=%d fps=%d suppressed=%d\n",
								static_cast<long long>(wallNowMs()),
								track.trackId.c_str(),
								qos::stateStr(track.qosCtrl->currentState()),
								track.qosCtrl->currentLevel(),
							track.videoSuppressed ? "audio-only" : "audio-video",
								usingMatrixTestProfile ? "matrix" : (usingServerStats ? "server" : "local"),
								track.encBitrate,
								observedBitrateBps > 0.0 ? observedBitrateBps : snap.targetBitrateBps,
								traceLossRate,
								static_cast<unsigned long long>(snap.packetsLost),
								snap.roundTripTimeMs,
							snap.jitterMs,
							track.encoder.has_value() ? track.encoder->get()->width : 0,
							track.encoder.has_value() ? track.encoder->get()->height : 0,
							track.configuredVideoFps,
							track.videoSuppressed ? 1 : 0);
					}

					if (trackBudgetRequests.size() > 1) {
						uint32_t totalBudgetBps = static_cast<uint32_t>(
							std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalDesiredBudgetBps));
						if (anyBandwidthLimited && totalObservedBudgetBps > 0) {
							totalBudgetBps = std::min<uint32_t>(totalBudgetBps, static_cast<uint32_t>(
								std::min<uint64_t>(std::numeric_limits<uint32_t>::max(), totalObservedBudgetBps)));
						}
						auto budgetDecision = qos::allocatePeerTrackBudgets(totalBudgetBps, trackBudgetRequests);
						auto coordinationOverrides =
							qos::buildCoordinationOverrides(peerTrackStates, budgetDecision);
						for (auto& track : videoTracks_) {
							if (!track.qosCtrl) continue;
							auto overrideIt = coordinationOverrides.find(track.trackId);
							if (overrideIt != coordinationOverrides.end())
								track.qosCtrl->setCoordinationOverride(overrideIt->second);
							else
								track.qosCtrl->setCoordinationOverride(std::nullopt);
						}
					} else {
						for (auto& track : videoTracks_) {
							if (track.qosCtrl)
								track.qosCtrl->setCoordinationOverride(std::nullopt);
						}
					}

					if (peerSnapshotRequested && !peerTrackStates.empty() &&
						testClientStatsPayloads_.empty() && !matrixLocalOnly_) {
						static constexpr size_t kMaxPendingClientStats = 8;
						static int peerSnapshotSeq = 0;
						if (ws_.pendingRequestCount() >= kMaxPendingClientStats) {
							std::printf("[QoS] clientStats dropped: too many pending requests (%zu)\n",
								ws_.pendingRequestCount());
						} else {
							auto peerDecision = qos::buildPeerDecision(peerTrackStates);
							auto peerSnapshot = qos::serializeSnapshot(
								peerSnapshotSeq++,
								wallNowMs(),
								peerDecision.peerQuality,
								false,
								peerTrackStates,
								peerTrackSignals,
								peerLastActions,
								peerRawSnapshots,
								peerActionApplied,
								(audIdx_ >= 0 && audioDecoder_.has_value() && audioEncoder_.has_value()));
							ws_.requestAsync("clientStats", peerSnapshot,
								[](bool ok, const json&, const std::string& error) {
									if (!ok) std::printf("[QoS] clientStats failed: %s\n", error.c_str());
								});
						}
					}
				}
			}

			if (++frameCnt % 100 == 0)
				std::printf("sent %d frames [nack=%d pli=%d tracks=%zu]\n",
					frameCnt, rtcp.nackRetransmitted, rtcp.pliResponded, videoTracks_.size());
		} else if (packet->stream_index == audIdx_ &&
			audioDecoder_.has_value() && audioEncoder_.has_value() && audioFrame_) {
			audioDecoder_->SendPacket(packet.get());
			while (audioDecoder_->ReceiveFrame(audioFrame_.get())) {
				audioEncoder_->SendFrame(audioFrame_.get());
				auto encodedPacket = msff::MakePacket();
					while (audioEncoder_->ReceivePacket(encodedPacket.get())) {
						if (SendOpus(
							udpFd_,
							encodedPacket->data,
							encodedPacket->size,
							audioPt_,
							(uint32_t)(pts * 48000),
							audioSsrc_,
							audioSeq_)) {
							rtcp.onAudioRtpSent(encodedPacket->size, (uint32_t)(pts * 48000));
						}
						msff::PacketUnref(encodedPacket.get());
					}
			}
		}

		msff::PacketUnref(packet.get());
	}

	rtcpContext_ = nullptr;
	std::printf("Done: %d video frames sent\n", frameCnt);
	return 0;
}
