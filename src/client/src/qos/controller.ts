import { QOS_CLIENT_SCHEMA_V1 } from './constants';
import type { QosSignalChannel } from './adapters/signalChannel';
import type { QosActionExecutionResult } from './executor';
import { planActions, planRecovery } from './planner';
import { planActionsForLevel } from './planner';
import { beginProbe, evaluateProbe } from './probe';
import { IntervalQosSampler, type QosStatsProvider } from './sampler';
import { deriveSignals } from './signals';
import {
	createInitialQosStateMachineContext,
	evaluateStateTransition,
	mapStateToQuality,
} from './stateMachine';
import { QosTraceBuffer } from './trace';
import type { QosClock } from './clock';
import type {
	ClientQosSnapshot,
	PeerQosTrackState,
	PlannedQosAction,
	QosOverride,
	QosPolicy,
	QosProfile,
	QosProbeContext,
	QosStateMachineContext,
	QosTrackKind,
	QosTrackMode,
	RawSenderSnapshot,
} from './types';

export type PublisherQosControllerActionExecutor = {
	execute(
		action: PlannedQosAction
	): Promise<QosActionExecutionResult | boolean | void>;
};
export type PublisherQosControllerSignalChannel = {
	publishSnapshot(snapshot: ClientQosSnapshot): Promise<void>;
	onPolicy?: (listener: Parameters<QosSignalChannel['onPolicy']>[0]) => void;
	onOverride?: (
		listener: Parameters<QosSignalChannel['onOverride']>[0]
	) => void;
};

export type PublisherQosControllerOptions = {
	clock: QosClock;
	profile: QosProfile;
	statsProvider: QosStatsProvider;
	actionExecutor: PublisherQosControllerActionExecutor;
	signalChannel?: PublisherQosControllerSignalChannel;
	trackId: string;
	kind: QosTrackKind;
	producerId?: string;
	sampleIntervalMs?: number;
	snapshotIntervalMs?: number;
	traceCapacity?: number;
	allowAudioOnly?: boolean;
	allowVideoPause?: boolean;
	initialLevel?: number;
	initialAudioOnlyMode?: boolean;
};

type ActiveOverride = {
	data: QosOverride;
	expiresAtMs: number;
};

export type PublisherQosCoordinationOverride = {
	maxLevelClamp?: number;
	disableRecovery?: boolean;
	forceAudioOnly?: boolean;
};

function resolvePeerMode(
	inAudioOnlyMode: boolean,
	kind: QosTrackKind
): QosTrackMode {
	if (inAudioOnlyMode || kind === 'audio') {
		return 'audio-only';
	}

	return 'audio-video';
}

function toError(error: unknown): Error {
	if (error instanceof Error) {
		return error;
	}

	return new Error(String(error));
}

function wasActionApplied(
	result: boolean | void | QosActionExecutionResult | { applied?: boolean }
): boolean {
	if (result === undefined || result === true) {
		return true;
	}

	if (result === false) {
		return false;
	}

	if (typeof result === 'object' && result !== null) {
		if ('status' in result) {
			return result.status !== 'failed';
		}

		if ('applied' in result) {
			return result.applied !== false;
		}
	}

	return true;
}

export class PublisherQosController {
	private readonly sampler: IntervalQosSampler;
	private readonly trace: QosTraceBuffer;
	private readonly clock: QosClock;
	private readonly profile: QosProfile;
	private readonly signalChannel?: PublisherQosControllerSignalChannel;
	private readonly actionExecutor: PublisherQosControllerActionExecutor;
	private readonly trackId: string;
	private readonly kind: QosTrackKind;
	private readonly producerId?: string;
	private readonly statsProvider: QosStatsProvider;

