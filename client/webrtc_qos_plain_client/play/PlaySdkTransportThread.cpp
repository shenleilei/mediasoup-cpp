#include "play/PlaySdkTransportThread.h"

#include <algorithm>
#include <chrono>
#include <utility>

#include "common/RtpRtcpClassifier.h"
#include "common/RuntimeLogHelpers.h"
#include "common/SdkRuntimeConfig.h"

namespace webrtc_qos_plain {

PlaySdkTransportThread::PlaySdkTransportThread(
	PlaySdkTransportThreadConfig config,
	std::shared_ptr<spdlog::logger> logger)
	: config_(std::move(config)),
	  logger_(std::move(logger))
{
	const auto& tracks = config_.session.video_tracks;
	if (tracks.empty()) {
		trackSnapshots_.push_back({config_.session.ids, std::make_unique<LatestValue<webrtc_qos::QosSnapshot>>()});
	} else {
		for (const auto& track : tracks) {
			if (!track.enabled) continue;
			trackSnapshots_.push_back({track.ids, std::make_unique<LatestValue<webrtc_qos::QosSnapshot>>()});
		}
	}
	if (trackSnapshots_.empty()) {
		trackSnapshots_.push_back({config_.session.ids, std::make_unique<LatestValue<webrtc_qos::QosSnapshot>>()});
	}
}

PlaySdkTransportThread::~PlaySdkTransportThread()
{
	Stop();
}

int PlaySdkTransportThread::Start(std::string* error)
{
	if (running_.exchange(true)) return 0;
	started_.store(false, std::memory_order_relaxed);
	stopped_.store(false, std::memory_order_relaxed);
	lastHeartbeatUs_.store(0, std::memory_order_relaxed);
	loopGapMaxUs_.store(0, std::memory_order_relaxed);
	loopIterations_.store(0, std::memory_order_relaxed);
	rtpPacketsIn_.store(0, std::memory_order_relaxed);
	rtcpPacketsIn_.store(0, std::memory_order_relaxed);
	malformedPacketsIn_.store(0, std::memory_order_relaxed);
	packetInputFailures_.store(0, std::memory_order_relaxed);
	rtcpPacketsOut_.store(0, std::memory_order_relaxed);
	rtcpBytesOut_.store(0, std::memory_order_relaxed);
	rtcpSendFailures_.store(0, std::memory_order_relaxed);
	StoreEndpoints(config_.udp.localEndpoint(), {config_.mediaRemoteIp, config_.mediaRemotePort});
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
	lock.unlock();
	if (thread_.joinable()) thread_.join();
	if (error) *error = startupError_;
	return startupCode_ == 0 ? 3 : startupCode_;
}

void PlaySdkTransportThread::Stop()
{
	if (!running_.exchange(false) && !thread_.joinable()) return;
	if (thread_.joinable()) thread_.join();
}

PlaySdkTransportThreadMetrics PlaySdkTransportThread::metrics() const
{
	PlaySdkTransportThreadMetrics out;
	out.started = started_.load(std::memory_order_relaxed);
	out.stopped = stopped_.load(std::memory_order_relaxed);
	out.lastHeartbeatUs = lastHeartbeatUs_.load(std::memory_order_relaxed);
	out.loopGapMaxUs = loopGapMaxUs_.load(std::memory_order_relaxed);
	out.loopIterations = loopIterations_.load(std::memory_order_relaxed);
	out.rtpPacketsIn = rtpPacketsIn_.load(std::memory_order_relaxed);
	out.rtcpPacketsIn = rtcpPacketsIn_.load(std::memory_order_relaxed);
	out.malformedPacketsIn = malformedPacketsIn_.load(std::memory_order_relaxed);
	out.packetInputFailures = packetInputFailures_.load(std::memory_order_relaxed);
	out.rtcpPacketsOut = rtcpPacketsOut_.load(std::memory_order_relaxed);
	out.rtcpBytesOut = rtcpBytesOut_.load(std::memory_order_relaxed);
	out.rtcpSendFailures = rtcpSendFailures_.load(std::memory_order_relaxed);
	(void)snapshot_.Load(&out.snapshot);
	out.tracks.reserve(trackSnapshots_.size());
	for (const auto& item : trackSnapshots_) {
		PlaySdkTrackMetrics trackMetrics;
		trackMetrics.trackId = item.first.track_id;
		trackMetrics.senderSsrc = item.first.sender_ssrc;
		if (item.second) {
			trackMetrics.snapshotAvailable = item.second->Load(&trackMetrics.snapshot);
		}
		out.tracks.push_back(trackMetrics);
	}
	{
		std::lock_guard<std::mutex> lock(metadataMutex_);
		out.localEndpoint = localEndpoint_;
		out.remoteEndpoint = remoteEndpoint_;
		out.stopReason = stopReason_;
		out.fatalError = fatalError_;
	}
	return out;
}

bool PlaySdkTransportThread::hasFatalError() const
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	return !fatalError_.empty();
}

