#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

namespace mediasoup::ffmpeg {

class OutputFormat {
public:
	static OutputFormat Create(
		const std::string& formatName,
		const std::string& outputPath);

	OutputFormat() = default;
	~OutputFormat();

	OutputFormat(const OutputFormat&) = delete;
	OutputFormat& operator=(const OutputFormat&) = delete;
	OutputFormat(OutputFormat&& other) noexcept;
	OutputFormat& operator=(OutputFormat&& other) noexcept;

	AVFormatContext* get() const { return ctx_; }
	AVStream* NewStream();
	void OpenIo();
	void WriteHeader();
	void WriteTrailer();
	void WriteInterleavedFrame(AVPacket* packet);
	bool headerWritten() const { return headerWritten_; }
	bool ioOpened() const { return ioOpened_; }
	void Close();

private:
	OutputFormat(AVFormatContext* ctx, std::string outputPath);

	AVFormatContext* ctx_{nullptr};
	std::string outputPath_;
	bool ioOpened_{false};
	bool headerWritten_{false};
};

} // namespace mediasoup::ffmpeg
