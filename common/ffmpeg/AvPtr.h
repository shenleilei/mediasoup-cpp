#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include "ffmpeg/AvError.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <memory>

namespace mediasoup::ffmpeg {

struct PacketDeleter {
	void operator()(AVPacket* packet) const {
		if (packet) av_packet_free(&packet);
	}
};

struct FrameDeleter {
	void operator()(AVFrame* frame) const {
		if (frame) av_frame_free(&frame);
	}
};

struct CodecContextDeleter {
	void operator()(AVCodecContext* codecContext) const {
		if (codecContext) avcodec_free_context(&codecContext);
	}
};

struct BitstreamFilterContextDeleter {
	void operator()(AVBSFContext* bsfContext) const {
		if (bsfContext) av_bsf_free(&bsfContext);
	}
};

using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;
using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
using BitstreamFilterContextPtr = std::unique_ptr<AVBSFContext, BitstreamFilterContextDeleter>;

inline PacketPtr MakePacket()
{
	return PacketPtr(av_packet_alloc());
}

inline FramePtr MakeFrame()
{
	return FramePtr(av_frame_alloc());
}

inline CodecContextPtr MakeCodecContext(const AVCodec* codec)
{
	return CodecContextPtr(avcodec_alloc_context3(codec));
}

inline BitstreamFilterContextPtr MakeBitstreamFilterContext(const AVBitStreamFilter* filter)
{
	AVBSFContext* context = nullptr;
	if (filter)
		CheckError(av_bsf_alloc(filter, &context), "av_bsf_alloc");
	return BitstreamFilterContextPtr(context);
}

inline void PacketUnref(AVPacket* packet)
{
	if (packet) av_packet_unref(packet);
}

inline void PacketRef(AVPacket* destination, const AVPacket* source)
{
	CheckError(av_packet_ref(destination, source), "av_packet_ref");
}

inline void FrameGetBuffer(AVFrame* frame, int align)
{
	CheckError(av_frame_get_buffer(frame, align), "av_frame_get_buffer");
}

inline void FrameMakeWritable(AVFrame* frame)
{
	CheckError(av_frame_make_writable(frame), "av_frame_make_writable");
}

} // namespace mediasoup::ffmpeg
