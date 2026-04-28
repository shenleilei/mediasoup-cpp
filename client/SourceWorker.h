// SourceWorker.h — Per-video-source worker thread: capture/decode/scale/encode → EncodedAccessUnit
// Supports file input (MP4) and V4L2 camera input.
// Phase 3 of linux-client-multi-source-thread-model migration.
#pragma once

#include "ThreadTypes.h"

extern "C" {
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavdevice/avdevice.h>
}

#include "DimensionUtils.h"
#include "TestHooks.h"
#include "ffmpeg/AvPtr.h"
#include "ffmpeg/Decoder.h"
#include "ffmpeg/Encoder.h"
#include "ffmpeg/InputFormat.h"

#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <optional>
#include <cstdlib>
#include <thread>

class SourceWorker {
public:
	enum class InputType { File, V4L2Camera };

	enum class SourceKind { File, Camera };

	struct Config {
		uint32_t trackIndex = 0;
		uint32_t ssrc = 0;
		uint8_t payloadType = 0;
		InputType inputType = InputType::File;
		std::string inputPath;             // file path or "/dev/video0"
		int captureWidth = 1280;           // V4L2 requested resolution
		int captureHeight = 720;
		int captureFps = 25;               // V4L2 requested framerate
		int initialBitrate = 900000;
		int initialFps = 25;
		double scaleResolutionDownBy = 1.0;
	};

	// Queues (set before start)
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity>* outputQueue = nullptr;
	mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity>* controlQueue = nullptr;
	mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity>* networkCmdQueue = nullptr;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity>* ackQueue = nullptr;
	std::function<void()> networkWakeupFn;

	explicit SourceWorker(const Config& cfg) : cfg_(cfg) {}
	~SourceWorker() { stop(); }

	void start() {
		if (running_.load()) return;
		running_ = true;
		thread_ = std::thread([this]() {
			try {
				if (cfg_.inputType == InputType::V4L2Camera)
					loopCamera();
				else
					loopFile();
			} catch (const std::exception& e) {
				spdlog::error("[src:{}] worker terminated after runtime failure: {}",
					cfg_.trackIndex, e.what());
			} catch (...) {
				spdlog::error("[src:{}] worker terminated after unknown runtime failure",
					cfg_.trackIndex);
			}
			running_ = false;
		});
	}

	void stop() {
		running_ = false;
		if (thread_.joinable()) thread_.join();
	}

private:
	// ─── Shared helpers ───────────────────────────────────

	static std::optional<uint32_t> loadOptionalTrackIndexEnv(const char* name) {
		const char* raw = loadTestHookEnv(name);
		if (!raw || std::strlen(raw) == 0) return std::nullopt;
		char* end = nullptr;
		long parsed = std::strtol(raw, &end, 10);
		if (!end || *end != '\0' || parsed < 0) return std::nullopt;
		return static_cast<uint32_t>(parsed);
	}

	bool initEncoder(int width, int height, int fps, int bitrate) {
		try {
			auto enc = mediasoup::ffmpeg::Encoder::Create(AV_CODEC_ID_H264,
				[width, height, fps, bitrate](AVCodecContext* ctx) {
					ctx->width = width;
					ctx->height = height;
					ctx->pix_fmt = AV_PIX_FMT_YUV420P;
					ctx->time_base = {1, std::max(1, fps)};
					ctx->framerate = {std::max(1, fps), 1};
					ctx->bit_rate = bitrate;
					ctx->rc_max_rate = bitrate;
					ctx->rc_buffer_size = bitrate;
					ctx->gop_size = std::max(1, fps);
					ctx->max_b_frames = 0;
					av_opt_set(ctx->priv_data, "preset", "ultrafast", 0);
					av_opt_set(ctx->priv_data, "tune", "zerolatency", 0);
					av_opt_set(ctx->priv_data, "profile", "baseline", 0);
				});
			encoder_ = std::move(enc);
		} catch (const std::exception& e) {
			spdlog::error("[src:{}] encoder creation failed: {}", cfg_.trackIndex, e.what());
			return false;
		}

		scaledFrame_ = mediasoup::ffmpeg::MakeFrame();
		scaledFrame_->format = AV_PIX_FMT_YUV420P;
		scaledFrame_->width = width;
		scaledFrame_->height = height;
		try {
			mediasoup::ffmpeg::FrameGetBuffer(scaledFrame_.get(), 32);
		} catch (const std::exception& e) {
			spdlog::error("[src:{}] scaled frame buffer allocation failed: {}", cfg_.trackIndex, e.what());
			encoder_ = mediasoup::ffmpeg::Encoder();
			scaledFrame_.reset();
			return false;
		}

		swsCtx_.reset();
		fps_ = std::max(1, fps);
		bitrate_ = bitrate;
		encoderRecreated_ = true;
		return true;
	}

