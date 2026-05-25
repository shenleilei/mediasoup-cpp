#include "push/PushSdkTransportThread.h"

#include <algorithm>
#include <chrono>
#include <numeric>
#include <utility>

#include "common/RtpRtcpClassifier.h"
#include "common/RuntimeLogHelpers.h"
#include "common/SdkRuntimeConfig.h"

namespace webrtc_qos_plain {

struct PushSdkTransportThread::TrackQueueState {
	explicit TrackQueueState(webrtc_qos::TransportIds trackIds, size_t capacity)
		: ids(trackIds),
		  queue(capacity)
	{
	}

	webrtc_qos::TransportIds ids;
	BoundedQueue<EncodedAccessUnit> queue;
	std::atomic<uint64_t> pushedAccessUnits{0};
	std::atomic<uint64_t> pushFailures{0};
	LatestValue<webrtc_qos::EncoderAdaptation> adaptation;
	LatestValue<webrtc_qos::QosSnapshot> snapshot;
};

PushSdkTransportThread::PushSdkTransportThread(
	PushSdkTransportThreadConfig config,
	std::shared_ptr<spdlog::logger> logger)
	: config_(std::move(config)),
	  logger_(std::move(logger))
{
	if (config_.session.video_tracks.empty()) {
		trackQueues_.push_back(std::make_unique<TrackQueueState>(
			config_.session.ids,
			config_.encodedQueueCapacity));
	} else {
		for (const auto& track : config_.session.video_tracks) {
			if (!track.enabled) continue;
			trackQueues_.push_back(std::make_unique<TrackQueueState>(
				track.ids,
				config_.encodedQueueCapacity));
		}
	}
	if (trackQueues_.empty()) {
		trackQueues_.push_back(std::make_unique<TrackQueueState>(
			config_.session.ids,
			config_.encodedQueueCapacity));
	}
}

PushSdkTransportThread::~PushSdkTransportThread()
{
	Stop();
}

int PushSdkTransportThread::Start(std::string* error)
{
	if (running_.exchange(true)) return 0;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	pushedAccessUnits_.store(0, std::memory_order_relaxed);
	pushFailures_.store(0, std::memory_order_relaxed);
	rtcpPacketsIn_.store(0, std::memory_order_relaxed);
	rtcpBytesIn_.store(0, std::memory_order_relaxed);
	rtcpFailures_.store(0, std::memory_order_relaxed);
	unexpectedRtpPacketsIn_.store(0, std::memory_order_relaxed);
	malformedPacketsIn_.store(0, std::memory_order_relaxed);
	nextDrainIndex_ = 0;
	StoreEndpoints({}, {});
	StoreStopReason("");
	StoreFatalError("");
	{
		std::lock_guard<std::mutex> lock(startupMutex_);
		startupDone_ = false;
		startupOk_ = false;
		startupCode_ = 0;
		startupError_.clear();
	}
	thread_ = std::thread([this]() { Run(); });
	std::unique_lock<std::mutex> lock(startupMutex_);
	startupCv_.wait(lock, [&] { return startupDone_; });
	if (startupOk_) return 0;
	running_.store(false, std::memory_order_relaxed);
	CloseTrackQueues();
	lock.unlock();
	if (thread_.joinable()) thread_.join();
	if (error) *error = startupError_;
	return startupCode_ == 0 ? 3 : startupCode_;
}

void PushSdkTransportThread::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	CloseTrackQueues();
	if (thread_.joinable()) thread_.join();
}

webrtc_qos::Status PushSdkTransportThread::Enqueue(EncodedAccessUnit item)
{
	if (item.bytes.empty()) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"empty encoded access unit");
	}
	auto* track = FindTrackQueue(item.ids.track_id);
	if (!track) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kInvalidArgument,
			"unknown push SDK encoded queue track_id=" + std::to_string(item.ids.track_id));
	}
	if (!track->queue.PushDropOldest(std::move(item))) {
		return webrtc_qos::Status::Error(
			webrtc_qos::StatusCode::kQueueFull,
			"push SDK encoded queue is closed");
	}
	return webrtc_qos::Status::Ok();
}

webrtc_qos::EncoderAdaptation PushSdkTransportThread::encoderAdaptation() const
{
	webrtc_qos::EncoderAdaptation adaptation;
	(void)adaptation_.Load(&adaptation);
	return adaptation;
}

