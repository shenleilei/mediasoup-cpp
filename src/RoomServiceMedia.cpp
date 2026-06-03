#include "RoomService.h"

#include "RoomMediaHelpers.h"
#include "RoomStatsQosHelpers.h"

#include <algorithm>
#include <cctype>

namespace mediasoup {
namespace {

constexpr const char* kPlainClientH264BaselineProfileLevelId = "42e01f";
constexpr const char* kMidExtensionUri =
	"urn:ietf:params:rtp-hdrext:sdes:mid";
constexpr const char* kTransportCcExtensionUri =
	"http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";
constexpr const char* kAbsCaptureTimeExtensionUri =
	"http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time";

struct RemovedPeerEntries {
	std::unordered_map<std::string, std::shared_ptr<Producer>> producers;
	size_t consumerCount = 0;
};

RemovedPeerEntries EraseClosedPeerEntries(const std::shared_ptr<Peer>& peer)
{
	RemovedPeerEntries removed;
	if (!peer) {
		return removed;
	}

	for (auto it = peer->producers.begin(); it != peer->producers.end(); ) {
		if (it->second && it->second->closed()) {
			removed.producers.emplace(it->first, it->second);
			it = peer->producers.erase(it);
		} else {
			++it;
		}
	}

	for (auto it = peer->consumers.begin(); it != peer->consumers.end(); ) {
		if (it->second && it->second->closed()) {
			++removed.consumerCount;
			it = peer->consumers.erase(it);
		} else {
			++it;
		}
	}

	return removed;
}

bool IsPlainClientBaselineH264Codec(const RtpCodecCapability& codec)
{
	if (codec.mimeType != "video/H264") {
		return false;
	}

	const int packetizationMode = codec.parameters.value("packetization-mode", 0);
	const std::string profileLevelId =
		codec.parameters.value("profile-level-id", std::string{});

	return packetizationMode == 1 &&
		profileLevelId == kPlainClientH264BaselineProfileLevelId;
}

bool IsPlainClientVp8Codec(const RtpCodecCapability& codec)
{
	return codec.mimeType == "video/VP8";
}

std::string NormalizeRequestedPlainVideoCodec(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return value;
}

uint8_t FindTransportCcExtensionId(const RtpCapabilities& caps, const std::string& kind)
{
	for (const auto& extension : caps.headerExtensions) {
		if (extension.kind != kind) {
			continue;
		}
		if (extension.uri != kTransportCcExtensionUri) {
			continue;
		}
		if (extension.preferredId == 0) {
			continue;
		}
		return extension.preferredId;
	}
	return 0;
}

uint8_t FindMidExtensionId(const RtpCapabilities& caps, const std::string& kind)
{
	for (const auto& extension : caps.headerExtensions) {
		if (extension.kind != kind) {
			continue;
		}
		if (extension.uri != kMidExtensionUri) {
			continue;
		}
		if (extension.preferredId == 0) {
			continue;
		}
		return extension.preferredId;
	}
	return 0;
}

uint8_t FindAbsCaptureTimeExtensionId(const RtpCapabilities& caps, const std::string& kind)
{
	for (const auto& extension : caps.headerExtensions) {
		if (extension.kind != kind) {
			continue;
		}

		if (extension.uri != kAbsCaptureTimeExtensionUri) {
			continue;
		}

		return extension.preferredId;
	}

	return 0;
}

} // namespace

RoomService::Result RoomService::createTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming, const json& rtpCapabilities)
{
	MS_INFO(logger_, "[{} {}] createTransport start producing={} consuming={} hasRtpCapabilities={}",
		roomId, peerId, producing ? "true" : "false", consuming ? "true" : "false",
		(!rtpCapabilities.is_null() && !rtpCapabilities.empty()) ? "true" : "false");
	if (producing == consuming)
	{
		MS_WARN(logger_, "[{} {}] createTransport failed: exactly one of producing or consuming must be true",
			roomId, peerId);
		return {false, {}, "", "exactly one of producing or consuming must be true"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] createTransport failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] createTransport failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}
	if (consuming) {
		if (!rtpCapabilities.is_null() && !rtpCapabilities.is_object()) {
			MS_WARN(logger_, "[{} {}] createTransport validation failed: invalid rtpCapabilities type",
				roomId, peerId);
			return {false, {}, "", "invalid rtpCapabilities"};
		}
		if (!rtpCapabilities.empty()) {
			peer->rtpCapabilities = rtpCapabilities.get<RtpCapabilities>();
		}
	}

	WebRtcTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.enableUdp = true;
	opts.enableTcp = true;
	opts.preferUdp = true;

	if (producing && peer->sendTransport) {
		peer->sendTransport->close();
		auto removed = EraseClosedPeerEntries(peer);
		for (const auto& [producerId, _] : removed.producers)
			room->router()->removeProducer(producerId);
		cleanupPeerProducerOwnerCache(roomId, removed.producers);
		cleanupPeerProducerDemandCache(roomId, removed.producers);
		peer->sendTransport.reset();
	} else if (!producing && peer->recvTransport) {
		peer->recvTransport->close();
		EraseClosedPeerEntries(peer);
		peer->recvTransport.reset();
	}

	std::shared_ptr<WebRtcTransport> transport;
	try {
		transport = room->router()->createWebRtcTransport(opts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] createTransport failed: createWebRtcTransport threw: {}",
			roomId, peerId, e.what());
		return {false, {}, "", std::string("createTransport failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] createTransport failed: createWebRtcTransport threw unknown error",
			roomId, peerId);
		return {false, {}, "", "createTransport failed: unknown error"};
	}
	transport->setContext(roomId, peerId);
	if (producing) peer->sendTransport = transport;
	else           peer->recvTransport = transport;

	json result = transport->toJson();

	if (!producing && peer->recvTransport) {
		result["consumers"] = roommedia::ConsumeExistingProducers(
			roomId,
			peerId,
			room,
			peer,
			std::static_pointer_cast<Transport>(peer->recvTransport),
			logger_,
			"auto-subscribe on createTransport");
	}

	MS_INFO(logger_, "[{} {}] createTransport done transportId={} producing={} consuming={} precreatedConsumers={}",
		roomId, peerId, transport->id(), producing ? "true" : "false", consuming ? "true" : "false",
		result.contains("consumers") && result["consumers"].is_array() ? result["consumers"].size() : 0);
	return {true, result};
}