	void sendAck(mt::TrackCommandType type, bool applied, uint64_t commandId, const char* reason = "") {
		if (!ackQueue) return;
		mt::CommandAck ack;
		ack.trackIndex = cfg_.trackIndex;
		ack.type = type;
		ack.commandId = commandId;
		ack.configGeneration = configGeneration_;
		ack.applied = applied;
		ack.reason = reason;
		ack.appliedAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
		if (encoder_) {
			ack.actualBitrateBps = bitrate_;
			ack.actualFps = fps_;
			ack.actualWidth = encoder_.width();
			ack.actualHeight = encoder_.height();
			ack.actualScale = scaleDown_;
		}
		ackQueue->tryPush(std::move(ack));
		if (type == mt::TrackCommandType::SetEncodingParameters) {
			spdlog::debug("[THREADED_ACK] track={} cmdId={} gen={} applied={} br={} fps={} scale={:.2f} reason={}",
				cfg_.trackIndex,
				commandId,
				configGeneration_,
				applied ? 1 : 0,
				bitrate_,
				fps_,
				scaleDown_,
				reason ? reason : "");
		}
	}

	void drainCommands() {
		// Drain all control commands, apply latest-wins for SetEncodingParameters (§15)
		mt::TrackControlCommand cmd;
		std::optional<mt::TrackControlCommand> latestEncoding;
		while (controlQueue && controlQueue->tryPop(cmd)) {
			switch (cmd.type) {
			case mt::TrackCommandType::SetEncodingParameters:
				latestEncoding = cmd; // overwrite — latest wins
				break;
			case mt::TrackCommandType::ForceKeyframe:
				forceKeyframe_ = true;
				sendAck(cmd.type, true, cmd.commandId);
				break;
			case mt::TrackCommandType::PauseTrack:
				paused_ = true;
				break;
			case mt::TrackCommandType::ResumeTrack:
				paused_ = false; forceKeyframe_ = true;
				break;
			case mt::TrackCommandType::StopSource:
				running_ = false; break;
			}
		}
		// Apply the latest encoding command (if any)
		if (latestEncoding.has_value()) {
			auto& le = *latestEncoding;
			if (le.configGeneration != configGeneration_) {
				sendAck(le.type, false, le.commandId, "stale-config-generation");
				return;
			}
			if (!rejectedFirstSetEncoding_
				&& rejectFirstSetEncodingTrackIndex_.has_value()
				&& *rejectFirstSetEncodingTrackIndex_ == cfg_.trackIndex) {
				rejectedFirstSetEncoding_ = true;
				sendAck(le.type, false, le.commandId, "test-reject-first-set-encoding");
				return;
			}
			int w = mediasoup::scaledDimension(sourceWidth_, le.scaleResolutionDownBy);
			int h = mediasoup::scaledDimension(sourceHeight_, le.scaleResolutionDownBy);
			bool needRecreate = !encoder_ || w != encoder_.width() || h != encoder_.height() || le.fps != fps_;
			bool ok = true;
			if (needRecreate) {
				ok = initEncoder(w, h, le.fps, le.bitrateBps);
				if (ok) {
					forceKeyframe_ = true;
					configGeneration_++;
				}
			} else if (encoder_) {
				bitrate_ = le.bitrateBps;
				encoder_.setBitRate(bitrate_);
			}
			scaleDown_ = le.scaleResolutionDownBy;
			sendAck(le.type, ok, le.commandId, ok ? "" : "encoder-recreate-failed");
		}
		// Network commands (PLI/FIR forwarded from network thread)
		mt::NetworkToSourceCommand ncmd;
		while (networkCmdQueue && networkCmdQueue->tryPop(ncmd)) {
			if (ncmd.type == mt::NetworkToSourceCommand::ForceKeyframe) {
				forceKeyframe_ = true;
			} else if (ncmd.type == mt::NetworkToSourceCommand::PauseTrack) {
				paused_ = true;
			} else if (ncmd.type == mt::NetworkToSourceCommand::ResumeTrack) {
				paused_ = false;
				forceKeyframe_ = true;
			}
		}
	}

