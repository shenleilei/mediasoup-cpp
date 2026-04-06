#include "PipeTransport.h"
#include "pipeTransport_generated.h"
#include "request_generated.h"
#include "srtpParameters_generated.h"

namespace mediasoup {

json PipeTransport::connect(const std::string& ip, uint16_t port, const json& srtpParameters) {
	if (closed_) throw std::runtime_error("Transport closed");

	auto& builder = channel_->bufferBuilder();

	flatbuffers::Offset<FBS::SrtpParameters::SrtpParameters> srtpOff = 0;
	if (!srtpParameters.empty()) {
		auto cryptoSuite = FBS::SrtpParameters::SrtpCryptoSuite::AES_CM_128_HMAC_SHA1_80;
		std::string cs = srtpParameters.value("cryptoSuite", "AES_CM_128_HMAC_SHA1_80");
		if (cs == "AEAD_AES_256_GCM")
			cryptoSuite = FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_256_GCM;
		else if (cs == "AEAD_AES_128_GCM")
			cryptoSuite = FBS::SrtpParameters::SrtpCryptoSuite::AEAD_AES_128_GCM;

		std::string keyBase64 = srtpParameters.value("keyBase64", "");
		srtpOff = FBS::SrtpParameters::CreateSrtpParameters(
			builder, cryptoSuite, builder.CreateString(keyBase64));
	}

	auto reqOff = FBS::PipeTransport::CreateConnectRequest(
		builder, builder.CreateString(ip),
		port > 0 ? flatbuffers::Optional<uint16_t>(port) : flatbuffers::Optional<uint16_t>(),
		srtpOff);

	channel_->requestWait(
		FBS::Request::Method::PIPETRANSPORT_CONNECT,
		FBS::Request::Body::PipeTransport_ConnectRequest,
		reqOff.Union(), id_);

	return {{"connected", true}};
}

} // namespace mediasoup
