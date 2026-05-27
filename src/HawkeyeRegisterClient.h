#pragma once

#include <atomic>
#include <string>
#include <thread>
#include <string_view>

namespace mediasoup {

class HawkeyeRegisterClient {
public:
	HawkeyeRegisterClient(std::string registerUrl, std::string server, std::string type);
	~HawkeyeRegisterClient();

	HawkeyeRegisterClient(const HawkeyeRegisterClient&) = delete;
	HawkeyeRegisterClient& operator=(const HawkeyeRegisterClient&) = delete;

	void start();
	void stop();

	bool enabled() const;

private:
	struct ParsedUrl {
		bool secure{ false };
		std::string host;
		std::string port;
		int portNumber{ 0 };
		std::string path{ "/" };
	};

	struct Frame {
		uint8_t opcode{ 0 };
		std::string payload;
	};

	void run();
	bool connectAndRegister();
	bool openSocket(const ParsedUrl& url, int& fd, std::string& error) const;
	bool performHandshake(int fd, const ParsedUrl& url, const std::string& key, std::string& error) const;
	bool sendRegisterFrame(int fd, std::string& error) const;
	bool sendFrame(int fd, uint8_t opcode, std::string_view payload, std::string& error) const;
	bool readLoop(int fd, std::string& error);
	bool readFrame(std::string& buffered, Frame& frame) const;
	static std::string buildWsKey();
	static std::string base64Encode(const unsigned char* data, size_t len);
	static std::string sha1Base64(const std::string& value);
	static bool parseUrl(const std::string& value, ParsedUrl& out, std::string& error);
	static std::string toLowerCopy(std::string value);
	static std::string trimCopy(const std::string& value);
	static std::string headerValue(const std::string& headers, const std::string& name);

	std::string registerUrl_;
	std::string server_;
	std::string type_;
	std::atomic<bool> stop_{ false };
	std::thread thread_;
	std::atomic<int> socketFd_{ -1 };
};

} // namespace mediasoup