RoomService::Result RoomService::connectTransport(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const DtlsParameters& dtlsParams)
{
	MS_INFO(logger_, "[{} {}] connectTransport start transportId={} dtlsParams={}",
		roomId, peerId, transportId, json(dtlsParams).dump());
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] connectTransport failed: room not found transportId={}",
			roomId, peerId, transportId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] connectTransport failed: peer not found transportId={}",
			roomId, peerId, transportId);
		return {false, {}, "", "peer not found"};
	}
	auto transport = peer->getTransport(transportId);
	if (!transport) {
		MS_WARN(logger_, "[{} {}] connectTransport failed: transport not found [{}]",
			roomId, peerId, transportId);
		return {false, {}, "", "transport not found"};
	}
	auto wt = std::dynamic_pointer_cast<WebRtcTransport>(transport);
	if (!wt) {
		MS_WARN(logger_, "[{} {}] connectTransport failed: not a WebRtcTransport [{}]",
			roomId, peerId, transportId);
		return {false, {}, "", "not a WebRtcTransport"};
	}
	auto result = wt->connect(dtlsParams);
	MS_INFO(logger_, "[{} {}] connectTransport done transportId={}", roomId, peerId, transportId);
	return {true, result};
}

RoomService::Result RoomService::createPlainTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming)
{
	MS_INFO(logger_, "[{} {}] createPlainTransport start producing={} consuming={}",
		roomId, peerId, producing ? "true" : "false", consuming ? "true" : "false");
	if (producing == consuming)
	{
		MS_WARN(logger_, "[{} {}] createPlainTransport failed: exactly one of producing or consuming must be true",
			roomId, peerId);
		return {false, {}, "", "exactly one of producing or consuming must be true"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] createPlainTransport failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] createPlainTransport failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}

	PlainTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.rtcpMux = true;
	opts.comedia = false;

	if (producing && peer->plainSendTransport) {
		peer->plainSendTransport->close();
		auto removed = EraseClosedPeerEntries(peer);
		for (const auto& [producerId, _] : removed.producers)
			room->router()->removeProducer(producerId);
		cleanupPeerProducerOwnerCache(roomId, removed.producers);
		cleanupPeerProducerDemandCache(roomId, removed.producers);
		peer->plainSendTransport.reset();
	} else if (!producing && peer->plainRecvTransport) {
		peer->plainRecvTransport->close();
		EraseClosedPeerEntries(peer);
		peer->plainRecvTransport.reset();
	}

	std::shared_ptr<PlainTransport> transport;
	try {
		transport = room->router()->createPlainTransport(opts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] createPlainTransport failed: createPlainTransport threw: {}",
			roomId, peerId, e.what());
		return {false, {}, "", std::string("createPlainTransport failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] createPlainTransport failed: createPlainTransport threw unknown error",
			roomId, peerId);
		return {false, {}, "", "createPlainTransport failed: unknown error"};
	}
	transport->setContext(roomId, peerId);
	if (producing) peer->plainSendTransport = transport;
	else           peer->plainRecvTransport = transport;

	json result = transport->toJson();

	if (!producing && peer->plainRecvTransport) {
		result["consumers"] = roommedia::ConsumeExistingProducers(
			roomId,
			peerId,
			room,
			peer,
			std::static_pointer_cast<Transport>(peer->plainRecvTransport),
			logger_,
			"auto-subscribe on createPlainTransport");
	}

	MS_INFO(logger_, "[{} {}] createPlainTransport done transportId={} producing={} consuming={} precreatedConsumers={}",
		roomId, peerId, transport->id(), producing ? "true" : "false", consuming ? "true" : "false",
		result.contains("consumers") && result["consumers"].is_array() ? result["consumers"].size() : 0);
	return {true, result};
}

