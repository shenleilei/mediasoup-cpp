#pragma once

#include "rtpStream_generated.h"
#include <nlohmann/json.hpp>

namespace mediasoup {

namespace detail {

inline int64_t FixedQ32Dot32ToMs(int64_t raw)
{
	return static_cast<int64_t>(
		std::llround((static_cast<long double>(raw) * 1000.0L) / static_cast<long double>(1ULL << 32)));
}

inline uint64_t Ntp64ToUnixMs(uint64_t raw)
{
	constexpr uint32_t unixNtpOffset = 0x83AA7E80;
	constexpr uint64_t ntpFractionalUnit = 1ULL << 32;

	const uint32_t seconds = static_cast<uint32_t>(raw >> 32);
	const uint32_t fractions = static_cast<uint32_t>(raw & 0xFFFFFFFFULL);
	const uint64_t unixSeconds =
		seconds >= unixNtpOffset ? static_cast<uint64_t>(seconds - unixNtpOffset) : 0u;
	const uint64_t fractionalMs = static_cast<uint64_t>(
		std::llround((static_cast<long double>(fractions) * 1000.0L) / ntpFractionalUnit));

	return unixSeconds * 1000u + fractionalMs;
}

inline const FBS::RtpStream::BaseStats* ExtractNestedBaseStats(const FBS::RtpStream::Stats* nested)
{
	if (!nested) {
		return nullptr;
	}

	return nested->data_as_BaseStats();
}

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
	if (base->abs_capture_timestamp_ntp().has_value()) {
		entry["absCaptureTimestampNtp"] = std::to_string(base->abs_capture_timestamp_ntp().value());
		entry["absCaptureTimestampMs"] = base->abs_capture_timestamp_ms().has_value()
			? base->abs_capture_timestamp_ms().value()
			: Ntp64ToUnixMs(base->abs_capture_timestamp_ntp().value());
	}
	if (base->estimated_capture_clock_offset().has_value()) {
		entry["estimatedCaptureClockOffset"] = std::to_string(base->estimated_capture_clock_offset().value());
		entry["estimatedCaptureClockOffsetMs"] = base->estimated_capture_clock_offset_ms().has_value()
			? base->estimated_capture_clock_offset_ms().value()
			: FixedQ32Dot32ToMs(base->estimated_capture_clock_offset().value());
	}
	if (base->abs_capture_receive_delta_ms().has_value()) {
		entry["absCaptureReceiveDeltaMs"] = base->abs_capture_receive_delta_ms().value();
	}

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

	auto* base = detail::ExtractNestedBaseStats(recv->base());
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

	auto* base = detail::ExtractNestedBaseStats(send->base());
	nlohmann::json entry;
	if (!detail::PopulateRtpStreamBaseStats(base, "outbound-rtp", entry)) {
		return nlohmann::json::object();
	}

	entry["packetCount"] = send->packet_count();
	entry["byteCount"] = send->byte_count();
	entry["bitrate"] = send->bitrate();

	return entry;
}

inline nlohmann::json AnyRtpStreamStatsToJson(const FBS::RtpStream::Stats* stat)
{
	if (!stat) {
		return nlohmann::json::object();
	}

	switch (stat->data_type()) {
		case FBS::RtpStream::StatsData::RecvStats:
			return ProducerRecvStatsToJson(stat);
		case FBS::RtpStream::StatsData::SendStats:
			return ConsumerSendStatsToJson(stat);
		default:
			return nlohmann::json::object();
	}
}

} // namespace mediasoup
