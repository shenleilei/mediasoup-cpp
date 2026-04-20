#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

namespace mediasoup::ffmpeg {

class InputFormat {
public:
	static InputFormat Open(const std::string& path);

	InputFormat() = default;
	~InputFormat();

	InputFormat(const InputFormat&) = delete;
	InputFormat& operator=(const InputFormat&) = delete;
	InputFormat(InputFormat&& other) noexcept;
	InputFormat& operator=(InputFormat&& other) noexcept;

	AVFormatContext* get() const { return ctx_; }
	void FindStreamInfo();
	int FindFirstStreamIndex(AVMediaType mediaType) const;
	AVStream* StreamAt(int index) const;
	bool ReadPacket(AVPacket* packet);
	void Close();

private:
	explicit InputFormat(AVFormatContext* ctx);

	AVFormatContext* ctx_{nullptr};
};

} // namespace mediasoup::ffmpeg