	void encodeAndEnqueue(AVFrame* vframe, double ptsSec) {
		if (!encoder_ || paused_) return;

		uint32_t rtpTs = (uint32_t)(ptsSec * 90000);
		AVFrame* frameToEncode = vframe;

		if (encoder_.width() != vframe->width || encoder_.height() != vframe->height
			|| vframe->format != AV_PIX_FMT_YUV420P) {
			try {
				mediasoup::ffmpeg::FrameMakeWritable(scaledFrame_.get());
			} catch (const std::exception& e) {
				spdlog::warn("[src:{}] frame make writable failed: {}", cfg_.trackIndex, e.what());
				return;
			}
			SwsContext* raw = swsCtx_.release();
			SwsContext* result = sws_getCachedContext(raw,
				vframe->width, vframe->height, (AVPixelFormat)vframe->format,
				encoder_.width(), encoder_.height(), AV_PIX_FMT_YUV420P,
				SWS_BILINEAR, nullptr, nullptr, nullptr);
			swsCtx_.reset(result);
			if (!swsCtx_) return;
			sws_scale(swsCtx_.get(), vframe->data, vframe->linesize, 0, vframe->height,
				scaledFrame_->data, scaledFrame_->linesize);
			scaledFrame_->pts = vframe->pts;
			frameToEncode = scaledFrame_.get();
		}

		if (forceKeyframe_) {
			frameToEncode->pict_type = AV_PICTURE_TYPE_I;
			forceKeyframe_ = false;
		} else {
			frameToEncode->pict_type = AV_PICTURE_TYPE_NONE;
		}

		if (encoder_.SendFrame(frameToEncode)) {
			auto encPkt = mediasoup::ffmpeg::MakePacket();
			while (encoder_.ReceivePacket(encPkt.get())) {
				enqueueEncoded(encPkt->data, encPkt->size, rtpTs,
					(encPkt->flags & AV_PKT_FLAG_KEY) != 0);
				mediasoup::ffmpeg::PacketUnref(encPkt.get());
			}
		}
	}

	void enqueueEncoded(const uint8_t* data, size_t size, uint32_t rtpTs, bool isKey) {
		if (!outputQueue) return;
		mt::EncodedAccessUnit au;
		au.trackIndex = cfg_.trackIndex;
		au.ssrc = cfg_.ssrc;
		au.payloadType = cfg_.payloadType;
		au.rtpTimestamp = rtpTs;
		au.isKeyframe = isKey;
		au.encoderRecreated = encoderRecreated_;
		au.configGeneration = configGeneration_;
		au.assign(data, size);
		if (encoderRecreated_) encoderRecreated_ = false;
		outputQueue->tryPush(std::move(au)); // drop on full (backpressure)
		if (networkWakeupFn) networkWakeupFn();
	}

	// ─── Common decode/encode loop ─────────────────────────