	private stateContext: QosStateMachineContext;
	private previousSnapshot?: RawSenderSnapshot;
	private previousSignals?: ReturnType<typeof deriveSignals>;
	private seq = 0;
	private running = false;
	private currentLevel: number;
	private inAudioOnlyMode: boolean;
	private cpuSampleCount = 0;
	private lastPublishedAtMs?: number;
	private sampleIntervalMs: number;
	private snapshotIntervalMs: number;
	private allowAudioOnly: boolean;
	private allowVideoPause: boolean;
	private activeOverride?: ActiveOverride;
	private coordinationOverride?: PublisherQosCoordinationOverride;
	private probeContext?: QosProbeContext;
	private recoverySuppressedUntilMs?: number;
	private lastAction?: { type: PlannedQosAction['type']; applied: boolean };
	private lastSampleError?: Error;

	constructor(options: PublisherQosControllerOptions) {
		this.clock = options.clock;
		this.profile = options.profile;
		this.statsProvider = options.statsProvider;
		this.actionExecutor = options.actionExecutor;
		this.signalChannel = options.signalChannel;
		this.trackId = options.trackId;
		this.kind = options.kind;
		this.producerId = options.producerId;
		this.currentLevel = Math.max(0, Math.floor(options.initialLevel ?? 0));
		this.inAudioOnlyMode = options.initialAudioOnlyMode === true;
		this.snapshotIntervalMs =
			options.snapshotIntervalMs ?? options.profile.snapshotIntervalMs;
		this.sampleIntervalMs =
			options.sampleIntervalMs ?? options.profile.sampleIntervalMs;
		this.allowAudioOnly = options.allowAudioOnly ?? true;
		this.allowVideoPause = options.allowVideoPause ?? true;

		this.stateContext = createInitialQosStateMachineContext(this.clock.nowMs());
		this.trace = new QosTraceBuffer(options.traceCapacity);
		this.sampler = new IntervalQosSampler(
			this.clock,
			options.statsProvider,
			this.sampleIntervalMs
		);

		this.signalChannel?.onPolicy?.(policy => {
			this.handlePolicy(policy);
		});
		this.signalChannel?.onOverride?.(override => {
			this.handleOverride(override);
		});
	}

	get isRunning(): boolean {
		return this.running;
	}

	get level(): number {
		return this.currentLevel;
	}

	get state(): QosStateMachineContext['state'] {
		return this.stateContext.state;
	}

	getTraceEntries() {
		return this.trace.getTraceEntries();
	}

	getLastSampleError(): Error | undefined {
		return this.lastSampleError;
	}

	getRuntimeSettings(): {
		sampleIntervalMs: number;
		snapshotIntervalMs: number;
		allowAudioOnly: boolean;
		allowVideoPause: boolean;
		probeActive: boolean;
	} {
		return {
			sampleIntervalMs: this.sampleIntervalMs,
			snapshotIntervalMs: this.snapshotIntervalMs,
			allowAudioOnly: this.allowAudioOnly,
			allowVideoPause: this.allowVideoPause,
			probeActive: this.probeContext?.active === true,
		};
	}

	getCoordinationOverride(): PublisherQosCoordinationOverride | undefined {
		return this.coordinationOverride
			? { ...this.coordinationOverride }
			: undefined;
	}

	getTrackState(): PeerQosTrackState {
		return {
			trackId: this.trackId,
			source: this.profile.source,
			kind: this.kind,
			state: this.stateContext.state,
			quality: this.previousSignals
				? mapStateToQuality(this.stateContext.state, this.previousSignals)
				: 'excellent',
			level: this.currentLevel,
			inAudioOnlyMode: this.inAudioOnlyMode,
		};
	}

	setCoordinationOverride(override?: PublisherQosCoordinationOverride): void {
		if (!override) {
			this.coordinationOverride = undefined;

			return;
		}

		const normalized: PublisherQosCoordinationOverride = {};

		if (typeof override.maxLevelClamp === 'number') {
			normalized.maxLevelClamp = Math.max(
				0,
				Math.floor(override.maxLevelClamp)
			);
		}

		if (override.disableRecovery === true) {
			normalized.disableRecovery = true;
		}

		if (override.forceAudioOnly === true) {
			normalized.forceAudioOnly = true;
		}

		this.coordinationOverride =
			Object.keys(normalized).length > 0 ? normalized : undefined;
	}