RoomService::Result RoomService::connectPlainTransport(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& ip, uint16_t port)
{
	MS_INFO(logger_, "[{} {}] connectPlainTransport start transportId={} ip={} port={}",
		roomId, peerId, transportId, ip, port);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] connectPlainTransport failed: room not found transportId={}",
			roomId, peerId, transportId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] connectPlainTransport failed: peer not found transportId={}",
			roomId, peerId, transportId);
		return {false, {}, "", "peer not found"};
	}
	auto transport = peer->getTransport(transportId);
	if (!transport) {
		MS_WARN(logger_, "[{} {}] connectPlainTransport failed: transport not found [{}]",
			roomId, peerId, transportId);
		return {false, {}, "", "transport not found"};
	}
	auto pt = std::dynamic_pointer_cast<PlainTransport>(transport);
	if (!pt) {
		MS_WARN(logger_, "[{} {}] connectPlainTransport failed: not a PlainTransport [{}]",
			roomId, peerId, transportId);
		return {false, {}, "", "not a PlainTransport"};
	}
	auto result = pt->connect(ip, port);
	MS_INFO(logger_, "[{} {}] connectPlainTransport done transportId={} ip={} port={}",
		roomId, peerId, transportId, ip, port);
	return {true, result};
}

RoomService::Result RoomService::plainPublish(const std::string& roomId,
	const std::string& peerId, const std::vector<uint32_t>& videoSsrcs, uint32_t audioSsrc,
	const std::string& videoCodec,
	bool enableAudio,
	const std::string& senderIp,
	uint16_t senderPort)
{
	MS_INFO(logger_, "[{} {}] plainPublish start videoTracks={} audioEnabled={} senderIp={} senderPort={} videoCodec={}",
		roomId, peerId, videoSsrcs.size(), enableAudio ? "true" : "false",
		senderIp.empty() ? "-" : senderIp, senderPort, videoCodec.empty() ? "-" : videoCodec);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}
	if (videoSsrcs.empty()) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: videoSsrcs cannot be empty", roomId, peerId);
		return {false, {}, "", "videoSsrcs cannot be empty"};
	}
	if (enableAudio && audioSsrc == 0) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: audioSsrc must be non-zero", roomId, peerId);
		return {false, {}, "", "audioSsrc must be non-zero"};
	}

	std::unordered_set<uint32_t> uniqueSsrcs;
	if (enableAudio) uniqueSsrcs.insert(audioSsrc);
	for (auto videoSsrc : videoSsrcs) {
		if (videoSsrc == 0) {
			MS_WARN(logger_, "[{} {}] plainPublish failed: videoSsrc must be non-zero", roomId, peerId);
			return {false, {}, "", "videoSsrcs must be non-zero"};
		}
		if (!uniqueSsrcs.insert(videoSsrc).second) {
			MS_WARN(logger_, "[{} {}] plainPublish failed: duplicate SSRC [{}]", roomId, peerId, videoSsrc);
			return {false, {}, "", "duplicate SSRC in plainPublish request"};
		}
	}

	PlainTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.rtcpMux = true;
	const bool hasExplicitSender = !senderIp.empty() && senderPort != 0;
	opts.comedia = !hasExplicitSender;

	if (peer->plainSendTransport) {
		peer->plainSendTransport->close();
		auto removed = EraseClosedPeerEntries(peer);
		for (const auto& [producerId, _] : removed.producers)
			room->router()->removeProducer(producerId);
		cleanupPeerProducerOwnerCache(roomId, removed.producers);
		cleanupPeerProducerDemandCache(roomId, removed.producers);
		peer->plainSendTransport.reset();
	}

	std::shared_ptr<PlainTransport> transport;
	try {
		transport = room->router()->createPlainTransport(opts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: createPlainTransport threw: {}",
			roomId, peerId, e.what());
		return {false, {}, "", std::string("plainPublish failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: createPlainTransport threw unknown error",
			roomId, peerId);
		return {false, {}, "", "plainPublish failed: unknown error"};
	}
	transport->setContext(roomId, peerId);
	peer->plainSendTransport = transport;
	if (hasExplicitSender) {
		try {
			transport->connect(senderIp, senderPort);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] plainPublish explicit connect failed: {}",
				roomId, peerId, e.what());
			transport->close();
			peer->plainSendTransport.reset();
			return {false, {}, "", std::string("plainPublish connect failed: ") + e.what()};
		} catch (...) {
			MS_WARN(logger_, "[{} {}] plainPublish explicit connect failed: unknown error",
				roomId, peerId);
			transport->close();
			peer->plainSendTransport.reset();
			return {false, {}, "", "plainPublish connect failed: unknown error"};
		}
	}

	auto caps = room->router()->rtpCapabilities();
	const std::string requestedVideoCodec =
		NormalizeRequestedPlainVideoCodec(videoCodec.empty() ? "h264" : videoCodec);
	if (requestedVideoCodec != "h264" && requestedVideoCodec != "vp8") {
		MS_WARN(logger_, "[{} {}] plainPublish failed: unsupported videoCodec [{}]",
			roomId, peerId, requestedVideoCodec);
		return {false, {}, "", "unsupported plain publish videoCodec"};
	}

	std::optional<RtpCodecCapability> selectedVideoCodec;
	std::optional<RtpCodecCapability> audioCodec;
	uint8_t audioPt = 0;
	for (auto& c : caps.codecs) {
		if (!selectedVideoCodec.has_value()) {
			if (requestedVideoCodec == "h264" && IsPlainClientBaselineH264Codec(c))
				selectedVideoCodec = c;
			else if (requestedVideoCodec == "vp8" && IsPlainClientVp8Codec(c))
				selectedVideoCodec = c;
		}
		if (enableAudio && c.mimeType == "audio/opus" && audioPt == 0) {
			audioPt = c.preferredPayloadType;
			audioCodec = c;
		}
	}
	if (!selectedVideoCodec.has_value()) {
		if (requestedVideoCodec == "vp8") {
			MS_WARN(logger_, "[{} {}] plainPublish failed: router has no VP8 codec", roomId, peerId);
			return {false, {}, "", "router has no VP8 codec"};
		}
		MS_WARN(logger_, "[{} {}] plainPublish failed: router has no H264 Baseline codec", roomId, peerId);
		return {false, {}, "", "router has no H264 Baseline codec"};
	}
	if (enableAudio && audioPt == 0) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: router has no opus codec", roomId, peerId);
		return {false, {}, "", "router has no opus codec"};
	}

	const uint8_t videoPt = selectedVideoCodec->preferredPayloadType;
	json videoCodecParameters = selectedVideoCodec->parameters;
	const uint8_t videoMidExtId = FindMidExtensionId(caps, "video");
	const uint8_t videoTransportCcExtId = FindTransportCcExtensionId(caps, "video");
	const uint8_t videoAbsCaptureTimeExtId = FindAbsCaptureTimeExtensionId(caps, "video");
	const uint8_t audioTransportCcExtId = enableAudio ? FindTransportCcExtensionId(caps, "audio") : 0;

	std::vector<std::shared_ptr<Producer>> videoProducers;
	videoProducers.reserve(videoSsrcs.size());
	for (size_t index = 0; index < videoSsrcs.size(); ++index) {
		uint32_t videoSsrc = videoSsrcs[index];
		json videoRtpParams = {
			{"codecs", {{
				{"mimeType", selectedVideoCodec->mimeType}, {"payloadType", videoPt},
				{"clockRate", 90000},
				{"parameters", videoCodecParameters},
				{"rtcpFeedback", selectedVideoCodec->rtcpFeedback}
			}}},
			{"encodings", {{{"ssrc", videoSsrc}}}},
			{"rtcp", {{"cname", peerId + "-video-" + std::to_string(index)}}}
		};
		json videoHeaderExtensions = json::array();
		if (videoMidExtId != 0) {
			videoHeaderExtensions.push_back({
				{"uri", kMidExtensionUri},
				{"id", videoMidExtId},
				{"encrypt", false},
				{"parameters", json::object()}
			});
		}
		if (videoTransportCcExtId != 0) {
			videoHeaderExtensions.push_back({
				{"uri", kTransportCcExtensionUri},
				{"id", videoTransportCcExtId},
				{"encrypt", false},
				{"parameters", json::object()}
			});
		}
		if (videoAbsCaptureTimeExtId != 0) {
			videoHeaderExtensions.push_back({
				{"uri", kAbsCaptureTimeExtensionUri},
				{"id", videoAbsCaptureTimeExtId},
				{"encrypt", false},
				{"parameters", json::object()}
			});
		}
		if (!videoHeaderExtensions.empty()) {
			videoRtpParams["headerExtensions"] = std::move(videoHeaderExtensions);
		}
		json videoProdOpts = {
			{"kind", "video"}, {"rtpParameters", videoRtpParams},
			{"routerRtpCapabilities", caps}
		};
	std::shared_ptr<Producer> videoProd;
	try {
		videoProd = transport->produce(videoProdOpts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: video produce threw at index {} ssrc={} error={}",
			roomId, peerId, index, videoSsrc, e.what());
		return {false, {}, "", std::string("plainPublish video produce failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] plainPublish failed: video produce threw at index {} ssrc={} error=unknown",
			roomId, peerId, index, videoSsrc);
		return {false, {}, "", "plainPublish video produce failed: unknown error"};
	}
	room->router()->addProducer(videoProd);
	roommedia::TrackPeerProducer(roomId, peerId, peer, videoProd, logger_);
	watchProducerScore(roomId, videoProd);
	videoProducers.push_back(videoProd);
}

	std::shared_ptr<Producer> audioProd;
	if (enableAudio) {
		json audioRtpParams = {
			{"codecs", {{
				{"mimeType", "audio/opus"}, {"payloadType", audioPt},
				{"clockRate", 48000}, {"channels", 2},
				{"parameters", {{"useinbandfec", 1}}},
				{"rtcpFeedback", audioCodec.has_value() ? audioCodec->rtcpFeedback : std::vector<RtcpFeedback>{}}
			}}},
			{"encodings", {{{"ssrc", audioSsrc}}}},
			{"rtcp", {{"cname", peerId + "-audio"}}}
		};
		if (audioTransportCcExtId != 0) {
			audioRtpParams["headerExtensions"] = json::array({
				{
					{"uri", kTransportCcExtensionUri},
					{"id", audioTransportCcExtId},
					{"encrypt", false},
					{"parameters", json::object()}
				}
			});
		}
		json audioProdOpts = {
			{"kind", "audio"}, {"rtpParameters", audioRtpParams},
			{"routerRtpCapabilities", caps}
		};
		try {
			audioProd = transport->produce(audioProdOpts);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] plainPublish failed: audio produce threw error={}",
				roomId, peerId, e.what());
			return {false, {}, "", std::string("plainPublish audio produce failed: ") + e.what()};
		} catch (...) {
			MS_WARN(logger_, "[{} {}] plainPublish failed: audio produce threw unknown error", roomId, peerId);
			return {false, {}, "", "plainPublish audio produce failed: unknown error"};
		}
		room->router()->addProducer(audioProd);
		roommedia::TrackPeerProducer(roomId, peerId, peer, audioProd, logger_);
		watchProducerScore(roomId, audioProd);
	}

	indexPeerProducers(roomId, peerId, peer->producers);

	std::vector<std::shared_ptr<Producer>> allProducers = videoProducers;
	if (audioProd) allProducers.push_back(audioProd);
	for (const auto& prod : allProducers) {
		roommedia::AutoSubscribeProducerToOtherPeers(
			roomId, peerId, room, prod, logger_, notify_, true);
	}

	auto tuple = transport->tuple();
	json videoTracks = json::array();
	for (size_t index = 0; index < videoProducers.size(); ++index) {
		videoTracks.push_back({
			{"index", index},
			{"pt", videoPt},
			{"ssrc", videoSsrcs[index]},
			{"producerId", videoProducers[index]->id()},
			{"transportCcExtId", videoTransportCcExtId},
			{"absCaptureTimeExtId", videoAbsCaptureTimeExtId}
		});
	}
	json result = {
		{"transportId", transport->id()},
		{"ip", tuple.localAddress}, {"port", tuple.localPort},
		{"videoPt", videoPt}, {"videoSsrc", videoSsrcs.front()},
		{"videoProdId", videoProducers.front()->id()},
		{"videoCodec", requestedVideoCodec},
		{"videoTracks", videoTracks},
		{"videoTransportCcExtId", videoTransportCcExtId},
		{"videoAbsCaptureTimeExtId", videoAbsCaptureTimeExtId},
		{"audioEnabled", enableAudio}
	};
	if (enableAudio && audioProd) {
		result["audioPt"] = audioPt;
		result["audioSsrc"] = audioSsrc;
		result["audioProdId"] = audioProd->id();
		result["audioTransportCcExtId"] = audioTransportCcExtId;
	}
	MS_INFO(logger_, "[{} {}] plainPublish done transportId={} videoTracks={} audioEnabled={}",
		roomId, peerId, transport->id(), videoProducers.size(), enableAudio ? "true" : "false");
	return {true, result};
}

