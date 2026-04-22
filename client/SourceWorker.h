// SourceWorker.h — Per-video-source worker thread: capture/decode/scale/encode → EncodedAccessUnit
// Supports file input (MP4) and V4L2 camera input.
// Phase 3 of linux-client-multi-source-thread-model migration.
#pragma once

#include "ThreadTypes.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
}

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <optional>
#include <cstdlib>
#include <thread>

class SourceWorker {
public:
	enum class InputType { File, V4L2Camera };

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
			if (cfg_.inputType == InputType::V4L2Camera)
				loopCamera();
			else
				loopFile();
		});
	}

	void stop() {
		running_ = false;
		if (thread_.joinable()) thread_.join();
	}

private:
	// ─── Shared helpers ───────────────────────────────────

	static int scaledDim(int src, double scale) {
		int s = (int)(src / std::max(1.0, scale));
		if (s < 2) s = 2;
		if (s % 2) s--;
		return std::max(2, s);
	}

	bool initEncoder(int width, int height, int fps, int bitrate) {
		auto* enc = avcodec_find_encoder(AV_CODEC_ID_H264);
		if (!enc) return false;
		auto* ctx = avcodec_alloc_context3(enc);
		if (!ctx) return false;
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
		if (avcodec_open2(ctx, enc, nullptr) < 0) {
			avcodec_free_context(&ctx);
			return false;
		}
		if (encoder_) avcodec_free_context(&encoder_);
		encoder_ = ctx;

		if (scaledFrame_) av_frame_free(&scaledFrame_);
		scaledFrame_ = av_frame_alloc();
		scaledFrame_->format = AV_PIX_FMT_YUV420P;
		scaledFrame_->width = width;
		scaledFrame_->height = height;
		av_frame_get_buffer(scaledFrame_, 32);

		if (swsCtx_) { sws_freeContext(swsCtx_); swsCtx_ = nullptr; }
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
			ack.actualWidth = encoder_->width;
			ack.actualHeight = encoder_->height;
			ack.actualScale = scaleDown_;
		}
		ackQueue->tryPush(std::move(ack));
		if (type == mt::TrackCommandType::SetEncodingParameters) {
			printf("[THREADED_ACK] track=%u cmdId=%llu gen=%llu applied=%d br=%d fps=%d scale=%.2f reason=%s\n",
				cfg_.trackIndex,
				static_cast<unsigned long long>(commandId),
				static_cast<unsigned long long>(configGeneration_),
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
			int w = scaledDim(sourceWidth_, le.scaleResolutionDownBy);
			int h = scaledDim(sourceHeight_, le.scaleResolutionDownBy);
			bool needRecreate = !encoder_ || w != encoder_->width || h != encoder_->height || le.fps != fps_;
			bool ok = true;
			if (needRecreate) {
				ok = initEncoder(w, h, le.fps, le.bitrateBps);
				if (ok) {
					forceKeyframe_ = true;
					configGeneration_++;
				}
			} else if (encoder_) {
				bitrate_ = le.bitrateBps;
				encoder_->bit_rate = bitrate_;
				encoder_->rc_max_rate = bitrate_;
				encoder_->rc_buffer_size = bitrate_;
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

		if (encoder_->width != vframe->width || encoder_->height != vframe->height
			|| vframe->format != AV_PIX_FMT_YUV420P) {
			av_frame_make_writable(scaledFrame_);
			swsCtx_ = sws_getCachedContext(swsCtx_,
				vframe->width, vframe->height, (AVPixelFormat)vframe->format,
				encoder_->width, encoder_->height, AV_PIX_FMT_YUV420P,
				SWS_BILINEAR, nullptr, nullptr, nullptr);
			if (!swsCtx_) return;
			sws_scale(swsCtx_, vframe->data, vframe->linesize, 0, vframe->height,
				scaledFrame_->data, scaledFrame_->linesize);
			scaledFrame_->pts = vframe->pts;
			frameToEncode = scaledFrame_;
		}

		if (forceKeyframe_) {
			frameToEncode->pict_type = AV_PICTURE_TYPE_I;
			forceKeyframe_ = false;
		} else {
			frameToEncode->pict_type = AV_PICTURE_TYPE_NONE;
		}

		if (avcodec_send_frame(encoder_, frameToEncode) == 0) {
			AVPacket* encPkt = av_packet_alloc();
			while (avcodec_receive_packet(encoder_, encPkt) == 0) {
				enqueueEncoded(encPkt->data, encPkt->size, rtpTs,
					(encPkt->flags & AV_PKT_FLAG_KEY) != 0);
				av_packet_unref(encPkt);
			}
			av_packet_free(&encPkt);
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

	// ─── File source loop ─────────────────────────────────

	void loopFile() {
		AVFormatContext* fmtCtx = nullptr;
		if (avformat_open_input(&fmtCtx, cfg_.inputPath.c_str(), nullptr, nullptr) < 0) {
			printf("[src:%u] cannot open %s\n", cfg_.trackIndex, cfg_.inputPath.c_str());
			running_ = false; return;
		}
		avformat_find_stream_info(fmtCtx, nullptr);

		int vidIdx = -1;
		for (unsigned i = 0; i < fmtCtx->nb_streams; i++)
			if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vidIdx = i; break; }
		if (vidIdx < 0) {
			printf("[src:%u] no video stream\n", cfg_.trackIndex);
			avformat_close_input(&fmtCtx); running_ = false; return;
		}

		auto* par = fmtCtx->streams[vidIdx]->codecpar;
		sourceWidth_ = par->width;
		sourceHeight_ = par->height;

		auto* dec = avcodec_find_decoder(par->codec_id);
		AVCodecContext* vdec = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(vdec, par);
		avcodec_open2(vdec, dec, nullptr);

		int w = scaledDim(sourceWidth_, cfg_.scaleResolutionDownBy);
		int h = scaledDim(sourceHeight_, cfg_.scaleResolutionDownBy);
		initEncoder(w, h, cfg_.initialFps, cfg_.initialBitrate);
		scaleDown_ = cfg_.scaleResolutionDownBy;

		AVFrame* vframe = av_frame_alloc();
		AVPacket* pkt = av_packet_alloc();
		auto t0 = std::chrono::steady_clock::now();
		double firstPts = -1;
		double nextEncodePts = -1;

		while (running_.load() && av_read_frame(fmtCtx, pkt) >= 0) {
			if (pkt->stream_index != vidIdx) { av_packet_unref(pkt); continue; }
			drainCommands();
			if (!running_.load()) break;

			double pts = pkt->pts * av_q2d(fmtCtx->streams[vidIdx]->time_base);
			if (firstPts < 0) firstPts = pts;

			// Pace to source clock
			auto target = t0 + std::chrono::microseconds((int64_t)((pts - firstPts) * 1e6));
			std::this_thread::sleep_until(target);

			if (paused_) { av_packet_unref(pkt); continue; }

			if (avcodec_send_packet(vdec, pkt) == 0) {
				while (avcodec_receive_frame(vdec, vframe) == 0) {
					double framePts = pts;
					if (vframe->best_effort_timestamp != AV_NOPTS_VALUE)
						framePts = vframe->best_effort_timestamp * av_q2d(fmtCtx->streams[vidIdx]->time_base);

					if (fps_ > 0 && !forceKeyframe_) {
						if (nextEncodePts < 0) nextEncodePts = framePts;
						if (framePts + 1e-6 < nextEncodePts) continue;
						nextEncodePts = framePts + 1.0 / fps_;
					}
					encodeAndEnqueue(vframe, framePts);
				}
			}
			av_packet_unref(pkt);
		}

		av_packet_free(&pkt);
		av_frame_free(&vframe);
		if (encoder_) avcodec_free_context(&encoder_);
		if (scaledFrame_) av_frame_free(&scaledFrame_);
		if (swsCtx_) sws_freeContext(swsCtx_);
		avcodec_free_context(&vdec);
		avformat_close_input(&fmtCtx);
		printf("[src:%u] file worker finished\n", cfg_.trackIndex);
	}

	// ─── V4L2 camera source loop ─────────────────────────

	void loopCamera() {
		avdevice_register_all();

		auto* v4l2Fmt = av_find_input_format("v4l2");
		if (!v4l2Fmt) {
			printf("[src:%u] v4l2 input format not available\n", cfg_.trackIndex);
			running_ = false; return;
		}

		AVDictionary* opts = nullptr;
		char sizeBuf[32], fpsBuf[16];
		snprintf(sizeBuf, sizeof(sizeBuf), "%dx%d", cfg_.captureWidth, cfg_.captureHeight);
		snprintf(fpsBuf, sizeof(fpsBuf), "%d", cfg_.captureFps);
		av_dict_set(&opts, "video_size", sizeBuf, 0);
		av_dict_set(&opts, "framerate", fpsBuf, 0);
		av_dict_set(&opts, "input_format", "mjpeg", 0); // prefer MJPEG for higher res

		AVFormatContext* fmtCtx = nullptr;
		if (avformat_open_input(&fmtCtx, cfg_.inputPath.c_str(), v4l2Fmt, &opts) < 0) {
			// Retry without MJPEG preference (fallback to YUYV etc)
			av_dict_free(&opts);
			opts = nullptr;
			av_dict_set(&opts, "video_size", sizeBuf, 0);
			av_dict_set(&opts, "framerate", fpsBuf, 0);
			if (avformat_open_input(&fmtCtx, cfg_.inputPath.c_str(), v4l2Fmt, &opts) < 0) {
				printf("[src:%u] cannot open camera %s\n", cfg_.trackIndex, cfg_.inputPath.c_str());
				av_dict_free(&opts);
				running_ = false; return;
			}
		}
		av_dict_free(&opts);
		avformat_find_stream_info(fmtCtx, nullptr);

		int vidIdx = -1;
		for (unsigned i = 0; i < fmtCtx->nb_streams; i++)
			if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { vidIdx = i; break; }
		if (vidIdx < 0) {
			printf("[src:%u] no video stream from camera\n", cfg_.trackIndex);
			avformat_close_input(&fmtCtx); running_ = false; return;
		}

		auto* par = fmtCtx->streams[vidIdx]->codecpar;
		sourceWidth_ = par->width;
		sourceHeight_ = par->height;
		printf("[src:%u] camera %s opened: %dx%d codec=%d\n",
			cfg_.trackIndex, cfg_.inputPath.c_str(), sourceWidth_, sourceHeight_, par->codec_id);

		// Decoder for camera frames (MJPEG, YUYV, etc)
		auto* dec = avcodec_find_decoder(par->codec_id);
		if (!dec) {
			printf("[src:%u] no decoder for camera codec %d\n", cfg_.trackIndex, par->codec_id);
			avformat_close_input(&fmtCtx); running_ = false; return;
		}
		AVCodecContext* vdec = avcodec_alloc_context3(dec);
		avcodec_parameters_to_context(vdec, par);
		avcodec_open2(vdec, dec, nullptr);

		int w = scaledDim(sourceWidth_, cfg_.scaleResolutionDownBy);
		int h = scaledDim(sourceHeight_, cfg_.scaleResolutionDownBy);
		initEncoder(w, h, cfg_.initialFps, cfg_.initialBitrate);
		scaleDown_ = cfg_.scaleResolutionDownBy;

		AVFrame* vframe = av_frame_alloc();
		AVPacket* pkt = av_packet_alloc();
		auto t0 = std::chrono::steady_clock::now();
		int64_t frameCount = 0;

		while (running_.load() && av_read_frame(fmtCtx, pkt) >= 0) {
			if (pkt->stream_index != vidIdx) { av_packet_unref(pkt); continue; }
			drainCommands();
			if (!running_.load()) break;

			if (paused_) { av_packet_unref(pkt); continue; }

			// Camera frames are real-time — no pacing needed, use wall clock for RTP ts
			auto now = std::chrono::steady_clock::now();
			double ptsSec = std::chrono::duration<double>(now - t0).count();
			uint32_t rtpTs = (uint32_t)(ptsSec * 90000);

			if (avcodec_send_packet(vdec, pkt) == 0) {
				while (avcodec_receive_frame(vdec, vframe) == 0) {
					encodeAndEnqueue(vframe, ptsSec);
					frameCount++;
				}
			}
			av_packet_unref(pkt);

			if (frameCount % 100 == 0) {
				printf("[src:%u] camera frames captured: %lld\n",
					cfg_.trackIndex, (long long)frameCount);
			}
		}

		av_packet_free(&pkt);
		av_frame_free(&vframe);
		if (encoder_) avcodec_free_context(&encoder_);
		if (scaledFrame_) av_frame_free(&scaledFrame_);
		if (swsCtx_) sws_freeContext(swsCtx_);
		avcodec_free_context(&vdec);
		avformat_close_input(&fmtCtx);
		printf("[src:%u] camera worker finished\n", cfg_.trackIndex);
	}

	Config cfg_;
	std::atomic<bool> running_{false};
	std::thread thread_;

	AVCodecContext* encoder_ = nullptr;
	AVFrame* scaledFrame_ = nullptr;
	SwsContext* swsCtx_ = nullptr;
	int fps_ = 25;
	int bitrate_ = 900000;
	double scaleDown_ = 1.0;
	int sourceWidth_ = 0;
	int sourceHeight_ = 0;
	bool forceKeyframe_ = false;
	bool paused_ = false;
	bool encoderRecreated_ = false;
	uint64_t configGeneration_ = 0;
};
