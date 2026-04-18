#pragma once

#include "SignalingServer.h"
#include "SignalingSocketState.h"

#include <App.h>

namespace mediasoup {

struct SignalingServerWs {
	using DownlinkRateLimitMap = std::unordered_map<std::string, DownlinkStatsRateLimitState>;

	static void ConfigureWorkerCallbacks(
		SignalingServer& server,
		const std::shared_ptr<WsMap>& wsMap,
		uWS::Loop* loop,
		const std::shared_ptr<DownlinkRateLimitMap>& downlinkStatsRateLimit);

	static bool EnsureWorkersReady(
		SignalingServer& server,
		const std::function<void(bool)>& notifyStartup);

	static void RegisterWebSocketRoutes(
		uWS::App& app,
		SignalingServer& server,
		const std::shared_ptr<WsMap>& wsMap,
		uWS::Loop* loop,
		const std::shared_ptr<DownlinkRateLimitMap>& downlinkStatsRateLimit);
};

} // namespace mediasoup
