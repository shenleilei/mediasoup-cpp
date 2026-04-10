import type {
	DerivedQosSignals,
	QosProfile,
	QosQuality,
	QosState,
	QosStateMachineContext,
	QosStateTransitionResult,
} from './types';

function isWarning(signals: DerivedQosSignals, profile: QosProfile): boolean {
	const { thresholds } = profile;

	return (
		signals.bitrateUtilization < thresholds.warnBitrateUtilization ||
		signals.lossEwma >= thresholds.warnLossRate ||
		signals.rttEwma >= thresholds.warnRttMs ||
		signals.bandwidthLimited ||
		signals.cpuLimited
	);
}

function isCongested(signals: DerivedQosSignals, profile: QosProfile): boolean {
	const { thresholds } = profile;

	return (
		signals.bitrateUtilization < thresholds.congestedBitrateUtilization ||
		signals.lossEwma >= thresholds.congestedLossRate ||
		signals.rttEwma >= thresholds.congestedRttMs
	);
}

function isHealthy(signals: DerivedQosSignals, profile: QosProfile): boolean {
	const { thresholds } = profile;

	return (
		signals.bitrateUtilization >= thresholds.stableBitrateUtilization &&
		signals.lossEwma < thresholds.stableLossRate &&
		signals.rttEwma < thresholds.stableRttMs &&
		!signals.bandwidthLimited &&
		!signals.cpuLimited
	);
}

function nextCounter(current: number, matched: boolean): number {
	return matched ? current + 1 : 0;
}

export function createInitialQosStateMachineContext(
	nowMs: number
): QosStateMachineContext {
	return {
		state: 'stable',
		enteredAtMs: nowMs,
		consecutiveHealthySamples: 0,
		consecutiveWarningSamples: 0,
		consecutiveCongestedSamples: 0,
	};
}

export function mapStateToQuality(
	state: QosState,
	signals: DerivedQosSignals
): QosQuality {
	if (state === 'congested' && signals.sendBitrateBps <= 0) {
		return 'lost';
	}

	switch (state) {
		case 'stable': {
			return 'excellent';
		}

		case 'early_warning': {
			return 'good';
		}

		case 'congested': {
			return 'poor';
		}

		case 'recovering': {
			return 'good';
		}
	}
}

export function evaluateStateTransition(
	context: QosStateMachineContext,
	signals: DerivedQosSignals,
	profile: QosProfile,
	nowMs: number
): QosStateTransitionResult {
	const healthy = isHealthy(signals, profile);
	const warning = isWarning(signals, profile);
	const congested = isCongested(signals, profile);

	let nextContext: QosStateMachineContext = {
		...context,
		consecutiveHealthySamples: nextCounter(
			context.consecutiveHealthySamples,
			healthy
		),
		consecutiveWarningSamples: nextCounter(
			context.consecutiveWarningSamples,
			warning
		),
		consecutiveCongestedSamples: nextCounter(
			context.consecutiveCongestedSamples,
			congested
		),
	};

	let nextState = context.state;
	let transitioned = false;

	switch (context.state) {
		case 'stable': {
			if (nextContext.consecutiveWarningSamples >= 2) {
				nextState = 'early_warning';
			}
			break;
		}

		case 'early_warning': {
			if (nextContext.consecutiveCongestedSamples >= 2) {
				nextState = 'congested';
			} else if (nextContext.consecutiveHealthySamples >= 3) {
				nextState = 'stable';
			}
			break;
		}

		case 'congested': {
			const congestedSinceMs = context.lastCongestedAtMs ?? context.enteredAtMs;
			const cooldownElapsed =
				nowMs - congestedSinceMs >= profile.recoveryCooldownMs;

			if (cooldownElapsed && nextContext.consecutiveHealthySamples >= 5) {
				nextState = 'recovering';
			}
			break;
		}

		case 'recovering': {
			if (nextContext.consecutiveCongestedSamples >= 2) {
				nextState = 'congested';
			} else if (nextContext.consecutiveHealthySamples >= 5) {
				nextState = 'stable';
			}
			break;
		}
	}

	if (nextState !== context.state) {
		transitioned = true;
		nextContext = {
			...nextContext,
			state: nextState,
			enteredAtMs: nowMs,
		};

		if (nextState === 'congested') {
			nextContext.lastCongestedAtMs = nowMs;
		}

		if (nextState === 'recovering') {
			nextContext.lastRecoveryAtMs = nowMs;
		}
	}

	return {
		context: nextContext,
		quality: mapStateToQuality(nextContext.state, signals),
		transitioned,
	};
}
