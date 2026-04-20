#include "SignalingServerWs.h"

#include "SignalingRequestDispatcher.h"
#include "WorkerThread.h"
#include "qos/QosValidator.h"

#include <atomic>
#include <random>

namespace mediasoup {

namespace {

std::atomic<uint64_t> g_nextSessionId{0};

uint64_t initSessionIdBase() {
	std::random_device rd;
	uint64_t base = (static_cast<uint64_t>(rd()) << 32) | rd();
	base = (base & 0x0000FFFFFFFFFFFF) | 0x0001000000000000;
	g_nextSessionId.store(base, std::memory_order_relaxed);
	return base;
}

uint64_t g_sessionIdBase [[maybe_unused]] = initSessionIdBase();

json BuildWorkerCompletedResponse(uint64_t id, const RoomService::Result& result)
{
	if (result.ok) {
		return {
			{"response", true},
			{"id", id},
			{"ok", true},
			{"data", result.data}
		};
	}

	return {
		{"response", true},
		{"id", id},
		{"ok", false},
		{"error", result.error}
	};
}

} // namespace

void SignalingServerWs::ConfigureWorkerCallbacks(
	SignalingServer& server,
	const std::shared_ptr<WsMap>& wsMap,
	uWS::Loop* loop,
	const std::shared_ptr<DownlinkRateLimitMap>& downlinkStatsRateLimit)
{
	for (auto& wt : server.workerThreads_) {
		WorkerThread* wtRaw = wt.get();
		wt->setNotify([wsMap, loop](const std::string& roomId, const std::string& peerId, const json& msg) {
			PostNotify(loop, wsMap, roomId, peerId, msg.dump());
		});

		wt->setBroadcast([wsMap, loop](const std::string& roomId,
			const std::string& excludePeerId, const json& msg)
		{
			PostBroadcast(loop, wsMap, roomId, excludePeerId, msg.dump());
		});

		wt->setRoomLifecycle([&server, loop, wtRaw](const std::string& roomId, bool created) {
			loop->defer([&server, roomId, wtRaw, created] {
				if (created) {
					server.destroyedRooms_.erase(roomId);
					return;
				}
				server.destroyedRooms_.insert(roomId);
				auto it = server.roomDispatch_.find(roomId);
				if (it != server.roomDispatch_.end() && it->second == wtRaw) {
					server.roomDispatch_.erase(it);
				}
			});
		});

		wt->setRegistryTask([&server](std::function<void()> task) {
			server.enqueueRegistryTask(std::move(task), "worker.registryTask");
		});
		wt->setTaskPoster([wtRaw](std::function<void()> task) {
			wtRaw->post(std::move(task));
		});
		wt->setDownlinkSnapshotApplied([loop, downlinkStatsRateLimit](
			const std::string& roomId, const std::string& peerId, uint64_t seq) {
			const auto mapKey = WsMap::key(roomId, peerId);
			loop->defer([downlinkStatsRateLimit, mapKey, seq] {
				auto it = downlinkStatsRateLimit->find(mapKey);
				if (it == downlinkStatsRateLimit->end()) return;
				if (it->second.pending && it->second.pendingSeq == seq)
					it->second.pending = false;
			});
		});

		wt->start();
	}
}

bool SignalingServerWs::EnsureWorkersReady(
	SignalingServer& server,
	const std::function<void(bool)>& notifyStartup)
{
	for (auto& wt : server.workerThreads_) {
		wt->waitReady();
	}

	bool anyWorkers = false;
	for (auto& wt : server.workerThreads_) {
		if (wt->workerCount() > 0) {
			anyWorkers = true;
			break;
		}
	}
	if (!anyWorkers) {
		spdlog::error("No mediasoup workers available, refusing to listen");
		notifyStartup(false);
		server.running_ = false;
		server.stopRegistryWorker();
		return false;
	}

	return true;
}

void SignalingServerWs::RegisterWebSocketRoutes(
	uWS::App& app,
	SignalingServer& server,
	const std::shared_ptr<WsMap>& wsMap,
	uWS::Loop* loop,
	const std::shared_ptr<DownlinkRateLimitMap>& downlinkStatsRateLimit)
{
	app.ws<PerSocketData>("/ws", {
		.compression = uWS::DISABLED,
		.maxPayloadLength = kWsMaxPayloadBytes,
		.idleTimeout = kWsIdleTimeoutSec,

		.open = [wsMap](auto* ws) {
			auto* d = ws->getUserData();
			d->peerId = "";
			d->roomId = "";
		},

		.message = [&server, wsMap, loop, downlinkStatsRateLimit](auto* ws, std::string_view message, uWS::OpCode) {
			try {
				auto req = json::parse(message);
				auto requestIt = req.find("request");
				if (requestIt == req.end() || !requestIt->is_boolean() || !requestIt->get<bool>()) return;

			auto* sd = ws->getUserData();
			if (!sd->alive->load()) return;

			std::string method = req.value("method", "");
			uint64_t id = req.value("id", 0);
			json data = req.value("data", json::object());
			std::string roomId = sd->roomId;
			std::string peerId = sd->peerId;
			uint64_t sessionId = sd->sessionId;

			if (method != "clientStats") {
				spdlog::debug("[{} {}] {} id={}", roomId, peerId, method, id);
			}

			if (method == "clientStats") {
				if (roomId.empty() || peerId.empty() || sessionId == kInvalidSessionId) {
					server.rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "clientStats requires joined session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				bool validSession = HasMappedSession(wsMap, ws, roomId, peerId, sessionId);
				if (!validSession) {
					server.staleRequestDrops_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "stale session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto* wt = server.getWorkerThread(roomId, false);
				if (!wt || !wt->roomService()) {
					server.rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "room not assigned"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto qosParse = qos::QosValidator::ParseClientSnapshot(data);
				if (!qosParse.ok) {
					server.rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "invalid clientStats: " + qosParse.error}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto alive = sd->alive;
				wt->post([wt, ws, alive, loop, id, roomId, peerId,
					snapshot = std::move(qosParse.value)]() mutable {
					RoomService::Result result;
					try {
						auto* rs = wt->roomService();
						if (!rs) {
							result = {false, json::object(), "", "worker not ready"};
						} else {
							result = rs->setClientStats(roomId, peerId, std::move(snapshot));
						}
					} catch (const std::exception& e) {
						spdlog::error("[{} {}] clientStats error: {}", roomId, peerId, e.what());
						result = {false, json::object(), "", e.what()};
					} catch (...) {
						spdlog::error("[{} {}] clientStats error: unknown error", roomId, peerId);
						result = {false, json::object(), "", "unknown clientStats error"};
					}

					std::string respStr = BuildWorkerCompletedResponse(id, result).dump();
					loop->defer([ws, alive, respStr = std::move(respStr)] {
						if (!alive->load(std::memory_order_relaxed)) return;
						ws->send(respStr, uWS::OpCode::TEXT);
					});
				});
				return;
			}

			if (method == "downlinkClientStats") {
				static constexpr int64_t kMinDownlinkStatsIntervalMs = 100;
				if (roomId.empty() || peerId.empty() || sessionId == kInvalidSessionId) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "downlinkClientStats requires joined session"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				{
					bool validSession = HasMappedSession(wsMap, ws, roomId, peerId, sessionId);
					if (!validSession) {
						server.staleRequestDrops_.fetch_add(1, std::memory_order_relaxed);
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "stale session"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
						return;
					}
				}
				auto dlParse = qos::QosValidator::ParseDownlinkSnapshot(data);
				if (!dlParse.ok) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "invalid downlinkClientStats: " + dlParse.error}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
				auto* wt = server.getWorkerThread(roomId, false);
				if (!wt || !wt->roomService()) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", "room not assigned"}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}

				const auto seq = dlParse.value.seq;
				const auto mapKey = WsMap::key(roomId, peerId);
				{
					const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
						std::chrono::steady_clock::now().time_since_epoch()).count();
					auto& state = (*downlinkStatsRateLimit)[mapKey];
					const bool advancingSeq = IsAdvancingDownlinkSeq(state, seq);
					if (advancingSeq &&
						state.pending &&
						nowMs - state.pendingSinceMs <= kMinDownlinkStatsIntervalMs) {
						server.rejectedClientStats_.fetch_add(1, std::memory_order_relaxed);
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "downlinkClientStats rate limited"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
						return;
					}
					if (advancingSeq) {
						state.pending = true;
						state.pendingSinceMs = nowMs;
						state.pendingSeq = seq;
					}
				}
				auto alive = sd->alive;
				wt->post([wt, ws, alive, loop, downlinkStatsRateLimit, id, roomId, peerId, mapKey, seq,
					snapshot = std::move(dlParse.value)]() mutable {
					RoomService::Result result;
					try {
						auto* rs = wt->roomService();
						if (!rs) {
							result = {false, json::object(), "", "worker not ready"};
						} else {
							result = rs->setDownlinkClientStats(roomId, peerId, std::move(snapshot));
						}
					} catch (const std::exception& e) {
						spdlog::error("[{} {}] downlinkClientStats error: {}", roomId, peerId, e.what());
						result = {false, json::object(), "", e.what()};
					} catch (...) {
						spdlog::error("[{} {}] downlinkClientStats error: unknown error", roomId, peerId);
						result = {false, json::object(), "", "unknown downlinkClientStats error"};
					}

					const bool stored = result.ok && result.data.value("stored", false);
					std::string respStr = BuildWorkerCompletedResponse(id, result).dump();
					loop->defer([ws, alive, downlinkStatsRateLimit, mapKey, seq, stored,
						respStr = std::move(respStr)] {
						auto it = downlinkStatsRateLimit->find(mapKey);
						if (it != downlinkStatsRateLimit->end()) {
							if (stored) {
								MarkAcceptedDownlinkSeq(it->second, seq);
							} else {
								ClearPendingDownlinkSeqIfMatches(it->second, seq);
							}
						}
						if (!alive->load(std::memory_order_relaxed)) return;
						ws->send(respStr, uWS::OpCode::TEXT);
					});
				});
				return;
			}

			signaldispatch::JoinRequestContext joinRequest;
			if (method == "join") {
				std::string joinError;
				if (!signaldispatch::BuildJoinRequestContext(
					data,
					std::string(ws->getRemoteAddressAsText()),
					joinRequest,
					joinError)) {
					json resp = {{"response", true}, {"id", id}, {"ok", false},
						{"error", joinError}};
					ws->send(resp.dump(), uWS::OpCode::TEXT);
					return;
				}
			}

			auto alive = sd->alive;
			uint64_t newSessionId = kInvalidSessionId;
			if (method == "join") newSessionId = g_nextSessionId++;

			std::string targetRoomId = (method == "join") ? joinRequest.roomId : roomId;
			WorkerThread* wt = nullptr;
			if (method == "join") {
				bool destroyed = (server.destroyedRooms_.find(targetRoomId) != server.destroyedRooms_.end());
				wt = destroyed ? nullptr : server.getWorkerThread(targetRoomId, false);
				if (!wt) {
					wt = server.pickLeastLoadedWorkerThread();
					if (wt) server.assignRoom(targetRoomId, wt);
				}
			} else {
				wt = server.getWorkerThread(targetRoomId, false);
			}
			if (!wt) {
				json resp = {{"response", true}, {"id", id}, {"ok", false},
					{"error", method == "join" ? "no available worker thread" : "room not assigned"}};
				ws->send(resp.dump(), uWS::OpCode::TEXT);
				return;
			}

			wt->post([&server, wt, wsMap, ws, alive, loop, method, id, data,
				roomId, peerId, joinRequest, targetRoomId, sessionId, newSessionId]
			{
				auto* rs = wt->roomService();
				if (!rs) {
					loop->defer([ws, alive, id] {
						if (!alive->load(std::memory_order_relaxed)) return;
						json resp = {{"response", true}, {"id", id}, {"ok", false},
							{"error", "worker not ready"}};
						ws->send(resp.dump(), uWS::OpCode::TEXT);
					});
					return;
				}

				if (method != "join" && sessionId != kInvalidSessionId) {
					auto room = rs->getRoom(roomId);
					if (room) {
						auto peer = room->getPeer(peerId);
						if (peer && peer->sessionId != 0 && peer->sessionId != sessionId) {
							spdlog::warn("[{} {}] dropping stale {} (session:{} current:{})",
								roomId, peerId, method, sessionId, peer->sessionId);
							loop->defer([ws, alive, id] {
								if (!alive->load()) return;
								json resp = {{"response", true}, {"id", id}, {"ok", false},
									{"error", "remote login"}};
								ws->send(resp.dump(), uWS::OpCode::TEXT);
							});
							return;
						}
					}
				}

				RoomService::Result result;
				try {
					result = signaldispatch::DispatchRoomServiceRequest(
						*rs,
						method,
						roomId,
						peerId,
						data,
						method == "join" ? &joinRequest : nullptr,
						newSessionId);
				} catch (const std::exception& e) {
					spdlog::error("[{} {}] {} error: {}", targetRoomId, peerId, method, e.what());
					result = {false, {}, "", e.what()};
				}

				wt->updateRoomCount();

				json resp;
				if (!result.redirect.empty()) {
					resp = {{"response", true}, {"id", id}, {"ok", false},
						{"redirect", result.redirect}};
				} else if (result.ok) {
					resp = {{"response", true}, {"id", id}, {"ok", true}, {"data", result.data}};
				} else {
					resp = {{"response", true}, {"id", id}, {"ok", false}, {"error", result.error}};
				}
				std::string respStr = resp.dump();

				bool joinOk = (method == "join" && result.ok && result.redirect.empty());
				bool joinFailed = (method == "join" && !joinOk);
				std::string jRoomId = joinRequest.roomId, jPeerId = joinRequest.peerId;
				json joinQosPolicy = json::object();
				if (joinOk && wt->roomService()) {
					joinQosPolicy = wt->roomService()->getDefaultQosPolicy();
				}

				loop->defer([&server, wt, wsMap, ws, alive, respStr = std::move(respStr),
					joinOk, joinFailed, newSessionId,
					jRoomId = std::move(jRoomId), jPeerId = std::move(jPeerId),
					joinQosPolicy = std::move(joinQosPolicy)]
				{
					if (!alive->load()) {
						return;
					}
					if (joinFailed) {
						if (!wt->roomService() || !wt->roomService()->hasRoom(jRoomId))
							server.unassignRoom(jRoomId);
					}
					if (joinOk) {
						auto oldWs = RegisterJoinedSocket(wsMap, ws, jRoomId, jPeerId, newSessionId);
						server.assignRoom(jRoomId, wt);
						spdlog::info("[{} {}] joined (session:{})", jRoomId, jPeerId, newSessionId);
						if (oldWs) {
							spdlog::info("[{} {}] kicking old connection (new session:{})", jRoomId, jPeerId, newSessionId);
							oldWs->end(4000, "replaced");
						}
					}
					ws->send(respStr, uWS::OpCode::TEXT);
					if (joinOk && !joinQosPolicy.empty()) {
						json msg = {
							{"notification", true},
							{"method", "qosPolicy"},
							{"data", joinQosPolicy}
						};
						ws->send(msg.dump(), uWS::OpCode::TEXT);
					}
				});
			});
			} catch (const json::exception& e) {
				spdlog::warn("dropping malformed websocket message: {}", e.what());
			} catch (const std::exception& e) {
				spdlog::error("websocket message handler failed: {}", e.what());
			} catch (...) {
				spdlog::error("websocket message handler failed: unknown error");
			}
		},

		.close = [&server, wsMap, downlinkStatsRateLimit](auto* ws, int, std::string_view) {
			auto* sd = ws->getUserData();
			sd->alive->store(false);
			if (!sd->peerId.empty()) {
				spdlog::info("[{} {}] disconnected (session:{})", sd->roomId, sd->peerId, sd->sessionId);
				std::string roomId = sd->roomId;
				std::string peerId = sd->peerId;
				std::string mapKey = WsMap::key(roomId, peerId);
				bool erased = UnregisterSocketSession(wsMap, ws, roomId, peerId);
				if (erased) downlinkStatsRateLimit->erase(mapKey);
				if (erased && !roomId.empty()) {
					auto* wt = server.getWorkerThread(roomId, false);
					if (wt) {
						wt->post([wt, roomId, peerId] {
							try {
								if (wt->roomService())
									wt->roomService()->leave(roomId, peerId);
							} catch (const std::exception& e) {
								spdlog::error("[{} {}] leave failed: {}", roomId, peerId, e.what());
							} catch (...) {
								spdlog::error("[{} {}] leave failed: unknown error", roomId, peerId);
							}
							wt->updateRoomCount();
						});
					}
				}
			}
		}
	});
}

} // namespace mediasoup
