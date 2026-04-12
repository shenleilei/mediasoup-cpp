import type { QosStatsProvider } from '../sampler';
import type { RawSenderSnapshot } from '../types';
import type { ProducerAdapter } from './producerAdapter';
export declare class ProducerSenderStatsProvider implements QosStatsProvider {
    private readonly adapter;
    constructor(adapter: ProducerAdapter);
    getSnapshot(): Promise<RawSenderSnapshot>;
}
//# sourceMappingURL=statsProvider.d.ts.map