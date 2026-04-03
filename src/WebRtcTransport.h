#pragma once
#include "Transport.h"
#include <nlohmann/json.hpp>

namespace mediasoup {

struct IceParameters {
	std::string usernameFragment;
	std::string password;
	bool iceLite = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IceParameters, usernameFragment, password, iceLite)

struct IceCandidate {
	std::string foundation;
	uint32_t priority = 0;
	std::string address;
	std::string protocol = "udp";
	uint16_t port = 0;
	std::string type = "host";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(IceCandidate, foundation, priority, address, protocol, port, type)

struct DtlsFingerprint {
	std::string algorithm;
	std::string value;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DtlsFingerprint, algorithm, value)

struct DtlsParameters {
	std::vector<DtlsFingerprint> fingerprints;
	std::string role = "auto";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(DtlsParameters, fingerprints, role)

struct TransportTuple {
	std::string localAddress;
	uint16_t localPort = 0;
	std::string remoteIp;
	uint16_t remotePort = 0;
	std::string protocol = "udp";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(TransportTuple, localAddress, localPort, remoteIp, remotePort, protocol)

class WebRtcTransport : public Transport {
public:
	WebRtcTransport(const std::string& id, Channel* channel, const std::string& routerId,
		const IceParameters& iceParams, const std::vector<IceCandidate>& iceCandidates,
		const DtlsParameters& dtlsParams)
		: Transport(id, channel, routerId)
		, iceParameters_(iceParams)
		, iceCandidates_(iceCandidates)
		, dtlsParameters_(dtlsParams) {}

	const IceParameters& iceParameters() const { return iceParameters_; }
	const std::vector<IceCandidate>& iceCandidates() const { return iceCandidates_; }
	const DtlsParameters& dtlsParameters() const { return dtlsParameters_; }
	const std::string& iceState() const { return iceState_; }
	const std::string& dtlsState() const { return dtlsState_; }

	json connect(const DtlsParameters& dtlsParameters);
	json restartIce();

	json toJson() const {
		return {
			{"id", id_},
			{"iceParameters", iceParameters_},
			{"iceCandidates", iceCandidates_},
			{"dtlsParameters", dtlsParameters_}
		};
	}

	void handleNotification(FBS::Notification::Event event, const FBS::Notification::Notification* notification);

private:
	IceParameters iceParameters_;
	std::vector<IceCandidate> iceCandidates_;
	DtlsParameters dtlsParameters_;
	std::string iceState_ = "new";
	std::string dtlsState_ = "new";
};

} // namespace mediasoup
