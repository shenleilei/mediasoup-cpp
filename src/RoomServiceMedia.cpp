#include "RoomService.h"

#include "RoomMediaHelpers.h"
#include "RoomStatsQosHelpers.h"

#include <algorithm>
#include <cctype>

namespace mediasoup {
namespace {

constexpr const char* kPlainClientH264BaselineProfileLevelId = "42e01f";
constexpr const char* kTransportCcExtensionUri =
	"http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";

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

} // namespace

RoomService::Result RoomService::createTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming, const json& rtpCapabilities)
{
	if (producing == consuming)
		return {false, {}, "", "exactly one of producing or consuming must be true"};

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
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

	auto transport = room->router()->createWebRtcTransport(opts);
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

	return {true, result};
}

RoomService::Result RoomService::connectTransport(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const DtlsParameters& dtlsParams)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto transport = peer->getTransport(transportId);
	if (!transport) return {false, {}, "", "transport not found"};
	auto wt = std::dynamic_pointer_cast<WebRtcTransport>(transport);
	if (!wt) return {false, {}, "", "not a WebRtcTransport"};
	return {true, wt->connect(dtlsParams)};
}

RoomService::Result RoomService::createPlainTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming)
{
	if (producing == consuming)
		return {false, {}, "", "exactly one of producing or consuming must be true"};

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};

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

	auto transport = room->router()->createPlainTransport(opts);
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

	return {true, result};
}

RoomService::Result RoomService::connectPlainTransport(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& ip, uint16_t port)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto transport = peer->getTransport(transportId);
	if (!transport) return {false, {}, "", "transport not found"};
	auto pt = std::dynamic_pointer_cast<PlainTransport>(transport);
	if (!pt) return {false, {}, "", "not a PlainTransport"};
	return {true, pt->connect(ip, port)};
}

RoomService::Result RoomService::plainPublish(const std::string& roomId,
	const std::string& peerId, const std::vector<uint32_t>& videoSsrcs, uint32_t audioSsrc,
	const std::string& videoCodec,
	bool enableAudio)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	if (videoSsrcs.empty()) return {false, {}, "", "videoSsrcs cannot be empty"};
	if (enableAudio && audioSsrc == 0) return {false, {}, "", "audioSsrc must be non-zero"};

	std::unordered_set<uint32_t> uniqueSsrcs;
	if (enableAudio) uniqueSsrcs.insert(audioSsrc);
	for (auto videoSsrc : videoSsrcs) {
		if (videoSsrc == 0) return {false, {}, "", "videoSsrcs must be non-zero"};
		if (!uniqueSsrcs.insert(videoSsrc).second)
			return {false, {}, "", "duplicate SSRC in plainPublish request"};
	}

	PlainTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.rtcpMux = true;
	opts.comedia = true;

	if (peer->plainSendTransport) {
		peer->plainSendTransport->close();
		auto removed = EraseClosedPeerEntries(peer);
		for (const auto& [producerId, _] : removed.producers)
			room->router()->removeProducer(producerId);
		cleanupPeerProducerOwnerCache(roomId, removed.producers);
		cleanupPeerProducerDemandCache(roomId, removed.producers);
		peer->plainSendTransport.reset();
	}

	auto transport = room->router()->createPlainTransport(opts);
	peer->plainSendTransport = transport;

	auto caps = room->router()->rtpCapabilities();
	const std::string requestedVideoCodec =
		NormalizeRequestedPlainVideoCodec(videoCodec.empty() ? "h264" : videoCodec);
	if (requestedVideoCodec != "h264" && requestedVideoCodec != "vp8")
		return {false, {}, "", "unsupported plain publish videoCodec"};

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
		if (requestedVideoCodec == "vp8")
			return {false, {}, "", "router has no VP8 codec"};
		return {false, {}, "", "router has no H264 Baseline codec"};
	}
	if (enableAudio && audioPt == 0) return {false, {}, "", "router has no opus codec"};

	const uint8_t videoPt = selectedVideoCodec->preferredPayloadType;
	json videoCodecParameters = selectedVideoCodec->parameters;
	const uint8_t videoTransportCcExtId = FindTransportCcExtensionId(caps, "video");
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
		if (videoTransportCcExtId != 0) {
			videoRtpParams["headerExtensions"] = json::array({
				{
					{"uri", kTransportCcExtensionUri},
					{"id", videoTransportCcExtId},
					{"encrypt", false},
					{"parameters", json::object()}
				}
			});
		}
		json videoProdOpts = {
			{"kind", "video"}, {"rtpParameters", videoRtpParams},
			{"routerRtpCapabilities", caps}
		};
	auto videoProd = transport->produce(videoProdOpts);
	room->router()->addProducer(videoProd);
	roommedia::TrackPeerProducer(peer, videoProd);
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
		audioProd = transport->produce(audioProdOpts);
		room->router()->addProducer(audioProd);
		roommedia::TrackPeerProducer(peer, audioProd);
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
			{"transportCcExtId", videoTransportCcExtId}
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
		{"audioEnabled", enableAudio}
	};
	if (enableAudio && audioProd) {
		result["audioPt"] = audioPt;
		result["audioSsrc"] = audioSsrc;
		result["audioProdId"] = audioProd->id();
		result["audioTransportCcExtId"] = audioTransportCcExtId;
	}
	return {true, result};
}