	void runLoop(SourceKind kind,
		mediasoup::ffmpeg::InputFormat fmtCtx, int vidIdx) {
		auto* par = fmtCtx.StreamAt(vidIdx)->codecpar;
		sourceWidth_ = par->width;
		sourceHeight_ = par->height;

		if (kind == SourceKind::Camera) {
			spdlog::info("[src:{}] camera {} opened: {}x{} codec={}",
				cfg_.trackIndex, cfg_.inputPath, sourceWidth_, sourceHeight_, static_cast<int>(par->codec_id));
		}

		mediasoup::ffmpeg::Decoder vdec;
		try {
			vdec = mediasoup::ffmpeg::Decoder::OpenFromParameters(par);
		} catch (const std::exception& e) {
			spdlog::error("[src:{}] decoder init failed for {} source: {}",
				cfg_.trackIndex,
				kind == SourceKind::Camera ? "camera" : "file",
				e.what());
			running_ = false;
			return;
		}

		int w = mediasoup::scaledDimension(sourceWidth_, cfg_.scaleResolutionDownBy);
		int h = mediasoup::scaledDimension(sourceHeight_, cfg_.scaleResolutionDownBy);
		if (!initEncoder(w, h, cfg_.initialFps, cfg_.initialBitrate)) {
			spdlog::error("[src:{}] encoder initialization failed for {} source",
				cfg_.trackIndex,
				kind == SourceKind::Camera ? "camera" : "file");
			running_ = false;
			return;
		}
		scaleDown_ = cfg_.scaleResolutionDownBy;

		auto vframe = mediasoup::ffmpeg::MakeFrame();
		auto pkt = mediasoup::ffmpeg::MakePacket();
		auto t0 = std::chrono::steady_clock::now();
		double firstPts = -1;
		double nextEncodePts = -1;
		int64_t frameCount = 0;

		while (running_.load() && fmtCtx.ReadPacket(pkt.get())) {
			if (pkt->stream_index != vidIdx) { mediasoup::ffmpeg::PacketUnref(pkt.get()); continue; }
			drainCommands();
			if (!running_.load()) break;

			// File: pace to source clock; Camera: real-time, no pacing
			double pts = 0;
			if (kind == SourceKind::File) {
				pts = pkt->pts * av_q2d(fmtCtx.StreamAt(vidIdx)->time_base);
				if (firstPts < 0) firstPts = pts;
				auto target = t0 + std::chrono::microseconds((int64_t)((pts - firstPts) * 1e6));
				std::this_thread::sleep_until(target);
			}

			if (paused_) { mediasoup::ffmpeg::PacketUnref(pkt.get()); continue; }

			// Camera: compute wall-clock pts for RTP timestamp
			if (kind == SourceKind::Camera) {
				auto now = std::chrono::steady_clock::now();
				pts = std::chrono::duration<double>(now - t0).count();
			}

			if (vdec.SendPacket(pkt.get())) {
				while (vdec.ReceiveFrame(vframe.get())) {
					double framePts = pts;
					if (kind == SourceKind::File) {
						if (vframe->best_effort_timestamp != AV_NOPTS_VALUE)
							framePts = vframe->best_effort_timestamp * av_q2d(fmtCtx.StreamAt(vidIdx)->time_base);

						if (fps_ > 0 && !forceKeyframe_) {
							if (nextEncodePts < 0) nextEncodePts = framePts;
							if (framePts + 1e-6 < nextEncodePts) continue;
							nextEncodePts = framePts + 1.0 / fps_;
						}
					}

					encodeAndEnqueue(vframe.get(), framePts);
					if (kind == SourceKind::Camera) frameCount++;
				}
			}
			mediasoup::ffmpeg::PacketUnref(pkt.get());

			if (kind == SourceKind::Camera && frameCount % 100 == 0) {
				spdlog::info("[src:{}] camera frames captured: {}", cfg_.trackIndex, frameCount);
			}
		}

		spdlog::info("[src:{}] {} worker finished", cfg_.trackIndex,
			kind == SourceKind::Camera ? "camera" : "file");
	}

