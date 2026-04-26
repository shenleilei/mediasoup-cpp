#include "RoomRegistry.h"

#include "Logger.h"
#include "RoomRegistryReplyUtils.h"

#include <fcntl.h>
#include <hiredis/hiredis.h>
#include <poll.h>

namespace mediasoup {

void RoomRegistry::startSubscriber()
{
	subStop_ = false;
	subThread_ = std::thread([this] { subscriberLoop(); });
}

void RoomRegistry::subscriberLoop()
{
	while (!subStop_) {
		struct timeval timeout = {kRedisConnectTimeoutSec, 0};
		redisContext* subscriber = redisConnectWithTimeout(redisHost_.c_str(), redisPort_, timeout);
		if (!subscriber || subscriber->err) {
			if (subscriber) redisFree(subscriber);
			MS_WARN(logger_, "Subscriber connect failed, retrying in 2s");
			for (int i = 0; i < 20 && !subStop_; ++i)
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		auto* reply = (redisReply*)redisCommand(
			subscriber, "SUBSCRIBE %s %s", kChannelNodes, kChannelRooms);
		if (reply) freeReplyObject(reply);
		else {
			redisFree(subscriber);
			continue;
		}

		redisSetTimeout(subscriber, (struct timeval){0, 0});
		int flags = fcntl(subscriber->fd, F_GETFL, 0);
		fcntl(subscriber->fd, F_SETFL, flags | O_NONBLOCK);

		MS_DEBUG(logger_, "Subscriber connected, adding random jitter before syncAll");
		
		// Add 0-2000ms random jitter to prevent thundering herd / sync storm
		// when all nodes reconnect to Redis simultaneously.
		uint32_t r = 0;
		if (FILE* f = fopen("/dev/urandom", "r")) {
			if (fread(&r, sizeof(r), 1, f) != 1) r = 0;
			fclose(f);
		}
		int jitterMs = r % 2000;
		std::this_thread::sleep_for(std::chrono::milliseconds(jitterMs));

		syncAll();

		while (!subStop_) {
			struct pollfd pfd = {subscriber->fd, POLLIN, 0};
			int ret = poll(&pfd, 1, 2000);
			if (ret == 0) continue;
			if (ret < 0) break;

			redisReply* msg = nullptr;
			int rc = redisGetReply(subscriber, (void**)&msg);
			if (rc != REDIS_OK || !msg) {
				if (subscriber->err == REDIS_ERR_IO || subscriber->err == REDIS_ERR_EOF)
					break;
				if (!msg) continue;
			}

			if (msg->type == REDIS_REPLY_ARRAY && msg->elements >= 3) {
				std::string type;
				if (!redisreply::GetTextElement(msg, 0, type)) {
					MS_WARN(logger_, "Subscriber received malformed pubsub message header");
				} else if (type == "message") {
					std::string channel;
					std::string data;
					if (redisreply::GetTextElement(msg, 1, channel) &&
						redisreply::GetTextElement(msg, 2, data)) {
						handlePubSubMessage(channel, data);
					} else {
						MS_WARN(logger_, "Subscriber received malformed pubsub message payload");
					}
				}
			}
			freeReplyObject(msg);
		}

		redisFree(subscriber);
		if (!subStop_) MS_WARN(logger_, "Subscriber disconnected, reconnecting...");
	}
}

void RoomRegistry::handlePubSubMessage(const std::string& channel, const std::string& data)
{
	auto eq = data.find('=');
	if (eq == std::string::npos) return;
	std::string key = data.substr(0, eq);
	std::string val = data.substr(eq + 1);

	if (channel == kChannelNodes) {
		if (val.empty()) {
			cache_.eraseNodeAndOwnedRooms(key);
		} else {
			cache_.upsertNode(key, parseNodeValue(val));
		}
	} else if (channel == kChannelRooms) {
		if (val.empty()) cache_.eraseRoom(key);
		else cache_.upsertRoom(key, val);
	}
}

} // namespace mediasoup
