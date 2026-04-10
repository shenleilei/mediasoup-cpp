import {
	QosActionExecutor,
	defaultQosActionKeyResolver,
} from '../qos/executor';
import type { PlannedQosAction } from '../qos/types';

function makeAction(level: number): PlannedQosAction {
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
	const executor = new QosActionExecutor(sink);
	const action = makeAction(1);

	const result = await executor.execute(action);

	expect(sink.execute).toHaveBeenCalledWith(action);
	expect(result.status).toBe('applied');
	expect(result.action).toBe(action);
	expect(result.actionKey).toBe(defaultQosActionKeyResolver(action));
});

test('qos action executor skips noop actions', async () => {
	const sink = {
		execute: jest.fn(),
	};
	const executor = new QosActionExecutor(sink);

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
	const executor = new QosActionExecutor(sink);
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
	const executor = new QosActionExecutor(sink, { stopOnError: true });

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
