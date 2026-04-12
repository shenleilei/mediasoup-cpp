import type { QosProfile, QosSource } from './types';
export declare function getDefaultCameraProfile(): QosProfile;
export declare function getDefaultScreenShareProfile(): QosProfile;
export declare function getDefaultAudioProfile(): QosProfile;
export declare function resolveProfile(source: QosSource, override?: Partial<QosProfile>): QosProfile;
export declare function resolveProfileByName(source: QosSource, name: string): QosProfile | undefined;
//# sourceMappingURL=profiles.d.ts.map