void PlaySdkTransportThread::Run()
{
	PlainUdpTransport udp = std::move(config_.udp);
	if (udp.fd() < 0) {
		StoreFatalError("UDP socket is not open");
		StoreStopReason("udp_not_open");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 2, "UDP socket is not open");
		return;
	}
	StoreEndpoints(udp.localEndpoint(), {config_.mediaRemoteIp, config_.mediaRemotePort});

	webrtc_qos::VideoPlayClientConfig sdkConfig;
	sdkConfig.session = config_.session;
	sdkConfig.decoded_access_unit_output = config_.decodedAccessUnitOutput;
	const bool sdkRuntimeFilesEnabled = ConfigureSdkRuntimeFiles(sdkConfig, "play", config_.logDir);
	if (logger_) logger_->info("sdk_runtime_files role=play enabled={}", sdkRuntimeFilesEnabled);
	sdkConfig.transport_output = [&](const webrtc_qos::TransportPacketView& packet) {
		std::string sendError;
		const bool isRtcp = packet.metadata.kind == webrtc_qos::TransportPacketKind::kRtcp;
		if (isRtcp) {
			rtcpPacketsOut_.fetch_add(1, std::memory_order_relaxed);
			rtcpBytesOut_.fetch_add(static_cast<uint64_t>(packet.size), std::memory_order_relaxed);
		}
		const bool ok = udp.SendTo(
			config_.mediaRemoteIp,
			config_.mediaRemotePort,
			packet.bytes,
			packet.size,
			&sendError);
		if (!ok) {
			if (isRtcp) rtcpSendFailures_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->error("sdk_transport_output_failed kind={} bytes={} remoteIp={} remotePort={} error={}",
				isRtcp ? "rtcp" : "rtp",
				packet.size,
				config_.mediaRemoteIp,
				config_.mediaRemotePort,
				sendError);
			return webrtc_qos::Status::Error(
				webrtc_qos::StatusCode::kInternalError,
				sendError);
		}
		return webrtc_qos::Status::Ok();
	};

	auto play = webrtc_qos::CreateVideoPlayClient(sdkConfig);
	if (!play) {
		StoreFatalError("create_video_play_client_failed");
		StoreStopReason("create_failed");
		stopped_.store(true, std::memory_order_relaxed);
		CompleteStartup(false, 3, "create_video_play_client_failed");
		return;
	}
	auto status = play->Start();
	if (!status) {
		const std::string statusText = StatusToString(status);
		StoreFatalError(statusText);
		StoreStopReason("start_failed");
		stopped_.store(true, std::memory_order_relaxed);
		if (logger_) logger_->error("play_start_failed status={}", statusText);
		CompleteStartup(false, 3, statusText);
		return;
	}

	started_.store(true, std::memory_order_relaxed);
	lastHeartbeatUs_.store(MonotonicNowUs(), std::memory_order_relaxed);
	snapshot_.Store(play->GetQosSnapshot(MonotonicNowUs()));
	for (auto& item : trackSnapshots_) {
		webrtc_qos::QosSnapshot trackSnapshot;
		if (item.second && play->GetTrackQosSnapshot(item.first.track_id, MonotonicNowUs(), &trackSnapshot)) {
			item.second->Store(trackSnapshot);
		}
	}
	if (logger_) logger_->info(
		"play_sdk_transport_thread_started udpLocalIp={} udpLocalPort={} udpRemoteIp={} udpRemotePort={} tracks={}",
		udp.localEndpoint().ip,
		udp.localEndpoint().port,
		config_.mediaRemoteIp,
		config_.mediaRemotePort,
		trackSnapshots_.size());
	CompleteStartup(true, 0, "");

	int64_t lastLoopUs = MonotonicNowUs();
	while (running_.load(std::memory_order_relaxed)) {
		const int64_t nowUs = MonotonicNowUs();
		RecordLoopTick(nowUs, &lastLoopUs);
		DrainUdp(udp, *play, nowUs, 64);
		status = play->Process(nowUs);
		if (!status) {
			const std::string statusText = StatusToString(status);
			StoreFatalError(statusText);
			if (logger_) logger_->error("play_process_failed status={}", statusText);
			break;
		}
		snapshot_.Store(play->GetQosSnapshot(nowUs));
		for (auto& item : trackSnapshots_) {
			webrtc_qos::QosSnapshot trackSnapshot;
			if (item.second && play->GetTrackQosSnapshot(item.first.track_id, nowUs, &trackSnapshot)) {
				item.second->Store(trackSnapshot);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(std::max(1, config_.processTickMs)));
	}

	status = play->Stop();
	if (!status && logger_) logger_->warn("play_stop_failed status={}", StatusToString(status));
	StoreStopReason(hasFatalError() ? "fatal_error" : "stopped");
	stopped_.store(true, std::memory_order_relaxed);
	if (logger_) {
		const auto finalMetrics = metrics();
		logger_->info(
			"play_sdk_transport_thread_stopped stopReason={} tracks={} rtpPackets={} rtcpPackets={} rtcpPacketsOut={} rtcpBytesOut={} rtcpSendFailures={} packetInputFailures={} loopIterations={} loopGapMaxUs={}",
			finalMetrics.stopReason,
			finalMetrics.tracks.size(),
			finalMetrics.rtpPacketsIn,
			finalMetrics.rtcpPacketsIn,
			finalMetrics.rtcpPacketsOut,
			finalMetrics.rtcpBytesOut,
			finalMetrics.rtcpSendFailures,
			finalMetrics.packetInputFailures,
			finalMetrics.loopIterations,
			finalMetrics.loopGapMaxUs);
	}
}

