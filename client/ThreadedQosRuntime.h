#pragma once

#include "NetworkThread.h"
#include "PlainClientApp.h"
#include "SourceWorker.h"
#include "ThreadTypes.h"
#include "ThreadedControlHelpers.h"

#include <atomic>
#include <memory>
#include <set>
#include <thread>
#include <vector>

class ThreadedQosRuntime {
public:
	explicit ThreadedQosRuntime(PlainClientApp& app);
	~ThreadedQosRuntime();

	int Run();

private:
	struct PerTrackQueues {
		mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> auQueue;
		mt::SpscQueue<mt::TrackControlCommand, mt::kControlCommandQueueCapacity> ctrlQueue;
		mt::SpscQueue<mt::NetworkToSourceCommand, mt::kNetworkSourceQueueCapacity> netCmdQueue;
		mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> ackQueue;
	};

	void setupQueues();
	void setupTrackControlState();
	void setupNetworkThread();
	void startAudioThreadIfNeeded();
	void resolveAudioAvailability();
	void startSourceWorkers();
	void createQosControllers();
	void processStatsQueue();
	void processNetworkAckQueue();
	void processWorkerAckQueues();
	void maybeRunSampling();
	void sampleTracks(int64_t nowMs, std::set<std::string>& sampledTrackIds);
	void applyPeerCoordinationAndMaybeUpload(const std::set<std::string>& sampledTrackIds);
	void applyAck(const mt::CommandAck& ack, bool logRejectReason);
	void syncTransportHint(size_t index);
	void clearCoordinationOverrides();
	void publishPeerSnapshot(
		const std::vector<qos::PeerTrackState>& peerTrackStates,
		const std::map<std::string, qos::DerivedSignals>& peerTrackSignals,
		const std::map<std::string, qos::PlannedAction>& peerLastActions,
		const std::map<std::string, qos::CanonicalTransportSnapshot>& peerRawSnapshots,
		const std::map<std::string, bool>& peerActionApplied);
	void logTrackSample(
		const PlainClientApp::VideoTrackRuntime& track,
		size_t trackIndex,
		const qos::CanonicalTransportSnapshot& snapshot,
		double observedBitrateBps) const;
	void stop();

	PlainClientApp& app_;
	std::vector<std::unique_ptr<PerTrackQueues>> queues_;
	mt::SpscQueue<mt::SenderStatsSnapshot, mt::kStatsQueueCapacity> statsQueue_;
	mt::SpscQueue<mt::NetworkControlCommand, mt::kControlCommandQueueCapacity> networkControlQueue_;
	mt::SpscQueue<mt::CommandAck, mt::kCommandAckQueueCapacity> networkAckQueue_;
	mt::SpscQueue<mt::EncodedAccessUnit, mt::kEncodedAuQueueCapacity> audioAuQueue_;
	std::unique_ptr<NetworkThread> netThread_;
	std::atomic<bool> audioRunning_{false};
	std::atomic<bool> audioActuallyStarted_{false};
	std::thread audioThread_;
	bool hasAudio_{false};
	std::vector<std::unique_ptr<SourceWorker>> workers_;
	std::vector<mt::PendingTrackCommand> pendingCommands_;
	std::vector<mt::ThreadedTrackControlState> trackControlStates_;
	std::vector<mt::ThreadedTrackStatsState> trackStats_;
	uint64_t nextCommandId_{1};
	int64_t lastPeerQosSampleMs_{0};
	int peerSnapshotSeq_{0};
	bool stopped_{false};
};
