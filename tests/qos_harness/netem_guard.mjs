import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const tcPath = '/usr/sbin/tc';
const DEFAULT_LOCK_ROOT = path.join(os.tmpdir(), 'mediasoup-qos-netem-locks');
const DEFAULT_WAIT_TIMEOUT_MS = 30_000;
const DEFAULT_STALE_AFTER_MS = 15 * 60 * 1000;
const DEFAULT_POLL_MS = 200;
const SIGNAL_EXIT_CODES = {
  SIGHUP: 129,
  SIGINT: 130,
  SIGTERM: 143,
};

const activeGuards = new Set();
let cleanupHandlersInstalled = false;

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

function isProcessAlive(pid) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return false;
  }

  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

function safeReadJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch {
    return null;
  }
}

function buildMetadata(label) {
  return {
    pid: process.pid,
    label,
    acquiredAt: new Date().toISOString(),
  };
}

function releaseAllGuardsSync() {
  for (const guard of [...activeGuards]) {
    try {
      releaseNetemGuardSync(guard);
    } catch {}
  }
}

function installCleanupHandlers() {
  if (cleanupHandlersInstalled) {
    return;
  }

  cleanupHandlersInstalled = true;
  process.once('exit', () => {
    releaseAllGuardsSync();
  });

  for (const signal of Object.keys(SIGNAL_EXIT_CODES)) {
    process.once(signal, () => {
      releaseAllGuardsSync();
      process.exit(SIGNAL_EXIT_CODES[signal]);
    });
  }
}

function removeStaleLock(lockPath, staleAfterMs) {
  const metadataPath = path.join(lockPath, 'owner.json');
  const metadata = safeReadJson(metadataPath);
  const acquiredAtMs = Date.parse(metadata?.acquiredAt ?? '');
  const ageMs = Number.isFinite(acquiredAtMs) ? Date.now() - acquiredAtMs : Number.POSITIVE_INFINITY;
  const staleByAge = ageMs >= staleAfterMs;
  const staleByPid = !isProcessAlive(metadata?.pid);

  if (!staleByAge && !staleByPid) {
    return false;
  }

  const latestMetadata = safeReadJson(metadataPath);
  if (JSON.stringify(latestMetadata) !== JSON.stringify(metadata)) {
    return false;
  }

  fs.rmSync(lockPath, { recursive: true, force: true });
  return true;
}

export function getNetemLockPath({ iface = 'lo', lockRoot = DEFAULT_LOCK_ROOT } = {}) {
  return path.join(lockRoot, `${iface}.lock`);
}

export function clearRootQdisc(iface = 'lo') {
  try {
    execFileSync(tcPath, ['qdisc', 'del', 'dev', iface, 'root'], { stdio: 'ignore' });
  } catch {}
}

export function readRootQdisc(iface = 'lo') {
  try {
    return execFileSync(tcPath, ['qdisc', 'show', 'dev', iface], {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'ignore'],
    }).trim();
  } catch {
    return '';
  }
}

export async function acquireNetemGuard({
  label,
  iface = 'lo',
  lockRoot = DEFAULT_LOCK_ROOT,
  waitTimeoutMs = DEFAULT_WAIT_TIMEOUT_MS,
  staleAfterMs = DEFAULT_STALE_AFTER_MS,
  pollMs = DEFAULT_POLL_MS,
  onAcquire = () => clearRootQdisc(iface),
  onRelease = () => clearRootQdisc(iface),
} = {}) {
  const effectiveLabel = label || `pid-${process.pid}`;
  const lockPath = getNetemLockPath({ iface, lockRoot });
  const metadataPath = path.join(lockPath, 'owner.json');
  const deadline = Date.now() + waitTimeoutMs;

  installCleanupHandlers();
  fs.mkdirSync(lockRoot, { recursive: true });

  while (Date.now() < deadline) {
    try {
      fs.mkdirSync(lockPath);
      fs.writeFileSync(metadataPath, `${JSON.stringify(buildMetadata(effectiveLabel), null, 2)}\n`);

      const guard = {
        iface,
        label: effectiveLabel,
        lockPath,
        released: false,
        onRelease,
      };

      activeGuards.add(guard);
      onAcquire();
      return guard;
    } catch (error) {
      if (error?.code !== 'EEXIST') {
        throw error;
      }

      removeStaleLock(lockPath, staleAfterMs);
      await sleep(pollMs);
    }
  }

  const owner = safeReadJson(metadataPath);
  throw new Error(
    `timed out waiting for netem guard on ${iface}; owner=${owner?.label ?? 'unknown'} pid=${owner?.pid ?? 'unknown'} acquiredAt=${owner?.acquiredAt ?? 'unknown'}`
  );
}

export function releaseNetemGuardSync(guard) {
  if (!guard || guard.released) {
    return;
  }

  guard.released = true;
  try {
    guard.onRelease?.();
  } finally {
    activeGuards.delete(guard);
    fs.rmSync(guard.lockPath, { recursive: true, force: true });
  }
}

export async function releaseNetemGuard(guard) {
  releaseNetemGuardSync(guard);
}
