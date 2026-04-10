import * as fs from 'node:fs';
import * as path from 'node:path';

import { getDefaultCameraProfile } from '../qos/profiles';
import { planActions } from '../qos/planner';
import type { QosPlannerInput, QosState } from '../qos/types';

type ReplayStep = {
	state: QosState;
	expectedPrimaryAction: string;
	expectedLevel: number;
};

type ReplayScenario = {
	initialLevel: number;
	initialAudioOnly: boolean;
	steps: ReplayStep[];
	expectedFinalLevel: number;
	expectedFinalAudioOnly: boolean;
};

function loadReplayFixture(name: string): ReplayScenario {
	const fixturePath = path.join(__dirname, 'fixtures', 'qos', `${name}.json`);
	const content = fs.readFileSync(fixturePath, 'utf8');
	const parsed = JSON.parse(content) as ReplayScenario;

	return parsed;
}

function makePlannerInput(
	level: number,
	state: QosState,
	inAudioOnlyMode: boolean
): QosPlannerInput {
	return {
		source: 'camera',
		profile: getDefaultCameraProfile(),
		state,
		currentLevel: level,
		inAudioOnlyMode,
		signals: {
			packetsSentDelta: 100,
			packetsLostDelta: 0,
			retransmittedPacketsSentDelta: 0,
			sendBitrateBps: 800000,
			targetBitrateBps: 900000,
			bitrateUtilization: 0.9,
			lossRate: 0.01,
			lossEwma: 0.01,
			rttMs: 120,
			rttEwma: 120,
			jitterMs: 8,
			jitterEwma: 8,
			cpuLimited: false,
			bandwidthLimited: false,
			reason: 'network',
		},
	};
}

function applyActions(
	level: number,
	inAudioOnlyMode: boolean,
	actions: ReturnType<typeof planActions>
): { nextLevel: number; nextAudioOnly: boolean } {
	let nextLevel = level;
	let nextAudioOnly = inAudioOnlyMode;

	for (const action of actions) {
		switch (action.type) {
			case 'setEncodingParameters':
			case 'setMaxSpatialLayer':
			case 'noop': {
				nextLevel = action.level;
				break;
			}

			case 'enterAudioOnly': {
				nextLevel = action.level;
				nextAudioOnly = true;
				break;
			}

			case 'exitAudioOnly': {
				nextLevel = action.level;
				nextAudioOnly = false;
				break;
			}
		}
	}

	return { nextLevel, nextAudioOnly };
}

function assertScenario(name: string): void {
	const scenario = loadReplayFixture(name);
	let level = scenario.initialLevel;
	let inAudioOnlyMode = scenario.initialAudioOnly;
	const primaryActions: string[] = [];

	for (const step of scenario.steps) {
		const input = makePlannerInput(level, step.state, inAudioOnlyMode);
		const actions = planActions(input);

		expect(actions.length).toBeGreaterThan(0);
		primaryActions.push(actions[0].type);
		expect(actions[0].type).toBe(step.expectedPrimaryAction);

		const next = applyActions(level, inAudioOnlyMode, actions);

		level = next.nextLevel;
		inAudioOnlyMode = next.nextAudioOnly;
		expect(level).toBe(step.expectedLevel);
	}

	expect(level).toBe(scenario.expectedFinalLevel);
	expect(inAudioOnlyMode).toBe(scenario.expectedFinalAudioOnly);
	expect(primaryActions.length).toBe(scenario.steps.length);
}

test('replay camera_mild_congestion scenario', () => {
	expect.hasAssertions();

	assertScenario('camera_mild_congestion');
});

test('replay camera_persistent_congestion scenario', () => {
	expect.hasAssertions();

	assertScenario('camera_persistent_congestion');
});

test('replay camera_recovery scenario', () => {
	expect.hasAssertions();

	assertScenario('camera_recovery');
});