RoomService::Result RoomService::plainSubscribe(const std::string& roomId,
	const std::string& peerId,
	const std::optional<std::string>& recvIp,
	const std::optional<uint16_t>& recvPort,
	bool autoReturn)
{
	MS_INFO(logger_, "[{} {}] plainSubscribe start autoReturn={} recvIp={} recvPort={}",
		roomId, peerId, autoReturn ? "true" : "false",
		recvIp.has_value() ? *recvIp : "-", recvPort.has_value() ? std::to_string(*recvPort) : "-");
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}

	if (autoReturn) {
		if (recvIp.has_value() || recvPort.has_value()) {
			MS_WARN(logger_, "[{} {}] plainSubscribe failed: autoReturn cannot be combined with recvIp/recvPort",
				roomId, peerId);
			return {false, {}, "", "plainSubscribe autoReturn cannot be combined with recvIp/recvPort"};
		}
	} else {
		if (!recvIp.has_value() || !recvPort.has_value() || recvIp->empty() || *recvPort == 0) {
			MS_WARN(logger_, "[{} {}] plainSubscribe failed: recvIp/recvPort required",
				roomId, peerId);
			return {false, {}, "", "plainSubscribe requires recvIp and recvPort"};
		}
	}

	PlainTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.rtcpMux = true;
	opts.comedia = autoReturn;

	if (peer->plainRecvTransport) {
		peer->plainRecvTransport->close();
		EraseClosedPeerEntries(peer);
		peer->plainRecvTransport.reset();
	}

	std::shared_ptr<PlainTransport> transport;
	try {
		transport = room->router()->createPlainTransport(opts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: createPlainTransport threw: {}",
			roomId, peerId, e.what());
		return {false, {}, "", std::string("plainSubscribe failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: createPlainTransport threw unknown error", roomId, peerId);
		return {false, {}, "", "plainSubscribe failed: unknown error"};
	}
	transport->setContext(roomId, peerId);
	peer->plainRecvTransport = transport;
	if (!autoReturn) {
		try {
			transport->connect(*recvIp, *recvPort);
		} catch (const std::exception& e) {
			MS_WARN(logger_, "[{} {}] plainSubscribe connect failed: {}",
				roomId, peerId, e.what());
			transport->close();
			peer->plainRecvTransport.reset();
			return {false, {}, "", std::string("plainSubscribe connect failed: ") + e.what()};
		} catch (...) {
			MS_WARN(logger_, "[{} {}] plainSubscribe connect failed: unknown error",
				roomId, peerId);
			transport->close();
			peer->plainRecvTransport.reset();
			return {false, {}, "", "plainSubscribe connect failed: unknown error"};
		}
	}

	json consumers;
	try {
		consumers = roommedia::ConsumeExistingProducers(
			roomId,
			peerId,
			room,
			peer,
			std::static_pointer_cast<Transport>(transport),
			logger_,
			"plainSubscribe",
			false);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: consume existing producers threw: {}",
			roomId, peerId, e.what());
		return {false, {}, "", std::string("plainSubscribe consume failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] plainSubscribe failed: consume existing producers threw unknown error",
			roomId, peerId);
		return {false, {}, "", "plainSubscribe consume failed: unknown error"};
	}

	auto tuple = transport->tuple();
	MS_INFO(logger_, "[{} {}] plainSubscribe done transportId={} consumers={} autoReturn={}",
		roomId, peerId, transport->id(), consumers.is_array() ? consumers.size() : 0, autoReturn ? "true" : "false");
	return {true, {
		{"transportId", transport->id()},
		{"ip", tuple.localAddress}, {"port", tuple.localPort},
		{"autoReturn", autoReturn},
		{"consumers", consumers}
	}};
}