	start(): void {
		if (this.running) {
			return;
		}

		this.running = true;
		this.sampler.start(
			async snapshot => {
				await this.processSample(snapshot);
			},
			error => {
				this.lastSampleError = error;
			}
		);
	}

	stop(): void {
		if (!this.running) {
			return;
		}

		this.running = false;
		this.sampler.stop();
	}

	async sampleNow(): Promise<void> {
		const snapshot = await this.sampler.sampleOnce();

		await this.processSample(snapshot);
	}

	private handlePolicy(policy: QosPolicy): void {
		this.sampleIntervalMs = policy.sampleIntervalMs;
		this.snapshotIntervalMs = policy.snapshotIntervalMs;
		this.allowAudioOnly = policy.allowAudioOnly;
		this.allowVideoPause = policy.allowVideoPause;
		this.sampler.updateIntervalMs(policy.sampleIntervalMs);
	}

	private handleOverride(override: QosOverride): void {
		if (override.scope === 'track' && override.trackId !== this.trackId) {
			return;
		}

		const nowMs = this.clock.nowMs();
		const ttlMs = Math.max(0, Math.floor(override.ttlMs));

		this.activeOverride = {
			data: override,
			expiresAtMs: nowMs + ttlMs,
		};
	}

	private getActiveOverride(nowMs: number): QosOverride | undefined {
		if (!this.activeOverride) {
			return undefined;
		}

		if (nowMs >= this.activeOverride.expiresAtMs) {
			this.activeOverride = undefined;

			return undefined;
		}

		return this.activeOverride.data;
	}

	private async processSample(snapshot: RawSenderSnapshot): Promise<void> {
		const nowMs = this.clock.nowMs();
		const stateBefore = this.stateContext.state;
		const override = this.getActiveOverride(nowMs);
		const activeOverride = Boolean(override);
		const coordinationOverride = this.coordinationOverride;
		const combinedClampLevels = [
			override?.maxLevelClamp,
			coordinationOverride?.maxLevelClamp,
		].filter((value): value is number => typeof value === 'number');
		const combinedClampLevel =
			combinedClampLevels.length > 0
				? Math.min(...combinedClampLevels)
				: undefined;
		const disableRecovery =
			coordinationOverride?.disableRecovery === true ||
			override?.disableRecovery === true ||
			(typeof this.recoverySuppressedUntilMs === 'number' &&
				nowMs < this.recoverySuppressedUntilMs);

		if (snapshot.qualityLimitationReason === 'cpu') {
			this.cpuSampleCount += 1;
		} else {
			this.cpuSampleCount = 0;
		}

		const signals = deriveSignals(
			snapshot,
			this.previousSnapshot,
			this.previousSignals,
			{
				reasonContext: {
					activeOverride,
					cpuSampleCount: this.cpuSampleCount,
				},
			}
		);

		const transition = evaluateStateTransition(
			this.stateContext,
			signals,
			this.profile,
			nowMs
		);

		this.stateContext = transition.context;

		if (this.probeContext) {
			const probeEvaluation = evaluateProbe(
				this.probeContext,
				signals,
				this.profile
			);

			if (probeEvaluation.result === 'failed') {
				await this.rollbackProbe(signals, nowMs, stateBefore);
				this.probeContext = undefined;
				this.recoverySuppressedUntilMs =
					nowMs + this.profile.recoveryCooldownMs;

				return;
			}

			if (probeEvaluation.result === 'successful') {
				this.probeContext = undefined;
			} else {
				this.probeContext = probeEvaluation.context;
			}
		}

		const plannedActions =
			this.stateContext.state === 'recovering' && !disableRecovery
				? planRecovery({
						source: this.profile.source,
						profile: this.profile,
						state: this.stateContext.state,
						currentLevel: this.currentLevel,
						overrideClampLevel: combinedClampLevel,
						inAudioOnlyMode: this.inAudioOnlyMode,
						signals,
					})
				: planActions({
						source: this.profile.source,
						profile: this.profile,
						state:
							this.stateContext.state === 'recovering'
								? 'early_warning'
								: this.stateContext.state,
						currentLevel: this.currentLevel,
						overrideClampLevel: combinedClampLevel,
						inAudioOnlyMode: this.inAudioOnlyMode,
						signals,
					});

		const filteredActions = this.filterActions(plannedActions, override);

		await this.executeActions(
			filteredActions,
			signals,
			nowMs,
			stateBefore,
			this.stateContext.state
		);

		const shouldPublish =
			transition.transitioned ||
			this.lastPublishedAtMs === undefined ||
			nowMs - this.lastPublishedAtMs >= this.snapshotIntervalMs;

		if (shouldPublish) {
			await this.publishSnapshot(snapshot, signals, transition.quality, nowMs);
			this.lastPublishedAtMs = nowMs;
		}

		this.previousSnapshot = snapshot;
		this.previousSignals = signals;
	}