PushSdkTransportThreadMetrics PushSdkTransportThread::metrics() const
{
	PushSdkTransportThreadMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.lastHeartbeatUs = lastHeartbeatUs_.load(std::memory_order_relaxed);
	out.loopGapMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	out.loopIterations = loopIterations_.load(std::memory_order_relaxed);
	out.pushedAccessUnits = pushedAccessUnits_.load(std::memory_order_relaxed);
	out.pushFailures = pushFailures_.load(std::memory_order_relaxed);
	out.rtcpPacketsIn = rtcpPacketsIn_.load(std::memory_order_relaxed);
	out.rtcpBytesIn = rtcpBytesIn_.load(std::memory_order_relaxed);
	out.rtcpFailures = rtcpFailures_.load(std::memory_order_relaxed);
	out.unexpectedRtpPacketsIn = unexpectedRtpPacketsIn_.load(std::memory_order_relaxed);
	out.malformedPacketsIn = malformedPacketsIn_.load(std::memory_order_relaxed);
	out.tracks.reserve(trackQueues_.size());
	for (const auto& track : trackQueues_) {
		if (!track) continue;
		PushSdkTrackQueueMetrics trackMetrics;
		trackMetrics.trackId = track->ids.track_id;
		trackMetrics.senderSsrc = track->ids.sender_ssrc;
		trackMetrics.queueDepth = track->queue.depth();
		trackMetrics.queueMaxDepth = track->queue.maxDepth();
		trackMetrics.enqueuedAccessUnits = track->queue.pushed();
		trackMetrics.droppedAccessUnits = track->queue.dropped();
		trackMetrics.pushedAccessUnits = track->pushedAccessUnits.load(std::memory_order_relaxed);
		trackMetrics.pushFailures = track->pushFailures.load(std::memory_order_relaxed);
		trackMetrics.adaptationAvailable = track->adaptation.Load(&trackMetrics.adaptation);
		trackMetrics.snapshotAvailable = track->snapshot.Load(&trackMetrics.snapshot);
		out.enqueuedAccessUnits += trackMetrics.enqueuedAccessUnits;
		out.droppedAccessUnits += trackMetrics.droppedAccessUnits;
		out.queueDepth += trackMetrics.queueDepth;
		out.queueMaxDepth = std::max(out.queueMaxDepth, trackMetrics.queueMaxDepth);
		out.tracks.push_back(trackMetrics);
	}
	(void)adaptation_.Load(&out.adaptation);
	(void)snapshot_.Load(&out.snapshot);
	{
		std::lock_guard<std::mutex> lock(metadataMutex_);
		out.localEndpoint = localEndpoint_;
		out.remoteEndpoint = remoteEndpoint_;
		out.stopReason = stopReason_;
		out.fatalError = fatalError_;
	}
	return out;
}

bool PushSdkTransportThread::hasFatalError() const
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	return !fatalError_.empty();
}

