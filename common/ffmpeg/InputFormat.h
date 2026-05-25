#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <functional>
#include <string>

namespace mediasoup::ffmpeg {

using InputInterruptCallback = std::function<bool()>;

class InputFormat {
public:
	static InputFormat Open(const std::string& path);
	static InputFormat OpenWithFormat(const std::string& path,
		const AVInputFormat* fmt, AVDictionary** opts);
	static InputFormat OpenWithFormatInterruptible(const std::string& path,
		const AVInputFormat* fmt, AVDictionary** opts, InputInterruptCallback interrupt);

	#ifdef MEDIASOUP_TEST_HOOKS
	static int SetReadFailureCountdown(int readsBeforeFailure);
	static InputFormat CreateForTesting(AVFormatContext* ctx, InputInterruptCallback interrupt);
	#endif

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
	explicit InputFormat(AVFormatContext* ctx, InputInterruptCallback interrupt);

	AVFormatContext* ctx_{nullptr};
	InputInterruptCallback interrupt_;
};

} // namespace mediasoup::ffmpeg
