#include "RoomRegistry.h"

#include "Logger.h"

#include <hiredis/hiredis.h>

#include <sstream>
#include <stdarg.h>

namespace mediasoup {

RoomRegistry::CommandConnection::~CommandConnection()
{
	if (ctx) {
		redisFree(ctx);
		ctx = nullptr;
	}
}

bool RoomRegistry::CommandConnection::reconnect(
	const std::string& redisHost,
	int redisPort,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (ctx) {
		redisFree(ctx);
		ctx = nullptr;
	}
	struct timeval timeout = {kRedisConnectTimeoutSec, 0};
	ctx = redisConnectWithTimeout(redisHost.c_str(), redisPort, timeout);
	if (!ctx || ctx->err) {
		std::string err = ctx ? ctx->errstr : "alloc failed";
		MS_ERROR(logger, "Redis connect failed: {}", err);
		if (ctx) {
			redisFree(ctx);
			ctx = nullptr;
		}
		return false;
	}
	return true;
}

bool RoomRegistry::CommandConnection::ensureConnected(
	const std::string& redisHost,
	int redisPort,
	const std::shared_ptr<spdlog::logger>& logger)
{
	if (connected()) return true;
	return reconnect(redisHost, redisPort, logger);
}

void RoomRegistry::CommandConnection::disconnect(
	const std::shared_ptr<spdlog::logger>& logger,
	bool logAsWarning)
{
	if (logAsWarning)
		MS_WARN(logger, "Redis connection lost, will reconnect on next operation");
	if (ctx) {
		redisFree(ctx);
		ctx = nullptr;
	}
}

bool RoomRegistry::CommandConnection::connected() const
{
	return ctx && !ctx->err;
}

redisReply* RoomRegistry::CommandConnection::command(const char* format, ...)
{
	if (!ctx) return nullptr;
	va_list args;
	va_start(args, format);
	auto* reply = (redisReply*)redisvCommand(ctx, format, args);
	va_end(args);
	return reply;
}

int RoomRegistry::CommandConnection::appendCommand(const char* format, ...)
{
	if (!ctx) return REDIS_ERR;
	va_list args;
	va_start(args, format);
	int rc = redisvAppendCommand(ctx, format, args);
	va_end(args);
	return rc;
}

int RoomRegistry::CommandConnection::getReply(void** reply)
{
	if (!ctx) return REDIS_ERR;
	return redisGetReply(ctx, reply);
}

redisReply* RoomRegistry::CommandConnection::commandArgv(
	int argc,
	const char** argv,
	const size_t* argvLen)
{
	if (!ctx) return nullptr;
	return (redisReply*)redisCommandArgv(ctx, argc, argv, argvLen);
}

bool RoomRegistry::reconnect()
{
	const bool ready = command_.reconnect(redisHost_, redisPort_, logger_);
	commandReady_.store(ready, std::memory_order_relaxed);
	return ready;
}

bool RoomRegistry::ensureConnected()
{
	const bool ready = command_.ensureConnected(redisHost_, redisPort_, logger_);
	commandReady_.store(ready, std::memory_order_relaxed);
	return ready;
}

void RoomRegistry::handleDisconnect()
{
	command_.disconnect(logger_);
	commandReady_.store(false, std::memory_order_relaxed);
}

std::string RoomRegistry::buildNodeValue(size_t rooms, size_t maxRooms)
{
	return nodeAddress_ + "|" + std::to_string(rooms) + "|" + std::to_string(maxRooms)
		+ "|" + std::to_string(nodeLat_) + "|" + std::to_string(nodeLng_)
		+ "|" + nodeIsp_ + "|" + nodeCountry_;
}

RoomRegistry::NodeInfo RoomRegistry::parseNodeValue(const std::string& val)
{
	NodeInfo info;
	std::vector<std::string> parts;
	std::istringstream stream(val);
	std::string token;
	while (std::getline(stream, token, '|'))
		parts.push_back(token);
	if (parts.empty()) return info;

	info.address = parts[0];
	if (parts.size() > 1) try { info.rooms = std::stoul(parts[1]); } catch (...) {}
	if (parts.size() > 2) try { info.maxRooms = std::stoul(parts[2]); } catch (...) {}
	if (parts.size() > 3) try { info.lat = std::stod(parts[3]); } catch (...) {}
	if (parts.size() > 4) try { info.lng = std::stod(parts[4]); } catch (...) {}
	if (parts.size() > 5) info.isp = parts[5];
	if (parts.size() > 6) info.country = parts[6];
	return info;
}

void RoomRegistry::registerNode()
{
	if (!command_.connected()) return;
	std::string key = std::string(kKeyPrefixNode) + nodeId_;
	auto* reply = command_.command("EXPIRE %s %d", key.c_str(), nodeTTL_);
	if (reply && reply->type == REDIS_REPLY_INTEGER && reply->integer == 0) {
		freeReplyObject(reply);
		std::string val = buildNodeValue(0, 0);
		reply = command_.command("SET %s %s EX %d", key.c_str(), val.c_str(), nodeTTL_);
		if (reply) {
			cache_.upsertNode(nodeId_, parseNodeValue(val));
			publishNodeUpdate(nodeId_, val);
		}
	}
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

void RoomRegistry::publishNodeUpdate(const std::string& nodeId, const std::string& nodeValue)
{
	if (!command_.connected()) return;
	std::string msg = nodeId + "=" + nodeValue;
	auto* reply = command_.command("PUBLISH %s %s", kChannelNodes, msg.c_str());
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

void RoomRegistry::publishRoomUpdate(const std::string& roomId, const std::string& ownerAddr)
{
	if (!command_.connected()) return;
	std::string msg = roomId + "=" + ownerAddr;
	auto* reply = command_.command("PUBLISH %s %s", kChannelRooms, msg.c_str());
	if (reply) freeReplyObject(reply);
	else handleDisconnect();
}

} // namespace mediasoup
