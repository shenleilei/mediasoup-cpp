#include "RoomService.h"

#include "RoomMediaHelpers.h"
#include "RoomRecordingHelpers.h"
#include "RoomStatsQosHelpers.h"

namespace mediasoup {
namespace {

constexpr const char* kPlainClientH264BaselineProfileLevelId = "42e01f";

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

} // namespace

RoomService::Result RoomService::createTransport(const std::string& roomId,
	const std::string& peerId, bool producing, bool consuming)
{
	if (producing == consuming)
		return {false, {}, "", "exactly one of producing or consuming must be true"};

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};

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
	const std::string& peerId, const std::vector<uint32_t>& videoSsrcs, uint32_t audioSsrc)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	if (videoSsrcs.empty()) return {false, {}, "", "videoSsrcs cannot be empty"};
	if (audioSsrc == 0) return {false, {}, "", "audioSsrc must be non-zero"};

	std::unordered_set<uint32_t> uniqueSsrcs;
	uniqueSsrcs.insert(audioSsrc);
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

	std::optional<RtpCodecCapability> videoCodec;
	uint8_t audioPt = 0;
	for (auto& c : caps.codecs) {
		if (!videoCodec.has_value() && IsPlainClientBaselineH264Codec(c))
			videoCodec = c;
		if (c.mimeType == "audio/opus" && audioPt == 0) audioPt = c.preferredPayloadType;
	}
	if (!videoCodec.has_value())
		return {false, {}, "", "router has no H264 Baseline codec"};
	if (audioPt == 0) return {false, {}, "", "router has no opus codec"};

	const uint8_t videoPt = videoCodec->preferredPayloadType;
	json videoCodecParameters = videoCodec->parameters;

	std::vector<std::shared_ptr<Producer>> videoProducers;
	videoProducers.reserve(videoSsrcs.size());
	for (size_t index = 0; index < videoSsrcs.size(); ++index) {
		uint32_t videoSsrc = videoSsrcs[index];
		json videoRtpParams = {
			{"codecs", {{
				{"mimeType", "video/H264"}, {"payloadType", videoPt},
				{"clockRate", 90000},
				{"parameters", videoCodecParameters}
			}}},
			{"encodings", {{{"ssrc", videoSsrc}}}},
			{"rtcp", {{"cname", peerId + "-video-" + std::to_string(index)}}}
		};
		json videoProdOpts = {
			{"kind", "video"}, {"rtpParameters", videoRtpParams},
			{"routerRtpCapabilities", caps}
		};
		auto videoProd = transport->produce(videoProdOpts);
		room->router()->addProducer(videoProd);
		roommedia::TrackPeerProducer(peer, videoProd);
		videoProducers.push_back(videoProd);
	}

	json audioRtpParams = {
		{"codecs", {{
			{"mimeType", "audio/opus"}, {"payloadType", audioPt},
			{"clockRate", 48000}, {"channels", 2},
			{"parameters", {{"useinbandfec", 1}}}
		}}},
		{"encodings", {{{"ssrc", audioSsrc}}}},
		{"rtcp", {{"cname", peerId + "-audio"}}}
	};
	json audioProdOpts = {
		{"kind", "audio"}, {"rtpParameters", audioRtpParams},
		{"routerRtpCapabilities", caps}
	};
	auto audioProd = transport->produce(audioProdOpts);
	room->router()->addProducer(audioProd);
	roommedia::TrackPeerProducer(peer, audioProd);

	indexPeerProducers(roomId, peerId, peer->producers);

	std::vector<std::shared_ptr<Producer>> allProducers = videoProducers;
	allProducers.push_back(audioProd);
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
			{"producerId", videoProducers[index]->id()}
		});
	}
	return {true, {
		{"transportId", transport->id()},
		{"ip", tuple.localAddress}, {"port", tuple.localPort},
		{"videoPt", videoPt}, {"videoSsrc", videoSsrcs.front()},
		{"videoProdId", videoProducers.front()->id()},
		{"videoTracks", videoTracks},
		{"audioPt", audioPt}, {"audioSsrc", audioSsrc},
		{"audioProdId", audioProd->id()}
	}};
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
	const std::string& kind, const json& rtpParameters)
{
	if (kind != "audio" && kind != "video") {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid kind '{}'", roomId, peerId, kind);
		return {false, {}, "", "invalid kind"};
	}
	if (!rtpParameters.is_object()) {
		MS_WARN(logger_, "[{} {}] produce validation failed: invalid rtpParameters type", roomId, peerId);
		return {false, {}, "", "invalid rtpParameters"};
	}

	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto peer = room->getPeer(peerId);
	if (!peer) return {false, {}, "", "peer not found"};
	auto transport = peer->getTransport(transportId);
	if (!transport) return {false, {}, "", "transport not found"};

	json produceOpts = {
		{"kind", kind}, {"rtpParameters", rtpParameters},
		{"routerRtpCapabilities", room->router()->rtpCapabilities()}
	};
	auto producer = transport->produce(produceOpts);
	room->router()->addProducer(producer);
	roommedia::TrackPeerProducer(peer, producer);
	indexPeerProducers(roomId, peerId, peer->producers);

	roommedia::AutoSubscribeProducerToOtherPeers(
		roomId, peerId, room, producer, logger_, notify_, false);

	autoRecord(roomId, peerId, room, producer);
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
	if (producer) producer->pause();
	return {true, {}};
}

RoomService::Result RoomService::resumeProducer(const std::string& roomId,
	const std::string& producerId)
{
	auto room = roomManager_.getRoom(roomId);
	if (!room) return {false, {}, "", "room not found"};
	auto producer = room->router()->getProducerById(producerId);
	if (producer) producer->resume();
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

void RoomService::autoRecord(const std::string& roomId, const std::string& peerId,
	std::shared_ptr<Room> room, std::shared_ptr<Producer> producer)
{
	roomrecording::AutoRecordProducer(
		roomId,
		peerId,
		room,
		producer,
		recordDir_,
		recorders_,
		recorderTransports_,
		logger_);
}

} // namespace mediasoup
