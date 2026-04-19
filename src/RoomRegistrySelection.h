#pragma once

#include <cmath>
#include <cstddef>
#include <string>

namespace mediasoup::roomregistry {

struct LoadCandidate {
	std::string address;
	size_t rooms = 0;
	double score = 0;
};

inline bool CompareGeoCandidates(const LoadCandidate& a, const LoadCandidate& b)
{
	if (std::abs(a.score - b.score) > 100.0) {
		return a.score < b.score;
	}
	return a.rooms < b.rooms;
}

inline bool CompareNoGeoCandidates(
	const LoadCandidate& a,
	const LoadCandidate& b,
	const std::string& selfAddress)
{
	if (a.rooms != b.rooms) {
		return a.rooms < b.rooms;
	}

	const bool aIsSelf = (a.address == selfAddress);
	const bool bIsSelf = (b.address == selfAddress);
	if (aIsSelf != bIsSelf) {
		return aIsSelf;
	}

	return a.address < b.address;
}

} // namespace mediasoup::roomregistry
