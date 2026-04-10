import { QosSignalChannel } from '../qos/adapters/signalChannel';

test('qos signal channel dispatches qosConnectionQuality notifications', () => {
	const channel = new QosSignalChannel(async () => undefined);
	const listener = jest.fn();

	channel.onConnectionQuality(listener);
	channel.handleMessage({
		method: 'qosConnectionQuality',
		data: {
			quality: 'poor',
			stale: false,
			lost: false,
			lastUpdatedMs: 1234,
		},
	});

	expect(listener).toHaveBeenCalledTimes(1);
	expect(listener).toHaveBeenCalledWith({
		quality: 'poor',
		stale: false,
		lost: false,
		lastUpdatedMs: 1234,
	});
});

test('qos signal channel dispatches qosRoomState notifications', () => {
	const channel = new QosSignalChannel(async () => undefined);
	const listener = jest.fn();

	channel.onRoomState(listener);
	channel.handleMessage({
		method: 'qosRoomState',
		data: {
			peerCount: 2,
			stalePeers: 0,
			poorPeers: 2,
			lostPeers: 0,
			quality: 'poor',
		},
	});

	expect(listener).toHaveBeenCalledTimes(1);
	expect(listener).toHaveBeenCalledWith({
		peerCount: 2,
		stalePeers: 0,
		poorPeers: 2,
		lostPeers: 0,
		quality: 'poor',
	});
});
