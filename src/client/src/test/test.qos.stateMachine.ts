import {
	createInitialQosStateMachineContext,
	evaluateStateTransition,
	mapStateToQuality,
} from '../qos/stateMachine';
import type {
	DerivedQosSignals,
	QosProfile,
	QosStateMachineContext,
} from '../qos/types';

function makeProfile(overrides: Partial<QosProfile> = {}): QosProfile {
	return {
		name: 'test-camera',
		source: 'camera',
		levelCount: 5,
		sampleIntervalMs: 1000,
		snapshotIntervalMs: 2000,
		recoveryCooldownMs: 8000,
		thresholds: {
			warnLossRate: 0.04,
			congestedLossRate: 0.08,
			warnRttMs: 220,
			congestedRttMs: 320,
			warnBitrateUtilization: 0.85,
			congestedBitrateUtilization: 0.65,
			stableLossRate: 0.03,
			stableRttMs: 180,
			stableBitrateUtilization: 0.92,
		},
		ladder: [],
		...overrides,
	};
}

function makeSignals(
	overrides: Partial<DerivedQosSignals> = {}
): DerivedQosSignals {
	return {
		packetsSentDelta: 100,
		packetsLostDelta: 0,
		retransmittedPacketsSentDelta: 0,
		sendBitrateBps: 900000,
		targetBitrateBps: 1000000,
		bitrateUtilization: 0.95,
		lossRate: 0.0,
		lossEwma: 0.01,
		rttMs: 120,
		rttEwma: 120,
		jitterMs: 10,
		jitterEwma: 10,
		cpuLimited: false,
		bandwidthLimited: false,
		reason: 'network',
		...overrides,
	};
}

test('stable -> early_warning after two warning samples', () => {
	const profile = makeProfile();
	let context = createInitialQosStateMachineContext(0);
	const warningSignals = makeSignals({ bitrateUtilization: 0.8 });

	let result = evaluateStateTransition(context, warningSignals, profile, 1000);

	expect(result.context.state).toBe('stable');
	expect(result.transitioned).toBe(false);

	context = result.context;
	result = evaluateStateTransition(context, warningSignals, profile, 2000);
	expect(result.context.state).toBe('early_warning');
	expect(result.transitioned).toBe(true);
});

test('early_warning -> congested after two congested samples', () => {
	const profile = makeProfile();
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'early_warning' as const,
	};
	const congestedSignals = makeSignals({
		bitrateUtilization: 0.5,
		lossEwma: 0.1,
	});

	let result = evaluateStateTransition(
		context,
		congestedSignals,
		profile,
		1000
	);

	expect(result.context.state).toBe('early_warning');
	expect(result.transitioned).toBe(false);

	context = result.context;
	result = evaluateStateTransition(context, congestedSignals, profile, 2000);
	expect(result.context.state).toBe('congested');
	expect(result.transitioned).toBe(true);
	expect(result.context.lastCongestedAtMs).toBe(2000);
});

test('early_warning -> stable after three healthy samples', () => {
	const profile = makeProfile();
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'early_warning' as const,
	};
	const healthySignals = makeSignals();

	let result = evaluateStateTransition(context, healthySignals, profile, 1000);

	expect(result.context.state).toBe('early_warning');
	context = result.context;

	result = evaluateStateTransition(context, healthySignals, profile, 2000);
	expect(result.context.state).toBe('early_warning');
	context = result.context;

	result = evaluateStateTransition(context, healthySignals, profile, 3000);
	expect(result.context.state).toBe('stable');
	expect(result.transitioned).toBe(true);
});

test('congested -> recovering requires cooldown and five healthy samples', () => {
	const profile = makeProfile({ recoveryCooldownMs: 8000 });
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'congested' as const,
		lastCongestedAtMs: 1000,
	};
	const healthySignals = makeSignals();

	// Accumulate healthy samples before cooldown expiry.
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		2000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		3000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		4000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		5000
	).context;
	let result = evaluateStateTransition(context, healthySignals, profile, 6000);

	expect(result.context.state).toBe('congested');

	// Cooldown elapsed and healthy count satisfied.
	context = result.context;
	result = evaluateStateTransition(context, healthySignals, profile, 9000);
	expect(result.context.state).toBe('recovering');
	expect(result.transitioned).toBe(true);
	expect(result.context.lastRecoveryAtMs).toBe(9000);
});

test('recovering -> stable after five healthy samples', () => {
	const profile = makeProfile();
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'recovering' as const,
	};
	const healthySignals = makeSignals();

	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		1000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		2000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		3000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		4000
	).context;
	const result = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		5000
	);

	expect(result.context.state).toBe('stable');
	expect(result.transitioned).toBe(true);
});

test('recovering -> congested after two congested samples', () => {
	const profile = makeProfile();
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'recovering' as const,
	};
	const congestedSignals = makeSignals({
		bitrateUtilization: 0.5,
		lossEwma: 0.12,
	});

	context = evaluateStateTransition(
		context,
		congestedSignals,
		profile,
		1000
	).context;
	const result = evaluateStateTransition(
		context,
		congestedSignals,
		profile,
		2000
	);

	expect(result.context.state).toBe('congested');
	expect(result.transitioned).toBe(true);
});

test('illegal transition: stable never jumps directly to congested', () => {
	const profile = makeProfile();
	let context = createInitialQosStateMachineContext(0);
	const severeSignals = makeSignals({ bitrateUtilization: 0.2, lossEwma: 0.2 });

	const result = evaluateStateTransition(context, severeSignals, profile, 1000);

	expect(result.context.state).toBe('stable');

	context = result.context;
	const second = evaluateStateTransition(context, severeSignals, profile, 2000);

	expect(second.context.state).toBe('early_warning');
});

test('illegal transition: congested does not jump directly to stable', () => {
	const profile = makeProfile();
	let context: QosStateMachineContext = {
		...createInitialQosStateMachineContext(0),
		state: 'congested' as const,
		lastCongestedAtMs: 1000,
	};

	const healthySignals = makeSignals();

	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		10000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		11000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		12000
	).context;
	context = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		13000
	).context;
	const result = evaluateStateTransition(
		context,
		healthySignals,
		profile,
		14000
	);

	expect(result.context.state).toBe('recovering');
	expect(result.context.state).not.toBe('stable');
});

test('mapStateToQuality returns expected values', () => {
	const healthy = makeSignals();
	const lostLike = makeSignals({ sendBitrateBps: 0 });

	expect(mapStateToQuality('stable', healthy)).toBe('excellent');
	expect(mapStateToQuality('early_warning', healthy)).toBe('good');
	expect(mapStateToQuality('recovering', healthy)).toBe('good');
	expect(mapStateToQuality('congested', healthy)).toBe('poor');
	expect(mapStateToQuality('congested', lostLike)).toBe('lost');
});