RoomService::Result RoomService::produce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& kind, const json& rtpParameters, const json& appData)
{
	const std::string source = appData.value("source", "");
	MS_INFO(logger_, "[{} {}] produce start transportId={} kind={} source={} appData={}",
		roomId, peerId, transportId, kind, source.empty() ? "-" : source, appData.dump());

	if (kind != "audio" && kind != "video") {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid kind '{}'", roomId, peerId, kind);
		return {false, {}, "", "invalid kind"};
	}
	if (!rtpParameters.is_object()) {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid rtpParameters type", roomId, peerId);
		return {false, {}, "", "invalid rtpParameters"};
	}
	if (!appData.is_object()) {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid appData type", roomId, peerId);
		return {false, {}, "", "invalid appData"};
	}
	if (appData.contains("source") && !appData.at("source").is_string()) {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid appData.source type", roomId, peerId);
		return {false, {}, "", "invalid appData.source"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] produce failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] produce failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}
	auto transport = peer->getTransport(transportId);
	if (!transport) {
		MS_WARN(logger_, "[{} {}] produce failed: transport not found [{}]", roomId, peerId, transportId);
		return {false, {}, "", "transport not found"};
	}

	json produceOpts = {
		{"kind", kind}, {"rtpParameters", rtpParameters},
		{"routerRtpCapabilities", room->router()->rtpCapabilities()},
		{"appData", appData}
	};
	std::shared_ptr<Producer> producer;
	try {
		producer = transport->produce(produceOpts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] produce failed: transport->produce threw: {}", roomId, peerId, e.what());
		return {false, {}, "", std::string("produce failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] produce failed: transport->produce threw unknown error", roomId, peerId);
		return {false, {}, "", "produce failed: unknown error"};
	}
	room->router()->addProducer(producer);
	roommedia::TrackPeerProducer(roomId, peerId, peer, producer, logger_);
	watchProducerScore(roomId, producer);
	indexPeerProducers(roomId, peerId, peer->producers);

	roommedia::AutoSubscribeProducerToOtherPeers(
		roomId, peerId, room, producer, logger_, notify_, false);

	MS_INFO(logger_, "[{} {}] produce done transportId={} kind={} source={} producerId={}",
		roomId, peerId, transportId, kind, source.empty() ? "-" : source, producer->id());
	return {true, {{"id", producer->id()}}};
}

