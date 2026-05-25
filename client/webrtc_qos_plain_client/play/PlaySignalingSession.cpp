#include "play/PlaySignalingSession.h"

#include <algorithm>
#include <stdexcept>

namespace webrtc_qos_plain {
namespace {

constexpr const char* kTransportCcUri =
	"http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";

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

std::string Lower(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return value;
}

} // namespace

json BuildMinimalPlainReceiveCapabilities()
{
	return {
		{"codecs", json::array({
			{
				{"kind", "audio"},
				{"mimeType", "audio/opus"},
				{"preferredPayloadType", 100},
				{"clockRate", 48000},
				{"channels", 2},
				{"parameters", json::object()},
				{"rtcpFeedback", json::array()}
			},
			{
				{"kind", "video"},
				{"mimeType", "video/H264"},
				{"preferredPayloadType", 127},
				{"clockRate", 90000},
				{"parameters", {
					{"level-asymmetry-allowed", 1},
					{"packetization-mode", 1},
					{"profile-level-id", "42e01f"}
				}},
				{"rtcpFeedback", json::array({
					{{"type", "nack"}, {"parameter", ""}},
					{{"type", "nack"}, {"parameter", "pli"}},
					{{"type", "transport-cc"}, {"parameter", ""}}
				})}
			}
		})},
		{"headerExtensions", json::array({
			{
				{"kind", "video"},
				{"uri", kTransportCcUri},
				{"preferredId", 5},
				{"preferredEncrypt", false},
				{"direction", "recvonly"}
			}
		})}
	};
}

PlaySignalingSession::PlaySignalingSession(std::shared_ptr<spdlog::logger> logger)
	: logger_(std::move(logger))
{
}

bool PlaySignalingSession::ConnectJoinAndSubscribe(const PlayOptions& options)
{
	if (!ws_.connect(options.serverIp, options.serverPort, "/ws")) {
		logger_->error("ws_connect_failed serverIp={} serverPort={}", options.serverIp, options.serverPort);
		return false;
	}
	logger_->info("ws_connected serverIp={} serverPort={}", options.serverIp, options.serverPort);

	ws_.setNotificationHandler([this](const json& msg) {
		const std::string method = msg.value("method", "");
		if (method == "newConsumer") {
			pendingConsumers_.push_back(msg.value("data", json::object()));
			logger_->info("new_consumer_notification data={}", msg.value("data", json::object()).dump());
			return;
		}
		logger_->info("ws_notification method={} data={}", method, msg.value("data", json::object()).dump());
	});

	try {
		ws_.request("join", {
			{"roomId", options.room},
			{"peerId", options.peer},
			{"displayName", options.peer},
			{"rtpCapabilities", BuildMinimalPlainReceiveCapabilities()}
		});
		logger_->info("join_ok roomId={} peerId={} receiveCaps=h264-baseline+opus-compat", options.room, options.peer);

		const auto response = ws_.request("plainSubscribe", {
			{"recvIp", options.advertiseIp},
			{"recvPort", options.listenPort}
		});
		transportId_ = response.at("transportId").get<std::string>();
		announcedIp_ = response.value("ip", "");
		plainTransportPort_ = JsonU16(response, "port");
		const auto consumers = response.value("consumers", json::array());
		if (consumers.is_array()) {
			for (const auto& consumer : consumers) pendingConsumers_.push_back(consumer);
		}
		logger_->info(
			"plain_subscribe_ok roomId={} peerId={} transportId={} announcedIp={} port={} recvIp={} recvPort={} returnedConsumers={}",
			options.room,
			options.peer,
			transportId_,
			announcedIp_,
			plainTransportPort_,
			options.advertiseIp,
			options.listenPort,
			consumers.is_array() ? consumers.size() : 0);
		return true;
	} catch (const std::exception& e) {
		logger_->error("play_signaling_failed error={}", e.what());
		return false;
	}
}

std::optional<ConsumerInfo> PlaySignalingSession::TakeSelectedConsumer(const PlayOptions& options)
{
	auto selected = TakeSelectedConsumers(options, 1);
	if (selected.empty()) return std::nullopt;
	return selected.front();
}

std::vector<ConsumerInfo> PlaySignalingSession::TakeSelectedConsumers(
	const PlayOptions& options,
	size_t maxConsumers)
{
	std::vector<ConsumerInfo> selected;
	if (maxConsumers == 0) return selected;
	for (auto it = pendingConsumers_.begin(); it != pendingConsumers_.end() && selected.size() < maxConsumers;) {
		auto parsed = TryParseConsumer(
			*it,
			options,
			transportId_,
			plainTransportPort_,
			announcedIp_);
		if (parsed) {
			pendingConsumers_.erase(it);
			selected.push_back(std::move(*parsed));
			it = pendingConsumers_.begin();
			continue;
		}
		++it;
	}
	return selected;
}

void PlaySignalingSession::DispatchNotifications()
{
	ws_.dispatchNotifications();
}

bool PlaySignalingSession::RequestConsumerKeyFrame(const std::string& consumerId)
{
	try {
		ws_.request("requestConsumerKeyFrame", {{"consumerId", consumerId}}, 5000);
		logger_->info("request_consumer_keyframe_ok consumerId={}", consumerId);
		return true;
	} catch (const std::exception& e) {
		logger_->warn("request_consumer_keyframe_failed consumerId={} error={}", consumerId, e.what());
		return false;
	}
}

void PlaySignalingSession::Close()
{
	ws_.close();
}

std::optional<ConsumerInfo> PlaySignalingSession::TryParseConsumer(
	const json& consumer,
	const PlayOptions& options,
	const std::string& transportId,
	uint16_t plainTransportPort,
	const std::string& announcedIp)
{
	try {
		if (consumer.value("kind", "") != "video") return std::nullopt;
		const std::string producerId = consumer.value("producerId", "");
		const std::string producerPeerId = consumer.value("peerId", "");
		if (!options.producerId.empty() && producerId != options.producerId) return std::nullopt;
		if (!options.producerPeerId.empty() && producerPeerId != options.producerPeerId) return std::nullopt;

		const auto& params = consumer.at("rtpParameters");
		uint8_t payloadType = 0;
		for (const auto& codec : params.value("codecs", json::array())) {
			if (Lower(codec.value("mimeType", "")) != "video/h264") continue;
			const auto codecParams = codec.value("parameters", json::object());
			if (codecParams.value("packetization-mode", 0) != 1) continue;
			if (Lower(codecParams.value("profile-level-id", std::string{})) != "42e01f") continue;
			payloadType = JsonU8(codec, "payloadType");
			break;
		}
		if (payloadType == 0) return std::nullopt;

		uint32_t ssrc = 0;
		const auto encodings = params.value("encodings", json::array());
		if (encodings.is_array() && !encodings.empty())
			ssrc = encodings.at(0).value("ssrc", 0u);
		if (ssrc == 0) return std::nullopt;

		uint8_t twccExtId = 0;
		for (const auto& ext : params.value("headerExtensions", json::array())) {
			if (ext.value("uri", "") == kTransportCcUri) {
				twccExtId = JsonU8(ext, "id");
				break;
			}
		}
		if (twccExtId == 0) {
			logger_->warn(
				"consumer_without_twcc_ext peerId={} producerId={} consumerId={} ssrc={} pt={} headerExtensions={}",
				producerPeerId,
				producerId,
				consumer.value("id", ""),
				ssrc,
				payloadType,
				params.value("headerExtensions", json::array()).dump());
		}

		ConsumerInfo info;
		info.peerId = producerPeerId;
		info.producerId = producerId;
		info.consumerId = consumer.at("id").get<std::string>();
		info.transportId = transportId;
		info.plainTransportPort = plainTransportPort;
		info.announcedIp = announcedIp;
		info.payloadType = payloadType;
		info.ssrc = ssrc;
		info.transportCcExtId = twccExtId;
		info.raw = consumer;
		logger_->info(
			"selected_consumer peerId={} producerId={} consumerId={} transportId={} ssrc={} pt={} twccExtId={}",
			info.peerId,
			info.producerId,
			info.consumerId,
			info.transportId,
			info.ssrc,
			info.payloadType,
			info.transportCcExtId);
		return info;
	} catch (const std::exception& e) {
		logger_->warn("consumer_parse_failed error={} data={}", e.what(), consumer.dump());
		return std::nullopt;
	}
}

} // namespace webrtc_qos_plain
