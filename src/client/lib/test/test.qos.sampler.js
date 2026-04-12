"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const clock_1 = require("../qos/clock");
const sampler_1 = require("../qos/sampler");
function createSnapshot(timestampMs) {
    return {
        timestampMs,
        source: 'camera',
        kind: 'video',
    };
}
test('interval qos sampler samples once on demand', async () => {
    const provider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const sampler = new sampler_1.IntervalQosSampler(new clock_1.ManualQosClock(), provider, 1000);
    await expect(sampler.sampleOnce()).resolves.toEqual(createSnapshot(1000));
    expect(provider.getSnapshot).toHaveBeenCalledTimes(1);
});
test('interval qos sampler runs on manual clock intervals and stops cleanly', async () => {
    const clock = new clock_1.ManualQosClock();
    const snapshots = [createSnapshot(1000), createSnapshot(2000)];
    const provider = {
        getSnapshot: jest
            .fn()
            .mockResolvedValueOnce(snapshots[0])
            .mockResolvedValueOnce(snapshots[1]),
    };
    const onSample = jest.fn();
    const sampler = new sampler_1.IntervalQosSampler(clock, provider, 1000);
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
    const clock = new clock_1.ManualQosClock();
    let resolveSnapshot;
    const provider = {
        getSnapshot: jest.fn(() => new Promise(resolve => {
            resolveSnapshot = resolve;
        })),
    };
    const onSample = jest.fn();
    const sampler = new sampler_1.IntervalQosSampler(clock, provider, 1000);
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
    const clock = new clock_1.ManualQosClock();
    const provider = {
        getSnapshot: jest.fn(async () => {
            throw new Error('boom');
        }),
    };
    const onError = jest.fn();
    const sampler = new sampler_1.IntervalQosSampler(clock, provider, 1000);
    sampler.start(jest.fn(), onError);
    clock.advanceBy(1000);
    await Promise.resolve();
    await Promise.resolve();
    expect(onError).toHaveBeenCalledTimes(1);
    expect(onError.mock.calls[0]?.[0]).toBeInstanceOf(Error);
    expect(onError.mock.calls[0]?.[0]?.message).toBe('boom');
});
test('interval qos sampler updates interval while running', async () => {
    const clock = new clock_1.ManualQosClock();
    const provider = {
        getSnapshot: jest.fn(async () => createSnapshot(1000)),
    };
    const onSample = jest.fn();
    const sampler = new sampler_1.IntervalQosSampler(clock, provider, 1000);
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