RoomService::Result RoomService::consume(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& producerId, const json& rtpCapabilities)
{
	MS_INFO(logger_, "[{} {}] consume start transportId={} producerId={}",
		roomId, peerId, transportId, producerId);

	if (!rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] consume validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] consume failed: room not found", roomId, peerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] consume failed: peer not found", roomId, peerId);
		return {false, {}, "", "peer not found"};
	}
	auto transport = peer->getTransport(transportId);
	if (!transport) {
		MS_WARN(logger_, "[{} {}] consume failed: transport not found [{}]", roomId, peerId, transportId);
		return {false, {}, "", "transport not found"};
	}
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) {
		MS_WARN(logger_, "[{} {}] consume failed: producer not found [{}]", roomId, peerId, producerId);
		return {false, {}, "", "producer not found"};
	}

	json consumeOpts = {
		{"producerId", producerId},
		{"rtpCapabilities", rtpCapabilities},
		{"consumableRtpParameters", producer->consumableRtpParameters()}
	};
	std::shared_ptr<Consumer> consumer;
	try {
		consumer = transport->consume(consumeOpts);
	} catch (const std::exception& e) {
		MS_WARN(logger_, "[{} {}] consume failed: transport->consume threw: {}", roomId, peerId, e.what());
		return {false, {}, "", std::string("consume failed: ") + e.what()};
	} catch (...) {
		MS_WARN(logger_, "[{} {}] consume failed: transport->consume threw unknown error", roomId, peerId);
		return {false, {}, "", "consume failed: unknown error"};
	}
	roommedia::TrackPeerConsumer(roomId, peerId, peer, consumer, logger_);
	MS_INFO(logger_, "[{} {}] consume done transportId={} producerId={} consumerId={} source={}",
		roomId, peerId, transportId, producerId, consumer->id(), producer->source().empty() ? "-" : producer->source());
	return {true, roommedia::BuildConsumerData(producer->peerId(), producer, consumer)};
}