void PushSdkTransportThread::Run()
{
	PlainUdpTransport udp;
	std::string error;
	if (!udp.Connect(config_.mediaRemoteIp, config_.mediaRemotePort, &error)) {
		if (logger_) logger_->error("push_sdk_udp_connect_failed remoteIp={} remotePort={} error={}",
			config_.mediaRemoteIp, config_.mediaRemotePort, error);
		StoreFatalError(error);
		StoreStopReason("udp_connect_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 2, error);
		return;
	}
	StoreEndpoints(udp.localEndpoint(), udp.remoteEndpoint());

	webrtc_qos::VideoPushClientConfig sdkConfig;
	sdkConfig.session = config_.session;
	const bool sdkRuntimeFilesEnabled = ConfigureSdkRuntimeFiles(sdkConfig, "push", config_.logDir);
	if (logger_) logger_->info("sdk_runtime_files role=push enabled={}", sdkRuntimeFilesEnabled);
	sdkConfig.transport_output = [&](const webrtc_qos::TransportPacketView& packet) {
		std::string sendError;
		const bool ok = udp.Send(packet.bytes, packet.size, &sendError);
		if (!ok) {
			if (logger_) logger_->error("sdk_transport_output_failed kind={} bytes={} error={}",
				packet.metadata.kind == webrtc_qos::TransportPacketKind::kRtcp ? "rtcp" : "rtp",
				packet.size,
				sendError);
			return webrtc_qos::Status::Error(
				webrtc_qos::StatusCode::kInternalError,
				sendError);
		}
		return webrtc_qos::Status::Ok();
	};

	auto push = webrtc_qos::CreateVideoPushClient(sdkConfig);
	if (!push) {
		StoreFatalError("create_video_push_client_failed");
		StoreStopReason("create_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 3, "create_video_push_client_failed");
		return;
	}
	auto status = push->Start();
	if (!status) {
		const std::string statusText = StatusToString(status);
		StoreFatalError(statusText);
		StoreStopReason("start_failed");
		stopped_.store(true, std::memory_order_relaxed);
		if (logger_) logger_->error("push_start_failed status={}", statusText);
		CompleteStartup(false, 3, statusText);
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	PublishSdkSnapshots(*push, MonotonicNowUs());
	if (logger_) logger_->info(
		"push_sdk_transport_thread_started udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={} trackQueues={}",
		udp.localEndpoint().ip,
		udp.localEndpoint().port,
		udp.remoteEndpoint().ip,
		udp.remoteEndpoint().port,
		trackQueues_.size());
	CompleteStartup(true, 0, "");

	int64_t lastLoopUs = MonotonicNowUs();
	while (running_.load(std::memory_order_relaxed)) {
		const int64_t nowUs = MonotonicNowUs();
		RecordLoopTick(nowUs, &lastLoopUs);
		DrainUdpFeedback(udp, *push, nowUs, 64);

		size_t drained = 0;
		if (!DrainEncodedQueues(*push, 64, &drained)) break;

		status = push->Process(nowUs);
		if (!status) {
			const std::string statusText = StatusToString(status);
			StoreFatalError(statusText);
			if (logger_) logger_->error("push_process_failed status={}", statusText);
			break;
		}
		PublishSdkSnapshots(*push, nowUs);
		std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, config_.processTickMs)));
	}

	const int64_t drainUntilUs = MonotonicNowUs() + 1000000;
	while (MonotonicNowUs() < drainUntilUs) {
		size_t drained = 0;
		if (!DrainEncodedQueues(*push, 64, &drained)) break;
		if (drained == 0) break;
		status = push->Process(MonotonicNowUs());
		if (!status) break;
	}

	status = push->Stop();
	if (!status && logger_) logger_->warn("push_stop_failed status={}", StatusToString(status));
	StoreStopReason(hasFatalError() ? "fatal_error" : "stopped");
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto finalMetrics = metrics();
		logger_->info(
			"push_sdk_transport_thread_stopped stopReason={} tracks={} queuedAu={} pushedAu={} droppedAu={} pushFailures={} rtcpFeedbackPacketsIn={} rtcpFeedbackBytesIn={} rtcpFeedbackFailures={} loopIterations={} loopGapMaxUs={}",
			finalMetrics.stopReason,
			finalMetrics.tracks.size(),
			finalMetrics.enqueuedAccessUnits,
			finalMetrics.pushedAccessUnits,
			finalMetrics.droppedAccessUnits,
			finalMetrics.pushFailures,
			finalMetrics.rtcpPacketsIn,
			finalMetrics.rtcpBytesIn,
			finalMetrics.rtcpFailures,
			finalMetrics.loopIterations,
			finalMetrics.loopGapMaxUs);
	}
}

bool PushSdkTransportThread::DrainEncodedQueues(
	webrtc_qos::VideoPushClient& push,
	size_t maxItems,
	size_t* drained)
{
	if (drained) *drained = 0;
	if (trackQueues_.empty() || maxItems == 0) return true;
	size_t localDrained = 0;
	const size_t trackCount = trackQueues_.size();
	const size_t startIndex = nextDrainIndex_ % trackCount;
	bool madeProgress = true;
	while (localDrained < maxItems && madeProgress) {
		madeProgress = false;
		for (size_t offset = 0; offset < trackCount && localDrained < maxItems; ++offset) {
			const size_t index = (startIndex + offset) % trackCount;
			auto* track = trackQueues_[index].get();
			if (!track) continue;
			EncodedAccessUnit item;
			if (!track->queue.TryPop(&item)) continue;
			madeProgress = true;
			if (!PushAccessUnit(push, *track, std::move(item))) {
				if (drained) *drained = localDrained;
				return false;
			}
			++localDrained;
			nextDrainIndex_ = (index + 1) % trackCount;
		}
	}
	if (drained) *drained = localDrained;
	return true;
}

bool PushSdkTransportThread::PushAccessUnit(
	webrtc_qos::VideoPushClient& push,
	TrackQueueState& track,
	EncodedAccessUnit item)
{
	if (item.ids.track_id == 0) item.ids.track_id = track.ids.track_id;
	if (item.ids.sender_ssrc == 0) item.ids.sender_ssrc = track.ids.sender_ssrc;
	auto view = ToAnnexBAccessUnitView(item);
	auto status = push.PushAnnexBAccessUnit(view);
	if (!status) {
		track.pushFailures.fetch_add(1, std::memory_order_relaxed);
		pushFailures_.fetch_add(1, std::memory_order_relaxed);
		const std::string statusText = StatusToString(status);
		StoreFatalError(statusText);
		if (logger_) logger_->error("push_au_failed trackId={} senderSsrc={} status={}",
			track.ids.track_id,
			track.ids.sender_ssrc,
			statusText);
		running_.store(false, std::memory_order_relaxed);
		return false;
	}
	track.pushedAccessUnits.fetch_add(1, std::memory_order_relaxed);
	pushedAccessUnits_.fetch_add(1, std::memory_order_relaxed);
	return true;
}

