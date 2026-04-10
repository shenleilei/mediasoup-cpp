import type {
	DerivedQosSignals,
	QosProbeContext,
	QosProbeResult,
	QosProfile,
} from './types';

export type QosProbeEvaluation = {
	context?: QosProbeContext;
	result: QosProbeResult;
};

function isProbeHealthy(
	signals: DerivedQosSignals,
	profile: QosProfile
): boolean {
	const { thresholds } = profile;

	return (
		signals.bitrateUtilization >= thresholds.stableBitrateUtilization &&
		signals.lossEwma < thresholds.stableLossRate &&
		signals.rttEwma < thresholds.stableRttMs &&
		!signals.bandwidthLimited &&
		!signals.cpuLimited
	);
}

function isProbeBad(signals: DerivedQosSignals, profile: QosProfile): boolean {
	const { thresholds } = profile;

	return (
		signals.bitrateUtilization < thresholds.congestedBitrateUtilization ||
		signals.lossEwma >= thresholds.congestedLossRate ||
		signals.rttEwma >= thresholds.congestedRttMs ||
		signals.bandwidthLimited ||
		signals.cpuLimited
	);
}

export function beginProbe(
	previousLevel: number,
	targetLevel: number,
	startedAtMs: number,
	previousAudioOnlyMode: boolean,
	targetAudioOnlyMode: boolean
): QosProbeContext {
	return {
		active: true,
		startedAtMs,
		previousLevel,
		targetLevel,
		previousAudioOnlyMode,
		targetAudioOnlyMode,
		healthySamples: 0,
		badSamples: 0,
	};
}

export function evaluateProbe(
	context: QosProbeContext | undefined,
	signals: DerivedQosSignals,
	profile: QosProfile
): QosProbeEvaluation {
	if (!context?.active) {
		return { result: 'inconclusive' };
	}

	let nextContext: QosProbeContext = { ...context };

	if (isProbeHealthy(signals, profile)) {
		nextContext = {
			...nextContext,
			healthySamples: nextContext.healthySamples + 1,
			badSamples: 0,
		};
	} else if (isProbeBad(signals, profile)) {
		nextContext = {
			...nextContext,
			healthySamples: 0,
			badSamples: nextContext.badSamples + 1,
		};
	}

	if (nextContext.badSamples >= 2) {
		return {
			context: nextContext,
			result: 'failed',
		};
	}

	if (nextContext.healthySamples >= 3) {
		return {
			context: nextContext,
			result: 'successful',
		};
	}

	return {
		context: nextContext,
		result: 'inconclusive',
	};
}
