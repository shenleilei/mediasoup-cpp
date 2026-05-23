#include "push/PushSignalingSession.h"

#include <stdexcept>

namespace webrtc_qos_plain {
namespace {

uint16_t JsonU16(const json& object, const char* key)
{
	const auto value = object.at(key).get<uint32_t>();
	if (value > 65535u) throw std::runtime_error(std::string(key) + " is out of range");
	return static_cast<uint16_t>(value);
}

uint8_t JsonU8(const json& object, const char* key)
{
	const auto value = object.at(key).get<uint32_t>();
	if (value > 255u) throw std::runtime_error(std::string(key) + " is out of range");
	return static_cast<uint8_t>(value);
}

} // namespace

PushSignalingSession::PushSignalingSession(std::shared_ptr<spdlog::logger> logger)
	: logger_(std::move(logger))
{
}

bool PushSignalingSession::ConnectAndPublish(const PushOptions& options, PublishInfo* info)
{
	if (!info) return false;
	if (!ws_.connect(options.serverIp, options.serverPort, "/ws")) {
		logger_->error("ws_connect_failed serverIp={} serverPort={}", options.serverIp, options.serverPort);
		return false;
	}
	logger_->info("ws_connected serverIp={} serverPort={}", options.serverIp, options.serverPort);

	ws_.setNotificationHandler([logger = logger_](const json& msg) {
		logger->info("ws_notification method={} data={}",
			msg.value("method", ""),
			msg.value("data", json::object()).dump());
	});

	try {
		ws_.request("join", {
			{"roomId", options.room},
			{"peerId", options.peer},
			{"displayName", options.peer},
			{"rtpCapabilities", json::object()}
		});
		logger_->info("join_ok roomId={} peerId={}", options.room, options.peer);

		const auto response = ws_.request("plainPublish", {
			{"videoCodec", "h264"},
			{"videoSsrc", options.videoSsrc},
			{"audioSsrc", options.audioSsrc}
		});

		if (response.value("videoCodec", "") != "h264")
			throw std::runtime_error("plainPublish returned non-H264 codec");
		auto tracks = response.value("videoTracks", json::array());
		if (!tracks.is_array() || tracks.size() != 1)
			throw std::runtime_error("plainPublish must return exactly one videoTracks[] item");
		const auto& track = tracks.at(0);

		PublishInfo parsed;
		parsed.transportId = response.at("transportId").get<std::string>();
		parsed.announcedIp = response.value("ip", "");
		parsed.port = JsonU16(response, "port");
		parsed.payloadType = JsonU8(track, "pt");
		parsed.ssrc = track.at("ssrc").get<uint32_t>();
		parsed.producerId = track.at("producerId").get<std::string>();
		parsed.transportCcExtId = JsonU8(track, "transportCcExtId");
		if (parsed.ssrc == 0 || parsed.payloadType == 0 || parsed.transportCcExtId == 0)
			throw std::runtime_error("plainPublish returned invalid SSRC/PT/TWCC ext id");

		*info = parsed;
		logger_->info(
			"plain_publish_ok roomId={} peerId={} transportId={} producerId={} mediaRemoteIp={} announcedIp={} port={} ssrc={} pt={} twccExtId={} audioSsrc={} audioEnabled=false",
			options.room,
			options.peer,
			parsed.transportId,
			parsed.producerId,
			options.mediaRemoteIp,
			parsed.announcedIp,
			parsed.port,
			parsed.ssrc,
			parsed.payloadType,
			parsed.transportCcExtId,
			options.audioSsrc);
		return true;
	} catch (const std::exception& e) {
		logger_->error("publish_signaling_failed error={}", e.what());
		return false;
	}
}

void PushSignalingSession::DispatchNotifications()
{
	ws_.dispatchNotifications();
}

void PushSignalingSession::Close()
{
	ws_.close();
}

} // namespace webrtc_qos_plain
