#include "push/PushSignalingSession.h"

#include <stdexcept>
#include <unordered_map>

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

		json publishRequest = {
			{"videoCodec", "h264"},
			{"videoSsrc", options.videoSsrc},
			{"videoSsrcs", json::array()},
			{"enableAudio", options.enableAudio}
		};
		for (const auto& track : options.tracks) {
			publishRequest["videoSsrcs"].push_back(track.videoSsrc);
		}
		if (options.enableAudio) publishRequest["audioSsrc"] = options.audioSsrc;

		const auto response = ws_.request("plainPublish", publishRequest);

		if (response.value("videoCodec", "") != "h264")
			throw std::runtime_error("plainPublish returned non-H264 codec");
		auto tracks = response.value("videoTracks", json::array());
		if (!tracks.is_array() || tracks.empty())
			throw std::runtime_error("plainPublish must return non-empty videoTracks[]");

		std::unordered_map<uint32_t, PushTrackOptions> requestedTracks;
		for (const auto& track : options.tracks) requestedTracks.emplace(track.videoSsrc, track);

		PublishInfo parsed;
		parsed.transportId = response.at("transportId").get<std::string>();
		parsed.announcedIp = response.value("ip", "");
		parsed.port = JsonU16(response, "port");
		parsed.videoTracks.reserve(tracks.size());
		for (size_t index = 0; index < tracks.size(); ++index) {
			const auto& track = tracks.at(index);
			PublishedVideoTrackInfo parsedTrack;
			parsedTrack.ssrc = track.at("ssrc").get<uint32_t>();
			parsedTrack.payloadType = JsonU8(track, "pt");
			parsedTrack.producerId = track.at("producerId").get<std::string>();
			parsedTrack.transportCcExtId = JsonU8(track, "transportCcExtId");
			parsedTrack.trackId = "track" + std::to_string(index);
			if (auto it = requestedTracks.find(parsedTrack.ssrc); it != requestedTracks.end()) {
				parsedTrack.trackId = it->second.id;
				parsedTrack.weight = it->second.weight;
			}
			if (parsedTrack.ssrc == 0 || parsedTrack.payloadType == 0 || parsedTrack.transportCcExtId == 0)
				throw std::runtime_error("plainPublish returned invalid SSRC/PT/TWCC ext id");
			parsed.videoTracks.push_back(parsedTrack);
		}
		const auto& firstTrack = parsed.videoTracks.front();
		parsed.payloadType = firstTrack.payloadType;
		parsed.ssrc = firstTrack.ssrc;
		parsed.producerId = firstTrack.producerId;
		parsed.transportCcExtId = firstTrack.transportCcExtId;
		if (parsed.ssrc == 0 || parsed.payloadType == 0 || parsed.transportCcExtId == 0)
			throw std::runtime_error("plainPublish returned invalid SSRC/PT/TWCC ext id");

		*info = parsed;
		const bool audioEnabled = response.value("audioEnabled", options.enableAudio);
		const uint32_t audioSsrc = response.value("audioSsrc", options.audioSsrc);
		logger_->info(
			"plain_publish_ok roomId={} peerId={} transportId={} producerId={} mediaRemoteIp={} announcedIp={} port={} ssrc={} pt={} twccExtId={} audioSsrc={} audioEnabled={}",
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
			audioSsrc,
			audioEnabled);
		logger_->info("plain_publish_tracks count={} tracks={}", parsed.videoTracks.size(), tracks.dump());
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
