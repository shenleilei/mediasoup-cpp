#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

inline std::string testHttpsRequestRaw(
	const std::string& host,
	int port,
	const std::string& request)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return "";

	struct timeval tv{5, 0};
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
	if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
		::close(fd);
		return "";
	}

	SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
	if (!ctx) {
		::close(fd);
		return "";
	}
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

	SSL* ssl = SSL_new(ctx);
	if (!ssl) {
		SSL_CTX_free(ctx);
		::close(fd);
		return "";
	}
	SSL_set_fd(ssl, fd);

	std::string response;
	if (SSL_connect(ssl) == 1 &&
		SSL_write(ssl, request.data(), static_cast<int>(request.size())) > 0) {
		char buf[4096];
		while (true) {
			int n = SSL_read(ssl, buf, sizeof(buf));
			if (n <= 0) break;
			response.append(buf, n);
		}
	}

	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	::shutdown(fd, SHUT_RDWR);
	::close(fd);
	return response;
}

inline std::string testHttpsGetRaw(
	const std::string& host,
	int port,
	const std::string& path,
	const std::string& extraHeaders = "")
{
	std::string req = "GET " + path + " HTTP/1.1\r\n"
		"Host: " + host + "\r\n" +
		extraHeaders +
		"Connection: close\r\n\r\n";
	return testHttpsRequestRaw(host, port, req);
}

inline std::string testHttpBodyFromRawResponse(const std::string& response)
{
	auto pos = response.find("\r\n\r\n");
	if (pos == std::string::npos) return "";
	return response.substr(pos + 4);
}

inline std::string testHttpsGetBody(
	const std::string& host,
	int port,
	const std::string& path,
	const std::string& extraHeaders = "")
{
	return testHttpBodyFromRawResponse(testHttpsGetRaw(host, port, path, extraHeaders));
}
