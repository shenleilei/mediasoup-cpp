#pragma once

#include "../TransportCcHelpers.h"

#include <chrono>
#include <cstdint>
#include <utility>

namespace mediasoup::plainclient::sendsidebwe {

class TwccFeedbackTracker {
public:
	static constexpr uint32_t kReferenceTimeMask = (1u << 24) - 1u;
	static constexpr int64_t kReferenceTimeResolutionUs = 64000;

	std::pair<int64_t, bool> ProcessReport(
		const mediasoup::plainclient::TransportCcFeedback& report,
		int64_t atUs)
	{
		numReports_++;
		if (lastFeedbackTimeUs_ == 0) {
			lastFeedbackTimeUs_ = atUs;
			highestReferenceTime_ = report.referenceTime;
			highestFeedbackCount_ = report.feedbackPacketCount;
			return { (cycles_ + static_cast<int64_t>(report.referenceTime)) * kReferenceTimeResolutionUs, false };
		}

		bool isOutOfOrder = false;
		if (static_cast<uint8_t>(report.feedbackPacketCount - highestFeedbackCount_) > (1u << 7)) {
			numReportsOutOfOrder_++;
			isOutOfOrder = true;
		}

		int64_t referenceTime = 0;
		if (((report.referenceTime - highestReferenceTime_) & kReferenceTimeMask) < (1u << 23)) {
			if (report.referenceTime < highestReferenceTime_) {
				cycles_ += (1ll << 24);
			}
			highestReferenceTime_ = report.referenceTime;
			referenceTime = cycles_ + static_cast<int64_t>(report.referenceTime);
		} else {
			int64_t cycles = cycles_;
			if (report.referenceTime > highestReferenceTime_ && cycles >= (1ll << 24)) {
				cycles -= (1ll << 24);
			}
			referenceTime = cycles + static_cast<int64_t>(report.referenceTime);
		}

		if (!isOutOfOrder) {
			const int64_t sinceLastUs = atUs - lastFeedbackTimeUs_;
			if (estimatedFeedbackIntervalUs_ == 0) {
				estimatedFeedbackIntervalUs_ = sinceLastUs;
			} else {
				if (sinceLastUs > estimatedFeedbackIntervalUs_ / 3 &&
					sinceLastUs < estimatedFeedbackIntervalUs_ * 3) {
					estimatedFeedbackIntervalUs_ =
						static_cast<int64_t>(0.9 * static_cast<double>(estimatedFeedbackIntervalUs_) +
							0.1 * static_cast<double>(sinceLastUs));
				}
			}
			lastFeedbackTimeUs_ = atUs;
			highestFeedbackCount_ = report.feedbackPacketCount;
		}

		return { referenceTime * kReferenceTimeResolutionUs, isOutOfOrder };
	}

	int64_t EstimatedFeedbackIntervalUs() const { return estimatedFeedbackIntervalUs_; }
	int NumReports() const { return numReports_; }
	int NumReportsOutOfOrder() const { return numReportsOutOfOrder_; }

private:
	int64_t lastFeedbackTimeUs_{ 0 };
	int64_t estimatedFeedbackIntervalUs_{ 0 };
	int numReports_{ 0 };
	int numReportsOutOfOrder_{ 0 };
	uint8_t highestFeedbackCount_{ 0 };
	int64_t cycles_{ 0 };
	uint32_t highestReferenceTime_{ 0 };
};

} // namespace mediasoup::plainclient::sendsidebwe
