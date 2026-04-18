#pragma once

#include <App.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace mediasoup {

namespace fs = std::filesystem;

struct ResolvedFile {
	fs::path path;
	uintmax_t size = 0;
};

inline bool IsPathWithin(const fs::path& base, const fs::path& candidate)
{
	auto baseIt = base.begin();
	auto candidateIt = candidate.begin();
	for (; baseIt != base.end() && candidateIt != candidate.end(); ++baseIt, ++candidateIt) {
		if (*baseIt != *candidateIt) {
			return false;
		}
	}

	return baseIt == base.end();
}

inline std::optional<ResolvedFile> ResolveFileUnderBase(
	const fs::path& baseDir,
	std::string_view requestPath,
	bool& forbidden)
{
	forbidden = false;
	std::error_code ec;
	fs::path base = fs::weakly_canonical(baseDir, ec);
	if (ec) {
		return std::nullopt;
	}

	fs::path relative = fs::path(std::string(requestPath)).relative_path();
	fs::path normalized = (base / relative).lexically_normal();
	if (!IsPathWithin(base, normalized)) {
		forbidden = true;
		return std::nullopt;
	}
	if (!fs::exists(normalized, ec) || ec) {
		return std::nullopt;
	}
	if (!fs::is_regular_file(normalized, ec) || ec) {
		return std::nullopt;
	}

	fs::path canonical = fs::weakly_canonical(normalized, ec);
	if (ec) {
		return std::nullopt;
	}
	if (!IsPathWithin(base, canonical)) {
		forbidden = true;
		return std::nullopt;
	}

	auto size = fs::file_size(canonical, ec);
	if (ec) {
		return std::nullopt;
	}

	return ResolvedFile{ canonical, size };
}

inline std::string ContentTypeForPath(std::string_view path)
{
	if (path.find(".webm") != std::string::npos) return "video/webm";
	if (path.find(".json") != std::string::npos) return "application/json";
	if (path.find(".html") != std::string::npos) return "text/html";
	if (path.find(".js") != std::string::npos) return "application/javascript";
	if (path.find(".css") != std::string::npos) return "text/css";
	return "application/octet-stream";
}

template <bool SSL>
struct FileStreamState {
	static constexpr size_t kChunkBytes = 64 * 1024;

	std::ifstream file;
	std::array<char, kChunkBytes> buffer{};
	uintmax_t totalSize = 0;
	std::shared_ptr<std::atomic<bool>> aborted = std::make_shared<std::atomic<bool>>(false);
};

template <bool SSL>
void StreamResolvedFile(
	uWS::HttpResponse<SSL>* res,
	std::shared_ptr<FileStreamState<SSL>> state)
{
	while (!state->aborted->load(std::memory_order_relaxed)) {
		uintmax_t offset = res->getWriteOffset();
		if (offset >= state->totalSize) {
			return;
		}

		state->file.clear();
		state->file.seekg(static_cast<std::streamoff>(offset));
		if (!state->file.good()) {
			res->close();
			return;
		}

		auto remaining = state->totalSize - offset;
		auto toRead = std::min<uintmax_t>(state->buffer.size(), remaining);
		state->file.read(state->buffer.data(), static_cast<std::streamsize>(toRead));
		auto readBytes = state->file.gcount();
		if (readBytes <= 0) {
			res->close();
			return;
		}

		auto [ok, done] = res->tryEnd(
			std::string_view(state->buffer.data(), static_cast<size_t>(readBytes)),
			state->totalSize);
		if (done) {
			return;
		}
		if (!ok) {
			res->onWritable([res, state](int) {
				StreamResolvedFile(res, state);
				return false;
			});
			return;
		}
	}
}

template <bool SSL>
void ServeResolvedFile(
	uWS::HttpResponse<SSL>* res,
	const ResolvedFile& resolved,
	const std::string& contentType)
{
	static constexpr size_t kInlineReadBytes = 128 * 1024;

	if (resolved.size <= kInlineReadBytes) {
		std::ifstream file(resolved.path, std::ios::binary);
		if (!file.is_open()) {
			res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}

		std::string content(
			(std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		res->writeHeader("Content-Type", contentType);
		res->end(content);
		return;
	}

	auto state = std::make_shared<FileStreamState<SSL>>();
	state->totalSize = resolved.size;
	state->file.open(resolved.path, std::ios::binary);
	if (!state->file.is_open()) {
		res->writeStatus("404 Not Found")->end("Not Found");
		return;
	}

	res->writeHeader("Content-Type", contentType);
	auto aborted = state->aborted;
	res->onAborted([aborted] {
		aborted->store(true, std::memory_order_relaxed);
	});
	StreamResolvedFile(res, state);
}

} // namespace mediasoup
