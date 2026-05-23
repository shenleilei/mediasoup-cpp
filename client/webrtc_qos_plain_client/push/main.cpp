#include <atomic>
#include <csignal>
#include <iostream>

#include "common/ClientArgs.h"
#include "common/RuntimeLogHelpers.h"
#include "push/PushSignalingSession.h"
#include "push/WebRtcQosPushRuntime.h"

namespace {

std::atomic<bool> gRunning{true};

void HandleSignal(int)
{
	gRunning.store(false);
}

} // namespace

int main(int argc, char* argv[])
{
	std::signal(SIGINT, HandleSignal);
	std::signal(SIGTERM, HandleSignal);

	webrtc_qos_plain::PushOptions options;
	std::string error;
	if (!webrtc_qos_plain::ParsePushOptions(argc, argv, &options, &error)) {
		std::cerr << error << "\n";
		return error.find("webrtc-qos-plain-push-client") == 0 ? 0 : 2;
	}

	auto logger = webrtc_qos_plain::CreateClientLogger("push", options.logDir);
	logger->info("push_client_start roomId={} peerId={} input={} logDir={}",
		options.room, options.peer, options.input, options.logDir);

	webrtc_qos_plain::PushSignalingSession signaling(logger);
	webrtc_qos_plain::PublishInfo publishInfo;
	if (!signaling.ConnectAndPublish(options, &publishInfo)) return 2;

	webrtc_qos_plain::WebRtcQosPushRuntime runtime(options, publishInfo, logger);
	const int rc = runtime.Run(gRunning, &signaling);
	signaling.Close();
	return rc;
}
