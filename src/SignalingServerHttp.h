#pragma once

#include "SignalingServer.h"

#include <App.h>

struct us_listen_socket_t;
struct us_timer_t;

namespace mediasoup {

struct SignalingServerHttp {
	static void RegisterHttpRoutes(uWS::App& app, SignalingServer& server, uWS::Loop* loop);
	static void StartBackgroundTimers(
		SignalingServer& server,
		uWS::Loop* loop,
		us_listen_socket_t* listenSocket,
		us_timer_t*& statsTimer,
		us_timer_t*& redisTimer);
};

} // namespace mediasoup