	private filterActions(
		actions: PlannedQosAction[],
		override?: QosOverride
	): PlannedQosAction[] {
		const filtered: PlannedQosAction[] = [];

		for (const action of actions) {
			if (
				(action.type === 'enterAudioOnly' || action.type === 'exitAudioOnly') &&
				(!this.allowAudioOnly || !this.allowVideoPause)
			) {
				continue;
			}

			filtered.push(action);
		}

		if (override?.forceAudioOnly && this.inAudioOnlyMode !== true) {
			if (this.profile.source === 'camera') {
				filtered.unshift({
					type: 'enterAudioOnly',
					level: this.currentLevel,
				});
			}
		}

		if (
			this.coordinationOverride?.forceAudioOnly === true &&
			this.inAudioOnlyMode !== true &&
			this.profile.source === 'camera'
		) {
			filtered.unshift({
				type: 'enterAudioOnly',
				level: this.currentLevel,
			});
		}

		if (filtered.length === 0) {
			return [
				{
					type: 'noop',
					level: this.currentLevel,
					reason: 'filtered-by-policy',
				},
			];
		}

		return filtered;
	}

	private async executeActions(
		actions: PlannedQosAction[],
		signals: ReturnType<typeof deriveSignals>,
		nowMs: number,
		stateBefore: QosStateMachineContext['state'],
		stateAfter: QosStateMachineContext['state']
	): Promise<void> {
		const levelBeforeActions = this.currentLevel;
		const audioOnlyBeforeActions = this.inAudioOnlyMode;

		for (const action of actions) {
			let applied = false;

			if (action.type === 'noop') {
				applied = true;
			} else {
				try {
					const result = await this.actionExecutor.execute(action);

					applied = wasActionApplied(result);
				} catch (error) {
					this.lastSampleError = toError(error);
					applied = false;
				}
			}

			this.lastAction = { type: action.type, applied };

			if (applied) {
				this.currentLevel = action.level;

				if (action.type === 'enterAudioOnly') {
					this.inAudioOnlyMode = true;
				} else if (action.type === 'exitAudioOnly') {
					this.inAudioOnlyMode = false;
				}
			}

			this.trace.appendTraceEntry({
				seq: this.seq + 1,
				tsMs: nowMs,
				trackId: this.trackId,
				source: this.profile.source,
				stateBefore,
				stateAfter,
				reason: signals.reason,
				signals: {
					sendBitrateBps: signals.sendBitrateBps,
					targetBitrateBps: signals.targetBitrateBps,
					lossRate: signals.lossRate,
					rttMs: signals.rttMs,
					jitterMs: signals.jitterMs,
				},
				plannedAction: {
					type: action.type,
					level: action.level,
				},
				applied,
				probe: this.probeContext
					? {
							active: true,
							previousLevel: this.probeContext.previousLevel,
							targetLevel: this.probeContext.targetLevel,
						}
					: undefined,
			});
		}

		if (
			stateAfter === 'recovering' &&
			actions.some(action => action.type !== 'noop') &&
			(this.currentLevel < levelBeforeActions ||
				(audioOnlyBeforeActions && !this.inAudioOnlyMode))
		) {
			this.probeContext = beginProbe(
				levelBeforeActions,
				this.currentLevel,
				nowMs,
				audioOnlyBeforeActions,
				this.inAudioOnlyMode
			);
		}
	}

