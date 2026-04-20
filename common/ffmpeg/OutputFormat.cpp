#include "ffmpeg/OutputFormat.h"

#include "ffmpeg/AvError.h"

#include <cstdio>
#include <stdexcept>

namespace mediasoup::ffmpeg {

OutputFormat::OutputFormat(AVFormatContext* ctx, std::string outputPath)
	: ctx_(ctx)
	, outputPath_(std::move(outputPath))
{
}

OutputFormat OutputFormat::Create(
	const std::string& formatName,
	const std::string& outputPath)
{
	AVFormatContext* ctx = nullptr;
	avformat_alloc_output_context2(
		&ctx,
		nullptr,
		formatName.empty() ? nullptr : formatName.c_str(),
		outputPath.c_str());
	if (!ctx)
		throw std::runtime_error("avformat_alloc_output_context2(" + outputPath + ") failed");
	return OutputFormat(ctx, outputPath);
}

OutputFormat::~OutputFormat()
{
	Close();
}

OutputFormat::OutputFormat(OutputFormat&& other) noexcept
	: ctx_(other.ctx_)
	, outputPath_(std::move(other.outputPath_))
	, ioOpened_(other.ioOpened_)
	, headerWritten_(other.headerWritten_)
{
	other.ctx_ = nullptr;
	other.ioOpened_ = false;
	other.headerWritten_ = false;
}

OutputFormat& OutputFormat::operator=(OutputFormat&& other) noexcept
{
	if (this == &other) return *this;
	Close();
	ctx_ = other.ctx_;
	outputPath_ = std::move(other.outputPath_);
	ioOpened_ = other.ioOpened_;
	headerWritten_ = other.headerWritten_;
	other.ctx_ = nullptr;
	other.ioOpened_ = false;
	other.headerWritten_ = false;
	return *this;
}

AVStream* OutputFormat::NewStream()
{
	AVStream* stream = avformat_new_stream(ctx_, nullptr);
	if (!stream)
		throw std::runtime_error("avformat_new_stream failed for " + outputPath_);
	return stream;
}

void OutputFormat::OpenIo()
{
	CheckError(avio_open(&ctx_->pb, outputPath_.c_str(), AVIO_FLAG_WRITE),
		"avio_open(" + outputPath_ + ")");
	ioOpened_ = true;
}

void OutputFormat::WriteHeader()
{
	CheckError(avformat_write_header(ctx_, nullptr), "avformat_write_header(" + outputPath_ + ")");
	headerWritten_ = true;
}

void OutputFormat::WriteTrailer()
{
	if (!ctx_ || !headerWritten_) return;
	CheckError(av_write_trailer(ctx_), "av_write_trailer(" + outputPath_ + ")");
	headerWritten_ = false;
}

void OutputFormat::WriteInterleavedFrame(AVPacket* packet)
{
	CheckError(av_interleaved_write_frame(ctx_, packet),
		"av_interleaved_write_frame(" + outputPath_ + ")");
}

void OutputFormat::Close()
{
	if (!ctx_) return;
	if (headerWritten_) {
		const int err = av_write_trailer(ctx_);
		if (err < 0) {
			std::fprintf(
				stderr,
				"OutputFormat::Close av_write_trailer(%s) failed: %s\n",
				outputPath_.c_str(),
				ErrorToString(err).c_str());
		}
		headerWritten_ = false;
	}
	if (ioOpened_ && ctx_->pb)
		avio_closep(&ctx_->pb);
	avformat_free_context(ctx_);
	ctx_ = nullptr;
	ioOpened_ = false;
}

} // namespace mediasoup::ffmpeg