void PlaySdkTransportThread::CompleteStartup(bool ok, int code, const std::string& error)
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

void PlaySdkTransportThread::RecordLoopTick(int64_t nowUs, int64_t* lastLoopUs)
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

void PlaySdkTransportThread::DrainUdp(
	PlainUdpTransport& udp,
	webrtc_qos::VideoPlayClient& play,
	int64_t nowUs,
	size_t maxPackets)
{
	uint8_t buffer[2048];
	for (size_t processed = 0; processed < maxPackets; ++processed) {
		UdpEndpoint from;
		std::string recvError;
		const ssize_t received = udp.Recv(buffer, sizeof(buffer), &from, &recvError);
		if (received == 0) return;
		if (received < 0) {
			if (logger_) logger_->warn("udp_recv_failed error={}", recvError);
			return;
		}
		const auto kind = ClassifyRtpOrRtcp(buffer, static_cast<size_t>(received));
		webrtc_qos::Status status = webrtc_qos::Status::Ok();
		if (kind == PacketKind::Rtp) {
			status = play.OnRtpPacket(buffer, static_cast<size_t>(received), nowUs);
			rtpPacketsIn_.fetch_add(1, std::memory_order_relaxed);
		} else if (kind == PacketKind::Rtcp) {
			status = play.OnRtcpPacket(buffer, static_cast<size_t>(received), nowUs);
			rtcpPacketsIn_.fetch_add(1, std::memory_order_relaxed);
		} else {
			malformedPacketsIn_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->warn("malformed_inbound_packet bytes={} from={}:{}", received, from.ip, from.port);
			continue;
		}
		if (!status) {
			packetInputFailures_.fetch_add(1, std::memory_order_relaxed);
			if (logger_) logger_->warn("play_packet_input_failed kind={} bytes={} status={}",
				PacketKindName(kind),
				received,
				StatusToString(status));
		}
	}
}

void PlaySdkTransportThread::StoreEndpoints(const UdpEndpoint& local, const UdpEndpoint& remote)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	localEndpoint_ = local;
	remoteEndpoint_ = remote;
}

void PlaySdkTransportThread::StoreStopReason(const std::string& reason)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	stopReason_ = reason;
}

void PlaySdkTransportThread::StoreFatalError(const std::string& error)
{
	std::lock_guard<std::mutex> lock(metadataMutex_);
	fatalError_ = error;
}

} // namespace webrtc_qos_plain