	private async rollbackProbe(
		signals: ReturnType<typeof deriveSignals>,
		nowMs: number,
		stateBefore: QosStateMachineContext['state']
	): Promise<void> {
		if (!this.probeContext) {
			return;
		}

		const revertActions = planActionsForLevel(
			{
				source: this.profile.source,
				profile: this.profile,
				state: 'congested',
				currentLevel: this.currentLevel,
				overrideClampLevel: undefined,
				inAudioOnlyMode: this.inAudioOnlyMode,
				signals,
			},
			this.probeContext.previousLevel
		);

		for (const action of revertActions) {
			const result = await this.actionExecutor.execute(action);
			const applied = wasActionApplied(result);

			if (applied) {
				this.currentLevel = action.level;
				if (action.type === 'enterAudioOnly') {
					this.inAudioOnlyMode = true;
				} else if (action.type === 'exitAudioOnly') {
					this.inAudioOnlyMode = false;
				}
			}
			this.trace.appendTraceEntry({
				seq: this.seq + 1,
				tsMs: nowMs,
				trackId: this.trackId,
				source: this.profile.source,
				stateBefore,
				stateAfter: 'congested',
				reason: signals.reason,
				signals: {
					sendBitrateBps: signals.sendBitrateBps,
					targetBitrateBps: signals.targetBitrateBps,
					lossRate: signals.lossRate,
					rttMs: signals.rttMs,
					jitterMs: signals.jitterMs,
				},
				plannedAction: {
					type: action.type,
					level: action.level,
				},
				applied,
				probe: {
					active: false,
					result: 'failed',
					previousLevel: this.probeContext.previousLevel,
					targetLevel: this.probeContext.targetLevel,
				},
			});
		}

		this.stateContext = {
			...this.stateContext,
			state: 'congested',
			enteredAtMs: nowMs,
			lastCongestedAtMs: nowMs,
		};
	}

	private async publishSnapshot(
		rawSnapshot: RawSenderSnapshot,
		signals: ReturnType<typeof deriveSignals>,
		quality: ClientQosSnapshot['peerState']['quality'],
		nowMs: number
	): Promise<void> {
		if (!this.signalChannel) {
			return;
		}

		const payload: ClientQosSnapshot = {
			schema: QOS_CLIENT_SCHEMA_V1,
			seq: ++this.seq,
			tsMs: nowMs,
			peerState: {
				mode: resolvePeerMode(this.inAudioOnlyMode, this.kind),
				quality,
				stale: false,
			},
			tracks: [
				{
					localTrackId: this.trackId,
					producerId: this.producerId,
					kind: this.kind,
					source: this.profile.source,
					state: this.stateContext.state,
					reason: signals.reason,
					quality,
					ladderLevel: this.currentLevel,
					signals: {
						sendBitrateBps: signals.sendBitrateBps,
						targetBitrateBps: signals.targetBitrateBps,
						lossRate: signals.lossRate,
						rttMs: signals.rttMs,
						jitterMs: signals.jitterMs,
						frameWidth: rawSnapshot.frameWidth,
						frameHeight: rawSnapshot.frameHeight,
						framesPerSecond: rawSnapshot.framesPerSecond,
						qualityLimitationReason: rawSnapshot.qualityLimitationReason,
					},
					lastAction: this.lastAction,
				},
			],
		};

		try {
			await this.signalChannel.publishSnapshot(payload);
		} catch (error) {
			this.lastSampleError = toError(error);
		}
	}
}
