#pragma once

#include "rtpStream_generated.h"
#include <nlohmann/json.hpp>

namespace mediasoup {

namespace detail {

inline bool PopulateRtpStreamBaseStats(
	const FBS::RtpStream::BaseStats* base,
	const char* type,
	nlohmann::json& entry)
{
	if (!base) {
		return false;
	}

	entry["type"] = type;
	entry["ssrc"] = base->ssrc();
	entry["kind"] = base->kind() == FBS::RtpParameters::MediaKind::AUDIO
		? "audio"
		: "video";
	entry["mimeType"] = base->mime_type() ? base->mime_type()->str() : "";
	entry["packetsLost"] = base->packets_lost();
	entry["fractionLost"] = base->fraction_lost();
	entry["nackCount"] = base->nack_count();
	entry["pliCount"] = base->pli_count();
	entry["firCount"] = base->fir_count();
	entry["score"] = base->score();
	entry["roundTripTime"] = base->round_trip_time();

	return true;
}

} // namespace detail

inline nlohmann::json ProducerRecvStatsToJson(const FBS::RtpStream::Stats* stat)
{
	if (!stat || stat->data_type() != FBS::RtpStream::StatsData::RecvStats) {
		return nlohmann::json::object();
	}

	auto* recv = stat->data_as_RecvStats();
	if (!recv || !recv->base() || !recv->base()->data()) {
		return nlohmann::json::object();
	}

	auto* base = recv->base()->data_as_BaseStats();
	nlohmann::json entry;
	if (!detail::PopulateRtpStreamBaseStats(base, "inbound-rtp", entry)) {
		return nlohmann::json::object();
	}

	entry["packetsDiscarded"] = base->packets_discarded();
	entry["packetsRetransmitted"] = base->packets_retransmitted();
	entry["packetsRepaired"] = base->packets_repaired();
	entry["nackPacketCount"] = base->nack_packet_count();
	if (base->rid()) {
		entry["rid"] = base->rid()->str();
	}
	entry["jitter"] = recv->jitter();
	entry["packetCount"] = recv->packet_count();
	entry["byteCount"] = recv->byte_count();
	entry["bitrate"] = recv->bitrate();

	return entry;
}

inline nlohmann::json ConsumerSendStatsToJson(const FBS::RtpStream::Stats* stat)
{
	if (!stat || stat->data_type() != FBS::RtpStream::StatsData::SendStats) {
		return nlohmann::json::object();
	}

	auto* send = stat->data_as_SendStats();
	if (!send || !send->base() || !send->base()->data()) {
		return nlohmann::json::object();
	}

	auto* base = send->base()->data_as_BaseStats();
	nlohmann::json entry;
	if (!detail::PopulateRtpStreamBaseStats(base, "outbound-rtp", entry)) {
		return nlohmann::json::object();
	}

	entry["packetCount"] = send->packet_count();
	entry["byteCount"] = send->byte_count();
	entry["bitrate"] = send->bitrate();

	return entry;
}

} // namespace mediasoup