void PushSdkTransportThread::PublishSdkSnapshots(
	const webrtc_qos::VideoPushClient& push,
	int64_t nowUs)
{
	adaptation_.Store(push.GetEncoderAdaptation(nowUs));
	snapshot_.Store(push.GetQosSnapshot(nowUs));
	for (auto& track : trackQueues_) {
		if (!track) continue;
		webrtc_qos::EncoderAdaptation adaptation;
		if (push.GetTrackEncoderAdaptation(track->ids.track_id, nowUs, &adaptation)) {
			track->adaptation.Store(adaptation);
		}
		webrtc_qos::QosSnapshot snapshot;
		if (push.GetTrackQosSnapshot(track->ids.track_id, nowUs, &snapshot)) {
			track->snapshot.Store(snapshot);
		}
	}
}

PushSdkTransportThread::TrackQueueState* PushSdkTransportThread::FindTrackQueue(uint32_t trackId)
{
	if (trackId == 0 && trackQueues_.size() == 1) return trackQueues_.front().get();
	for (auto& track : trackQueues_) {
		if (track && track->ids.track_id == trackId) return track.get();
	}
	return nullptr;
}

const PushSdkTransportThread::TrackQueueState* PushSdkTransportThread::FindTrackQueue(uint32_t trackId) const
{
	if (trackId == 0 && trackQueues_.size() == 1) return trackQueues_.front().get();
	for (const auto& track : trackQueues_) {
		if (track && track->ids.track_id == trackId) return track.get();
	}
	return nullptr;
}

void PushSdkTransportThread::CloseTrackQueues()
{
	for (auto& track : trackQueues_) {
		if (track) track->queue.Close();
	}
}

void PushSdkTransportThread::CompleteStartup(bool ok, int code, const std::string& error)
{
	{
		std::lock_guard<std::mutex> lock(startupMutex_);
		startupDone_ = true;
		startupOk_ = ok;
		startupCode_ = code;
		startupError_ = error;
	}
	startupCv_.notify_all();
}

void PushSdkTransportThread::RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs)
{
	lastHeartbeatUs_.store(nowUs, std::memory_order_relaxed);
	loopIterations_.fetch_add(1, std::memory_order_relaxed);
	if (!lastLoopUs) return;
	const int64_t gapUs = std::max<int64_t>(0, nowUs - *lastLoopUs);
	int64_t currentMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	while (gapUs > currentMaxUs &&
		!loopGapMaxUs_.compare_exchange_weak(
			currentMaxUs,
			gapUs,
			std::memory_order_relaxed,
			std::memory_order_relaxed)) {
	}
	*lastLoopUs = nowUs;
}

void PushSdkTransportThread::DrainUdpFeedback(
	PlainUdpTransport& udp,
	webrtc_qos::VideoPushClient& push,
	int64_t nowUs,
	size_t maxPackets)
{
	uint8_t buffer[2048];
	for (size_t processed = 0; processed < maxPackets; ++processed) {
		UdpEndpoint from;
		std::string error;
		const ssize_t received = udp.Recv(buffer, sizeof(buffer), &from, &error);
		if (received == 0) return;
		if (received < 0) {
			if (logger_) logger_->warn("udp_recv_failed error={}", error);
			return;
		}
		const auto kind = ClassifyRtpOrRtcp(buffer, static_cast<size_t>(received));
		if (kind == PacketKind::Rtcp) {
			rtcpPacketsIn_.fetch_add(1, std::memory_order_relaxed);
			rtcpBytesIn_.fetch_add(static_cast<uint64_t>(received), std::memory_order_relaxed);
			auto status = push.OnTransportFeedback(buffer, static_cast<size_t>(received), nowUs);
			if (!status) {
				rtcpFailures_.fetch_add(1, std::memory_order_relaxed);
				if (logger_) logger_->warn("push_feedback_failed bytes={} from={}:{} status={}",
					received,
					from.ip,
					from.port,
					StatusToString(status));
			}
		} else if (kind == PacketKind::Rtp) {
			unexpectedRtpPacketsIn_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->warn("unexpected_inbound_rtp bytes={} from={}:{}", received, from.ip, from.port);
		} else {
			malformedPacketsIn_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->warn("malformed_inbound_packet bytes={} from={}:{}", received, from.ip, from.port);
		}
	}
}

void PushSdkTransportThread::StoreEndpoints(const UdpEndpoint& local, const UdpEndpoint& remote)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	localEndpoint_ = local;
	remoteEndpoint_ = remote;
}

void PushSdkTransportThread::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	stopReason_ = reason;
}

void PushSdkTransportThread::StoreFatalError(const std::string& error)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	fatalError_ = error;
}

} // namespace webrtc_qos_plain
