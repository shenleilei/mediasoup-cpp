#include "SignalingServerHttp.h"

#include "Logger.h"
#include "StaticFileResponder.h"
#include "WorkerThread.h"

#include <App.h>
#include <atomic>
#include <chrono>
#include <sstream>

extern std::atomic<bool> g_shutdown;

namespace mediasoup {
namespace {

std::atomic<uint64_t> g_nextResolveRequestId{1};

template<typename Response, typename Request>
std::string ResolveClientIp(Response* res, Request* req)
{
	std::string clientIp;
	// Ignore the query parameter "clientIp" to avoid spoofing
	std::string xff(req->getHeader("x-forwarded-for"));
	if (!xff.empty()) {
		auto comma = xff.find(',');
		clientIp = (comma != std::string::npos) ? xff.substr(0, comma) : xff;
	}
	if (clientIp.empty()) {
		clientIp = std::string(res->getRemoteAddressAsText());
	}
	return clientIp;
}

} // namespace

void SignalingServerHttp::RegisterHttpRoutes(uWS::SSLApp& app, SignalingServer& server, uWS::Loop* loop)
{
	app.get("/api/resolve", [&server, loop](auto* res, auto* req) {
		const auto requestId = g_nextResolveRequestId.fetch_add(1, std::memory_order_relaxed);
		std::string roomId(req->getQuery("roomId"));
		if (roomId.empty()) {
			res->writeStatus("400 Bad Request")->writeHeader("Content-Type", "application/json")
				->end(R"({"error":"roomId required"})");
			return;
		}
		const std::string clientIp = ResolveClientIp(res, req);
		const auto requestStart = std::chrono::steady_clock::now();
		(void)loop;
		(void)clientIp;
		(void)requestStart;
		spdlog::debug("api.resolve received [req:{} roomId:{} local-only:true]", requestId, roomId);
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(R"({"wsUrl":"","isNew":true})");
	});

	app.get("/api/node-load", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		json load = {
			{"rooms", snapshot.totalRooms},
			{"maxRooms", snapshot.totalMaxRooms},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", server.workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"healthy", server.isHealthy(snapshot)},
			{"ready", server.isReady(snapshot)},
				{"dispatchRooms", snapshot.dispatchRooms},
				{"staleRequestDrops", snapshot.staleRequestDrops},
				{"rejectedClientStats", snapshot.rejectedClientStats},
				{"joinFailures", snapshot.joinFailures},
				{"plainPublishFailures", snapshot.plainPublishFailures},
				{"plainSubscribeFailures", snapshot.plainSubscribeFailures},
				{"workerDeaths", snapshot.workerDeaths},
				{"workerRespawns", snapshot.workerRespawns},
				{"wsDisconnects", snapshot.wsDisconnects},
				{"malformedWsMessages", snapshot.malformedWsMessages},
				{"workerQueueStats", snapshot.workerQueues},
				{"roomOwnership", snapshot.roomOwnership}
			};
		res->writeHeader("Content-Type", "application/json")
			->writeHeader("Access-Control-Allow-Origin", "*")
			->end(load.dump());
	});

	app.get("/healthz", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		bool healthy = server.isHealthy(snapshot);
		json health = {
			{"ok", healthy},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"ready", server.isReady(snapshot)},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", server.workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads},
			{"rooms", snapshot.totalRooms},
			{"maxRooms", snapshot.totalMaxRooms}
		};
		if (healthy) {
			res->writeHeader("Content-Type", "application/json")->end(health.dump());
		} else {
			res->writeStatus("503 Service Unavailable")
				->writeHeader("Content-Type", "application/json")
				->end(health.dump());
		}
	});

	app.get("/readyz", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		bool ready = server.isReady(snapshot);
		json readiness = {
			{"ok", ready},
			{"startupSucceeded", snapshot.startupSucceeded},
			{"shutdownRequested", snapshot.shutdownRequested},
			{"workers", snapshot.totalWorkers},
			{"workerThreads", server.workerThreads_.size()},
			{"availableWorkerThreads", snapshot.availableWorkerThreads}
		};
		if (ready) {
			res->writeHeader("Content-Type", "application/json")->end(readiness.dump());
		} else {
			res->writeStatus("503 Service Unavailable")
				->writeHeader("Content-Type", "application/json")
				->end(readiness.dump());
		}
	});

	app.get("/metrics", [&server](auto* res, auto*) {
		auto snapshot = server.collectRuntimeSnapshot();
		res->writeHeader("Content-Type", "text/plain; version=0.0.4")
			->end(server.buildPrometheusMetrics(snapshot));
	});

	app.get("/*", [](auto* res, auto* req) {
		std::string url(req->getUrl());
		if (url == "/") url = "/index.html";
		bool forbidden = false;
		auto resolved = ResolveFileUnderBase("public", url, forbidden);
		if (!resolved) {
			if (forbidden) res->writeStatus("403 Forbidden")->end("Forbidden");
			else res->writeStatus("404 Not Found")->end("Not Found");
			return;
		}
		ServeResolvedFile(res, *resolved, ContentTypeForPath(url));
	});
}

void SignalingServerHttp::StartBackgroundTimers(
	SignalingServer& server,
	uWS::Loop* loop,
	us_listen_socket_t* listenSocket,
	us_timer_t*& statsTimer,
	us_timer_t*& shutdownTimer)
{
	statsTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(SignalingServer*));
	SignalingServer* self = &server;
	memcpy(us_timer_ext(statsTimer), &self, sizeof(SignalingServer*));
	us_timer_set(statsTimer, [](struct us_timer_t* t) {
		SignalingServer* s;
		memcpy(&s, us_timer_ext(t), sizeof(SignalingServer*));
		for (auto& wt : s->workerThreads_) {
			wt->post([wtp = wt.get()] {
				try {
					if (wtp->roomService())
						wtp->roomService()->broadcastStats();
				} catch (const std::exception& e) {
					spdlog::error("broadcastStats exception: {}", e.what());
				}
			});
		}
	}, kStatsBroadcastIntervalMs, kStatsBroadcastIntervalMs);

	(void)loop;

	struct ShutdownCtx { us_listen_socket_t* sock; };
	shutdownTimer = us_create_timer((struct us_loop_t*)loop, 0, sizeof(ShutdownCtx));
	ShutdownCtx sctx{listenSocket};
	memcpy(us_timer_ext(shutdownTimer), &sctx, sizeof(ShutdownCtx));
	us_timer_set(shutdownTimer, [](struct us_timer_t* t) {
		auto* ctx = static_cast<ShutdownCtx*>(us_timer_ext(t));
		if (g_shutdown && ctx && ctx->sock) {
			us_listen_socket_close(0, ctx->sock);
			ctx->sock = nullptr;
		}
	}, kShutdownPollIntervalMs, kShutdownPollIntervalMs);
}

} // namespace mediasoup
