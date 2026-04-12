import type { ClientQosSnapshot, QosOverride, QosPolicy } from './types';
export declare function parseClientQosSnapshot(payload: unknown): ClientQosSnapshot;
export declare function parseQosPolicy(payload: unknown): QosPolicy;
export declare function parseQosOverride(payload: unknown): QosOverride;
export declare function isClientQosSnapshot(value: unknown): value is ClientQosSnapshot;
export declare function isQosPolicy(value: unknown): value is QosPolicy;
export declare function isQosOverride(value: unknown): value is QosOverride;
export declare function serializeClientQosSnapshot(snapshot: ClientQosSnapshot): unknown;
//# sourceMappingURL=protocol.d.ts.map