RoomService::Result RoomService::pauseProducer(const std::string& roomId,
	const std::string& producerId)
{
	MS_INFO(logger_, "[{} system] pauseProducer start producerId={}", roomId, producerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} system] pauseProducer failed: room not found producerId={}", roomId, producerId);
		return {false, {}, "", "room not found"};
	}
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) {
		MS_WARN(logger_, "[{} system] pauseProducer failed: producer not found [{}]", roomId, producerId);
		return {false, {}, "", "producer not found"};
	}
	producer->pause();
	MS_INFO(logger_, "[{} system] pauseProducer done producerId={}", roomId, producerId);
	return {true, {}};
}

RoomService::Result RoomService::resumeProducer(const std::string& roomId,
	const std::string& producerId)
{
	MS_INFO(logger_, "[{} system] resumeProducer start producerId={}", roomId, producerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} system] resumeProducer failed: room not found producerId={}", roomId, producerId);
		return {false, {}, "", "room not found"};
	}
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) {
		MS_WARN(logger_, "[{} system] resumeProducer failed: producer not found [{}]", roomId, producerId);
		return {false, {}, "", "producer not found"};
	}
	producer->resume();
	MS_INFO(logger_, "[{} system] resumeProducer done producerId={}", roomId, producerId);
	return {true, {}};
}

RoomService::Result RoomService::restartIce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId)
{
	MS_INFO(logger_, "[{} {}] restartIce start transportId={}", roomId, peerId, transportId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] restartIce failed: room not found transportId={}", roomId, peerId, transportId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] restartIce failed: peer not found transportId={}", roomId, peerId, transportId);
		return {false, {}, "", "peer not found"};
	}
	auto wt = std::dynamic_pointer_cast<WebRtcTransport>(peer->getTransport(transportId));
	if (!wt) {
		MS_WARN(logger_, "[{} {}] restartIce failed: transport not found [{}]", roomId, peerId, transportId);
		return {false, {}, "", "transport not found"};
	}
	auto result = wt->restartIce();
	MS_INFO(logger_, "[{} {}] restartIce done transportId={}", roomId, peerId, transportId);
	return {true, result};
}

