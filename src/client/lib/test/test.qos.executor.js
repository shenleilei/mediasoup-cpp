"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const executor_1 = require("../qos/executor");
function makeAction(level) {
    return {
        type: 'setEncodingParameters',
        level,
        encodingParameters: {
            maxBitrateBps: 100000 * (level + 1),
        },
    };
}
test('qos action executor applies action through sink', async () => {
    const sink = {
        execute: jest.fn(),
    };
    const executor = new executor_1.QosActionExecutor(sink);
    const action = makeAction(1);
    const result = await executor.execute(action);
    expect(sink.execute).toHaveBeenCalledWith(action);
    expect(result.status).toBe('applied');
    expect(result.action).toBe(action);
    expect(result.actionKey).toBe((0, executor_1.defaultQosActionKeyResolver)(action));
});
test('qos action executor skips noop actions', async () => {
    const sink = {
        execute: jest.fn(),
    };
    const executor = new executor_1.QosActionExecutor(sink);
    const result = await executor.execute({
        type: 'noop',
        level: 0,
    });
    expect(sink.execute).not.toHaveBeenCalled();
    expect(result.status).toBe('skipped_noop');
});
test('qos action executor skips idempotent repeat actions by default', async () => {
    const sink = {
        execute: jest.fn(),
    };
    const executor = new executor_1.QosActionExecutor(sink);
    const action = makeAction(2);
    await executor.execute(action);
    const result = await executor.execute(action);
    expect(sink.execute).toHaveBeenCalledTimes(1);
    expect(result.status).toBe('skipped_idempotent');
});
test('qos action executor can stop executeMany after failure', async () => {
    const sink = {
        execute: jest
            .fn()
            .mockResolvedValueOnce(undefined)
            .mockRejectedValueOnce(new Error('boom'))
            .mockResolvedValueOnce(undefined),
    };
    const executor = new executor_1.QosActionExecutor(sink, { stopOnError: true });
    const results = await executor.executeMany([
        makeAction(0),
        makeAction(1),
        makeAction(2),
    ]);
    expect(results).toHaveLength(2);
    expect(results[0]?.status).toBe('applied');
    expect(results[1]?.status).toBe('failed');
    expect(sink.execute).toHaveBeenCalledTimes(2);
});
