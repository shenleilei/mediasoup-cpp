#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <utility>

#include "common/ClientArgs.h"
#include "common/PlainUdpTransport.h"
#include "common/RuntimeLogHelpers.h"
#include "play/PlaySignalingSession.h"
#include "play/WebRtcQosPlayRuntime.h"

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

	webrtc_qos_plain::PlayOptions options;
	std::string error;
	if (!webrtc_qos_plain::ParsePlayOptions(argc, argv, &options, &error)) {
		std::cerr << error << "\n";
		return error.find("webrtc-qos-plain-play-client") == 0 ? 0 : 2;
	}

	auto logger = webrtc_qos_plain::CreateClientLogger("play", options.logDir);
	logger->info("play_client_start roomId={} peerId={} listenIp={} advertiseIp={} listenPort={} logDir={}",
		options.room,
		options.peer,
		options.listenIp,
		options.advertiseIp,
		options.listenPort,
		options.logDir);

	webrtc_qos_plain::PlainUdpTransport udp;
	if (!udp.Bind(options.listenIp, options.listenPort, &error)) {
		logger->error("udp_bind_failed listenIp={} listenPort={} error={}",
			options.listenIp,
			options.listenPort,
			error);
		return 2;
	}
	logger->info("udp_bound listenIp={} listenPort={} localIp={} localPort={}",
		options.listenIp,
		options.listenPort,
		udp.localEndpoint().ip,
		udp.localEndpoint().port);
	options.listenPort = udp.localEndpoint().port;

	webrtc_qos_plain::PlaySignalingSession signaling(logger);
	if (!signaling.ConnectJoinAndSubscribe(options)) return 2;

	const int64_t deadlineUs =
		webrtc_qos_plain::MonotonicNowUs() +
		static_cast<int64_t>(options.waitConsumerTimeoutMs) * 1000;
	std::optional<webrtc_qos_plain::ConsumerInfo> consumer;
	while (gRunning.load() && webrtc_qos_plain::MonotonicNowUs() < deadlineUs) {
		signaling.DispatchNotifications();
		consumer = signaling.TakeSelectedConsumer(options);
		if (consumer) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(options.processTickMs));
	}
	if (!consumer) {
		logger->error("wait_consumer_timeout timeoutMs={}", options.waitConsumerTimeoutMs);
		signaling.Close();
		return 3;
	}

	signaling.RequestConsumerKeyFrame(consumer->consumerId);
	std::thread delayedKeyframe([&signaling, consumerId = consumer->consumerId]() {
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		signaling.RequestConsumerKeyFrame(consumerId);
	});

	webrtc_qos_plain::WebRtcQosPlayRuntime runtime(options, *consumer, logger, std::move(udp));
	const int rc = runtime.Run(gRunning, &signaling);
	if (delayedKeyframe.joinable()) delayedKeyframe.join();
	signaling.Close();
	return rc;
}