	// ─── File source loop ─────────────────────────────────

	void loopFile() {
		mediasoup::ffmpeg::InputFormat fmtCtx;
		try {
			fmtCtx = mediasoup::ffmpeg::InputFormat::Open(cfg_.inputPath);
		} catch (const std::exception& e) {
			spdlog::error("[src:{}] cannot open {}: {}", cfg_.trackIndex, cfg_.inputPath, e.what());
			running_ = false; return;
		}
		fmtCtx.FindStreamInfo();

		int vidIdx = fmtCtx.FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		if (vidIdx < 0) {
			spdlog::error("[src:{}] no video stream", cfg_.trackIndex);
			running_ = false; return;
		}

		runLoop(SourceKind::File, std::move(fmtCtx), vidIdx);
	}

	// ─── V4L2 camera source loop ─────────────────────────

	void loopCamera() {
		avdevice_register_all();

		auto* v4l2Fmt = av_find_input_format("v4l2");
		if (!v4l2Fmt) {
			spdlog::error("[src:{}] v4l2 input format not available", cfg_.trackIndex);
			running_ = false; return;
		}

		AVDictionary* opts = nullptr;
		char sizeBuf[32], fpsBuf[16];
		snprintf(sizeBuf, sizeof(sizeBuf), "%dx%d", cfg_.captureWidth, cfg_.captureHeight);
		snprintf(fpsBuf, sizeof(fpsBuf), "%d", cfg_.captureFps);
		av_dict_set(&opts, "video_size", sizeBuf, 0);
		av_dict_set(&opts, "framerate", fpsBuf, 0);
		av_dict_set(&opts, "input_format", "mjpeg", 0);

		mediasoup::ffmpeg::InputFormat fmtCtx;
		try {
			fmtCtx = mediasoup::ffmpeg::InputFormat::OpenWithFormat(cfg_.inputPath, v4l2Fmt, &opts);
		} catch (...) {
			// Retry without MJPEG preference (fallback to YUYV etc)
			av_dict_free(&opts);
			opts = nullptr;
			av_dict_set(&opts, "video_size", sizeBuf, 0);
			av_dict_set(&opts, "framerate", fpsBuf, 0);
			try {
				fmtCtx = mediasoup::ffmpeg::InputFormat::OpenWithFormat(cfg_.inputPath, v4l2Fmt, &opts);
			} catch (const std::exception& e) {
				spdlog::error("[src:{}] cannot open camera {}: {}", cfg_.trackIndex, cfg_.inputPath, e.what());
				av_dict_free(&opts);
				running_ = false; return;
			}
		}
		av_dict_free(&opts);
		fmtCtx.FindStreamInfo();

		int vidIdx = fmtCtx.FindFirstStreamIndex(AVMEDIA_TYPE_VIDEO);
		if (vidIdx < 0) {
			spdlog::error("[src:{}] no video stream from camera", cfg_.trackIndex);
			running_ = false; return;
		}

		runLoop(SourceKind::Camera, std::move(fmtCtx), vidIdx);
	}

	Config cfg_;
	std::atomic<bool> running_{false};
	std::thread thread_;

	mediasoup::ffmpeg::Encoder encoder_;
	mediasoup::ffmpeg::FramePtr scaledFrame_;
	mediasoup::ffmpeg::SwsContextPtr swsCtx_;
	int fps_ = 25;
	int bitrate_ = 900000;
	double scaleDown_ = 1.0;
	int sourceWidth_ = 0;
	int sourceHeight_ = 0;
	bool forceKeyframe_ = false;
	bool paused_ = false;
	bool encoderRecreated_ = false;
	uint64_t configGeneration_ = 0;
	std::optional<uint32_t> rejectFirstSetEncodingTrackIndex_ =
		loadOptionalTrackIndexEnv("QOS_TEST_REJECT_FIRST_SET_ENCODING_TRACK_INDEX");
	bool rejectedFirstSetEncoding_ = false;
};