RoomService::Result RoomService::setQosOverride(
	const std::string& roomId, const std::string& callerPeerId,
	const std::string& targetPeerId, const json& overrideData)
{
	MS_INFO(logger_, "[{} {}] setQosOverride start targetPeerId={}", roomId, callerPeerId, targetPeerId);
	if (callerPeerId != targetPeerId) {
		MS_WARN(logger_, "[{} {}] setQosOverride failed: permission denied targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "permission denied: can only set QoS override for self"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] setQosOverride failed: room not found targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(targetPeerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] setQosOverride failed: peer not found targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "peer not found"};
	}

	auto parsed = qos::QosValidator::ParseOverride(overrideData);
	if (!parsed.ok) {
		MS_WARN(logger_, "[{} {}] setQosOverride failed: invalid qosOverride targetPeerId={} error={}",
			roomId, callerPeerId, targetPeerId, parsed.error);
		return {false, {}, "", "invalid qosOverride: " + parsed.error};
	}

	if (notify_) {
		MS_INFO(logger_, "[{} {}] notify qosOverride target={} reason={}",
			roomId, callerPeerId, targetPeerId, parsed.value.reason);
		notify_(roomId, targetPeerId, {
			{"notification", true},
			{"method", "qosOverride"},
			{"data", qos::ToJson(parsed.value)}
		});
	}

	MS_INFO(logger_, "[{} {}] setQosOverride done targetPeerId={}", roomId, callerPeerId, targetPeerId);
	return {true, json::object()};
}

RoomService::Result RoomService::setQosPolicy(
	const std::string& roomId, const std::string& callerPeerId,
	const std::string& targetPeerId, const json& policyData)
{
	MS_INFO(logger_, "[{} {}] setQosPolicy start targetPeerId={}", roomId, callerPeerId, targetPeerId);
	if (callerPeerId != targetPeerId) {
		MS_WARN(logger_, "[{} {}] setQosPolicy failed: permission denied targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "permission denied: can only set QoS policy for self"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] setQosPolicy failed: room not found targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(targetPeerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] setQosPolicy failed: peer not found targetPeerId={}", roomId, callerPeerId, targetPeerId);
		return {false, {}, "", "peer not found"};
	}

	auto parsed = qos::QosValidator::ParsePolicy(policyData);
	if (!parsed.ok) {
		MS_WARN(logger_, "[{} {}] setQosPolicy failed: invalid qosPolicy targetPeerId={} error={}",
			roomId, callerPeerId, targetPeerId, parsed.error);
		return {false, {}, "", "invalid qosPolicy: " + parsed.error};
	}

	if (notify_) {
		MS_INFO(logger_, "[{} {}] notify qosPolicy target={} schema={}",
			roomId, callerPeerId, targetPeerId, parsed.value.schema);
		notify_(roomId, targetPeerId, {
			{"notification", true},
			{"method", "qosPolicy"},
			{"data", qos::ToJson(parsed.value)}
		});
	}

	MS_INFO(logger_, "[{} {}] setQosPolicy done targetPeerId={}", roomId, callerPeerId, targetPeerId);
	return {true, json::object()};
}

RoomService::Result RoomService::pauseConsumer(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	MS_INFO(logger_, "[{} {}] pauseConsumer start consumerId={}", roomId, peerId, consumerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] pauseConsumer failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] pauseConsumer failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] pauseConsumer failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	it->second->pause();
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	MS_INFO(logger_, "[{} {}] pauseConsumer done consumerId={}", roomId, peerId, consumerId);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::resumeConsumer(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	MS_INFO(logger_, "[{} {}] resumeConsumer start consumerId={}", roomId, peerId, consumerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] resumeConsumer failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] resumeConsumer failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] resumeConsumer failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	it->second->resume();
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	MS_INFO(logger_, "[{} {}] resumeConsumer done consumerId={}", roomId, peerId, consumerId);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::getConsumerState(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	MS_INFO(logger_, "[{} {}] getConsumerState start consumerId={}", roomId, peerId, consumerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] getConsumerState failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] getConsumerState failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] getConsumerState failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	MS_INFO(logger_, "[{} {}] getConsumerState done consumerId={}", roomId, peerId, consumerId);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::setConsumerPreferredLayers(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId,
	uint8_t spatialLayer, uint8_t temporalLayer)
{
	MS_INFO(logger_, "[{} {}] setConsumerPreferredLayers start consumerId={} spatial={} temporal={}",
		roomId, peerId, consumerId, spatialLayer, temporalLayer);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] setConsumerPreferredLayers failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] setConsumerPreferredLayers failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] setConsumerPreferredLayers failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	it->second->setPreferredLayers(spatialLayer, temporalLayer);
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	MS_INFO(logger_, "[{} {}] setConsumerPreferredLayers done consumerId={} spatial={} temporal={}",
		roomId, peerId, consumerId, spatialLayer, temporalLayer);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::setConsumerPriority(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId, uint8_t priority)
{
	MS_INFO(logger_, "[{} {}] setConsumerPriority start consumerId={} priority={}",
		roomId, peerId, consumerId, priority);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] setConsumerPriority failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] setConsumerPriority failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] setConsumerPriority failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	it->second->setPriority(priority);
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	MS_INFO(logger_, "[{} {}] setConsumerPriority done consumerId={} priority={}",
		roomId, peerId, consumerId, priority);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::requestConsumerKeyFrame(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	MS_INFO(logger_, "[{} {}] requestConsumerKeyFrame start consumerId={}", roomId, peerId, consumerId);
	auto room = roomManager_.getRoom(roomId);
	if (!room) {
		MS_WARN(logger_, "[{} {}] requestConsumerKeyFrame failed: room not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "room not found"};
	}
	auto peer = room->getPeer(peerId);
	if (!peer) {
		MS_WARN(logger_, "[{} {}] requestConsumerKeyFrame failed: peer not found consumerId={}", roomId, peerId, consumerId);
		return {false, {}, "", "peer not found"};
	}
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed()) {
		MS_WARN(logger_, "[{} {}] requestConsumerKeyFrame failed: consumer not found [{}]", roomId, peerId, consumerId);
		return {false, {}, "", "consumer not found"};
	}
	it->second->requestKeyFrame();
	MS_INFO(logger_, "[{} {}] requestConsumerKeyFrame done consumerId={}", roomId, peerId, consumerId);
	return {true, it->second->toJson()};
}

} // namespace mediasoup
