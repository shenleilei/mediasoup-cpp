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

function sleepSync(ms) {
  Atomics.wait(new Int32Array(new SharedArrayBuffer(4)), 0, 0, ms);
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

function sameMetadata(left, right) {
  return JSON.stringify(left) === JSON.stringify(right);
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
  const state = inspectNetemGuard({ lockPath, staleAfterMs });
  if (!state.isStale) {
    return false;
  }

  const latestMetadata = safeReadJson(path.join(lockPath, 'owner.json'));
  if (!sameMetadata(latestMetadata, state.owner)) {
    return false;
  }

  fs.rmSync(lockPath, { recursive: true, force: true });
  return true;
}

function clearLockIfUnchanged(lockPath, owner, onClear) {
  const metadataPath = path.join(lockPath, 'owner.json');
  const latestMetadata = safeReadJson(metadataPath);
  if (!sameMetadata(latestMetadata, owner)) {
    return false;
  }

  onClear?.();
  fs.rmSync(lockPath, { recursive: true, force: true });
  return true;
}

function ownerLabel(owner) {
  return `owner=${owner?.label ?? 'unknown'} pid=${owner?.pid ?? 'unknown'} acquiredAt=${owner?.acquiredAt ?? 'unknown'}`;
}

function terminateOwnerProcessSync(pid) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return true;
  }
  if (pid === process.pid) {
    return true;
  }
  if (!isProcessAlive(pid)) {
    return true;
  }

  try {
    process.kill(pid, 'SIGTERM');
  } catch {}
  for (let attempt = 0; attempt < 15; ++attempt) {
    if (!isProcessAlive(pid)) {
      return true;
    }
    sleepSync(100);
  }

  try {
    process.kill(pid, 'SIGKILL');
  } catch {}
  for (let attempt = 0; attempt < 15; ++attempt) {
    if (!isProcessAlive(pid)) {
      return true;
    }
    sleepSync(100);
  }

  return !isProcessAlive(pid);
}

export function getNetemLockPath({ iface = 'lo', lockRoot = DEFAULT_LOCK_ROOT } = {}) {
  return path.join(lockRoot, `${iface}.lock`);
}

export function inspectNetemGuard({
  iface = 'lo',
  lockRoot = DEFAULT_LOCK_ROOT,
  lockPath = getNetemLockPath({ iface, lockRoot }),
  staleAfterMs = DEFAULT_STALE_AFTER_MS,
} = {}) {
  const metadataPath = path.join(lockPath, 'owner.json');
  if (!fs.existsSync(lockPath)) {
    return {
      iface,
      lockPath,
      metadataPath,
      exists: false,
      owner: null,
      ageMs: null,
      staleByAge: false,
      staleByPid: false,
      isStale: false,
    };
  }

  const owner = safeReadJson(metadataPath);
  const acquiredAtMs = Date.parse(owner?.acquiredAt ?? '');
  const ageMs = Number.isFinite(acquiredAtMs)
    ? Date.now() - acquiredAtMs
    : Number.POSITIVE_INFINITY;
  const staleByAge = ageMs >= staleAfterMs;
  const staleByPid = !isProcessAlive(owner?.pid);

  return {
    iface,
    lockPath,
    metadataPath,
    exists: true,
    owner,
    ageMs,
    staleByAge,
    staleByPid,
    isStale: staleByAge || staleByPid,
  };
}

export function describeNetemGuard(state) {
  if (!state?.exists) {
    return `iface=${state?.iface ?? 'lo'} status=free`;
  }

  return [
    `iface=${state.iface}`,
    ownerLabel(state.owner),
    `staleByAge=${state.staleByAge ? 1 : 0}`,
    `staleByPid=${state.staleByPid ? 1 : 0}`,
  ].join(' ');
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

export function sweepNetemGuard({
  iface = 'lo',
  lockRoot = DEFAULT_LOCK_ROOT,
  staleAfterMs = DEFAULT_STALE_AFTER_MS,
  forceClearLive = false,
  onClear = () => clearRootQdisc(iface),
} = {}) {
  let firstState = null;
  let clearedStale = false;
  let clearedLive = false;

  for (let attempt = 0; attempt < 4; ++attempt) {
    const state = inspectNetemGuard({ iface, lockRoot, staleAfterMs });
    if (!firstState) {
      firstState = state;
    }

    if (!state.exists) {
      return {
        before: firstState,
        after: state,
        clearedStale,
        clearedLive,
      };
    }

    if (state.isStale) {
      if (clearLockIfUnchanged(state.lockPath, state.owner, onClear)) {
        clearedStale = true;
      }
      continue;
    }

    if (forceClearLive) {
      if (!terminateOwnerProcessSync(state.owner?.pid)) {
        return {
          before: firstState,
          after: state,
          clearedStale,
          clearedLive,
          conflict: state,
        };
      }
      if (clearLockIfUnchanged(state.lockPath, state.owner, onClear)) {
        clearedLive = true;
      }
      continue;
    }

    return {
      before: firstState,
      after: state,
      clearedStale,
      clearedLive,
      conflict: state,
    };
  }

  const state = inspectNetemGuard({ iface, lockRoot, staleAfterMs });
  return {
    before: firstState ?? state,
    after: state,
    clearedStale,
    clearedLive,
    conflict: state.exists ? state : null,
  };
}

export function preflightNetemGuard(options = {}) {
  const result = sweepNetemGuard(options);
  if (result.conflict?.exists) {
    throw new Error(
      `loopback netem guard busy: ${describeNetemGuard(result.conflict)}`
    );
  }
  return result;
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