RoomService::Result RoomService::plainSubscribe(const std::string& roomId,
	const std::string& peerId, const std::string& recvIp, uint16_t recvPort)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};

	PlainTransportOptions opts;
	opts.listenInfos = roomManager_.listenInfos();
	opts.rtcpMux = true;
	opts.comedia = false;

	if (peer->plainRecvTransport) {
		peer->plainRecvTransport->close();
		EraseClosedPeerEntries(peer);
		peer->plainRecvTransport.reset();
	}

	auto transport = room->router()->createPlainTransport(opts);
	peer->plainRecvTransport = transport;
	transport->connect(recvIp, recvPort);

	json consumers = roommedia::ConsumeExistingProducers(
		roomId,
		peerId,
		room,
		peer,
		std::static_pointer_cast<Transport>(transport),
		logger_,
		"plainSubscribe",
		false);

	auto tuple = transport->tuple();
	return {true, {
		{"transportId", transport->id()},
		{"ip", tuple.localAddress}, {"port", tuple.localPort},
		{"consumers", consumers}
	}};
}

RoomService::Result RoomService::produce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& kind, const json& rtpParameters, const json& appData)
{
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
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto transport = peer->getTransport(transportId);
	if (!transport) return {false, {}, "", "transport not found"};

	json produceOpts = {
		{"kind", kind}, {"rtpParameters", rtpParameters},
		{"routerRtpCapabilities", room->router()->rtpCapabilities()},
		{"appData", appData}
	};
	auto producer = transport->produce(produceOpts);
	room->router()->addProducer(producer);
	roommedia::TrackPeerProducer(peer, producer);
	watchProducerScore(roomId, producer);
	indexPeerProducers(roomId, peerId, peer->producers);

	roommedia::AutoSubscribeProducerToOtherPeers(
		roomId, peerId, room, producer, logger_, notify_, false);

	return {true, {{"id", producer->id()}}};
}

RoomService::Result RoomService::consume(const std::string& roomId,
	const std::string& peerId, const std::string& transportId,
	const std::string& producerId, const json& rtpCapabilities)
{
	if (!rtpCapabilities.is_object()) {
		MS_WARN(logger_, "[{} {}] consume validation failed: invalid rtpCapabilities type", roomId, peerId);
		return {false, {}, "", "invalid rtpCapabilities"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto transport = peer->getTransport(transportId);
	if (!transport) return {false, {}, "", "transport not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) return {false, {}, "", "producer not found"};

	json consumeOpts = {
		{"producerId", producerId},
		{"rtpCapabilities", rtpCapabilities},
		{"consumableRtpParameters", producer->consumableRtpParameters()}
	};
	auto consumer = transport->consume(consumeOpts);
	roommedia::TrackPeerConsumer(peer, consumer);
	return {true, consumer->toJson()};
}

RoomService::Result RoomService::pauseProducer(const std::string& roomId,
	const std::string& producerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) return {false, {}, "", "producer not found"};
	producer->pause();
	return {true, {}};
}

RoomService::Result RoomService::resumeProducer(const std::string& roomId,
	const std::string& producerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (!producer) return {false, {}, "", "producer not found"};
	producer->resume();
	return {true, {}};
}

RoomService::Result RoomService::restartIce(const std::string& roomId,
	const std::string& peerId, const std::string& transportId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto wt = std::dynamic_pointer_cast<WebRtcTransport>(peer->getTransport(transportId));
	if (!wt) return {false, {}, "", "transport not found"};
	return {true, wt->restartIce()};
}

RoomService::Result RoomService::setQosOverride(
	const std::string& roomId, const std::string& callerPeerId,
	const std::string& targetPeerId, const json& overrideData)
{
	if (callerPeerId != targetPeerId)
		return {false, {}, "", "permission denied: can only set QoS override for self"};

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(targetPeerId);
	if (!peer) return {false, {}, "", "peer not found"};

	auto parsed = qos::QosValidator::ParseOverride(overrideData);
	if (!parsed.ok) return {false, {}, "", "invalid qosOverride: " + parsed.error};

	if (notify_) {
		notify_(roomId, targetPeerId, {
			{"notification", true},
			{"method", "qosOverride"},
			{"data", qos::ToJson(parsed.value)}
		});
	}

	return {true, json::object()};
}

RoomService::Result RoomService::setQosPolicy(
	const std::string& roomId, const std::string& callerPeerId,
	const std::string& targetPeerId, const json& policyData)
{
	if (callerPeerId != targetPeerId)
		return {false, {}, "", "permission denied: can only set QoS policy for self"};

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(targetPeerId);
	if (!peer) return {false, {}, "", "peer not found"};

	auto parsed = qos::QosValidator::ParsePolicy(policyData);
	if (!parsed.ok) return {false, {}, "", "invalid qosPolicy: " + parsed.error};

	if (notify_) {
		notify_(roomId, targetPeerId, {
			{"notification", true},
			{"method", "qosPolicy"},
			{"data", qos::ToJson(parsed.value)}
		});
	}

	return {true, json::object()};
}

RoomService::Result RoomService::pauseConsumer(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	it->second->pause();
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::resumeConsumer(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	it->second->resume();
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::getConsumerState(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::setConsumerPreferredLayers(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId,
	uint8_t spatialLayer, uint8_t temporalLayer)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	it->second->setPreferredLayers(spatialLayer, temporalLayer);
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::setConsumerPriority(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId, uint8_t priority)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	it->second->setPriority(priority);
	subscriberControllers_[roomstatsqos::MakePeerKey(roomId, peerId)].syncConsumerState(peer->consumers);
	return {true, it->second->toJson()};
}

RoomService::Result RoomService::requestConsumerKeyFrame(const std::string& roomId,
	const std::string& peerId, const std::string& consumerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto it = peer->consumers.find(consumerId);
	if (it == peer->consumers.end() || !it->second || it->second->closed())
		return {false, {}, "", "consumer not found"};
	it->second->requestKeyFrame();
	return {true, it->second->toJson()};
}

} // namespace mediasoup
