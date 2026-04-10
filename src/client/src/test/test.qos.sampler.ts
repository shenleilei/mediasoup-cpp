import { ManualQosClock } from '../qos/clock';
import { IntervalQosSampler, type QosStatsProvider } from '../qos/sampler';

function createSnapshot(timestampMs: number) {
	return {
		timestampMs,
		source: 'camera' as const,
		kind: 'video' as const,
	};
}

test('interval qos sampler samples once on demand', async () => {
	const provider: QosStatsProvider = {
		getSnapshot: jest.fn(async () => createSnapshot(1000)),
	};
	const sampler = new IntervalQosSampler(new ManualQosClock(), provider, 1000);

	await expect(sampler.sampleOnce()).resolves.toEqual(createSnapshot(1000));
	expect(provider.getSnapshot).toHaveBeenCalledTimes(1);
});

test('interval qos sampler runs on manual clock intervals and stops cleanly', async () => {
	const clock = new ManualQosClock();
	const snapshots = [createSnapshot(1000), createSnapshot(2000)];
	const provider: QosStatsProvider = {
		getSnapshot: jest
			.fn<Promise<(typeof snapshots)[number]>, []>()
			.mockResolvedValueOnce(snapshots[0])
			.mockResolvedValueOnce(snapshots[1]),
	};
	const onSample = jest.fn();
	const sampler = new IntervalQosSampler(clock, provider, 1000);

	sampler.start(onSample);
	expect(sampler.running).toBe(true);

	clock.advanceBy(1000);
	await Promise.resolve();
	await Promise.resolve();

	clock.advanceBy(1000);
	await Promise.resolve();
	await Promise.resolve();

	expect(provider.getSnapshot).toHaveBeenCalledTimes(2);
	expect(onSample).toHaveBeenNthCalledWith(1, snapshots[0]);
	expect(onSample).toHaveBeenNthCalledWith(2, snapshots[1]);

	sampler.stop();
	expect(sampler.running).toBe(false);

	clock.advanceBy(2000);
	await Promise.resolve();
	expect(provider.getSnapshot).toHaveBeenCalledTimes(2);
});

test('interval qos sampler ignores overlapping ticks while sampling is in flight', async () => {
	const clock = new ManualQosClock();
	let resolveSnapshot:
		| ((value: ReturnType<typeof createSnapshot>) => void)
		| undefined;
	const provider: QosStatsProvider = {
		getSnapshot: jest.fn(
			() =>
				new Promise(resolve => {
					resolveSnapshot = resolve;
				})
		),
	};
	const onSample = jest.fn();
	const sampler = new IntervalQosSampler(clock, provider, 1000);

	sampler.start(onSample);

	clock.advanceBy(1000);
	clock.advanceBy(1000);

	expect(provider.getSnapshot).toHaveBeenCalledTimes(1);

	resolveSnapshot?.(createSnapshot(3000));
	await Promise.resolve();
	await Promise.resolve();

	expect(onSample).toHaveBeenCalledTimes(1);
	expect(onSample).toHaveBeenCalledWith(createSnapshot(3000));
});

test('interval qos sampler forwards provider errors', async () => {
	const clock = new ManualQosClock();
	const provider: QosStatsProvider = {
		getSnapshot: jest.fn(async () => {
			throw new Error('boom');
		}),
	};
	const onError = jest.fn();
	const sampler = new IntervalQosSampler(clock, provider, 1000);

	sampler.start(jest.fn(), onError);
	clock.advanceBy(1000);
	await Promise.resolve();
	await Promise.resolve();

	expect(onError).toHaveBeenCalledTimes(1);
	expect(onError.mock.calls[0]?.[0]).toBeInstanceOf(Error);
	expect(onError.mock.calls[0]?.[0]?.message).toBe('boom');
});

test('interval qos sampler updates interval while running', async () => {
	const clock = new ManualQosClock();
	const provider: QosStatsProvider = {
		getSnapshot: jest.fn(async () => createSnapshot(1000)),
	};
	const onSample = jest.fn();
	const sampler = new IntervalQosSampler(clock, provider, 1000);

	sampler.start(onSample);
	sampler.updateIntervalMs(2000);

	clock.advanceBy(1000);
	await Promise.resolve();
	expect(onSample).toHaveBeenCalledTimes(0);

	clock.advanceBy(1000);
	await Promise.resolve();
	await Promise.resolve();
	expect(onSample).toHaveBeenCalledTimes(1);
	expect(sampler.intervalMs).toBe(2000);
});
