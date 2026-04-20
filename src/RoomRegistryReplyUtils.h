#pragma once

#include <hiredis/hiredis.h>

#include <string>

namespace mediasoup::redisreply {

inline bool IsTextReply(const redisReply* reply)
{
	return reply &&
		(reply->type == REDIS_REPLY_STRING || reply->type == REDIS_REPLY_STATUS) &&
		reply->str != nullptr;
}

inline bool GetTextElement(
	const redisReply* reply,
	size_t index,
	std::string& out)
{
	if (!reply || reply->type != REDIS_REPLY_ARRAY || index >= reply->elements) {
		return false;
	}

	const auto* element = reply->element[index];
	if (!IsTextReply(element)) {
		return false;
	}

	out.assign(element->str, element->len);
	return true;
}

inline const redisReply* GetArrayElement(
	const redisReply* reply,
	size_t index,
	int expectedType)
{
	if (!reply || reply->type != REDIS_REPLY_ARRAY || index >= reply->elements) {
		return nullptr;
	}

	const auto* element = reply->element[index];
	if (!element || element->type != expectedType) {
		return nullptr;
	}

	return element;
}

} // namespace mediasoup::redisreply
