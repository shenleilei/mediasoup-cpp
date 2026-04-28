import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import {
  acquireNetemGuard,
  getNetemLockPath,
  preflightNetemGuard,
  releaseNetemGuard,
} from './netem_guard.mjs';

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

test('netem guard acquires and releases lock state', async () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const clearCalls = [];

  const guard = await acquireNetemGuard({
    label: 'acquire-release',
    lockRoot,
    waitTimeoutMs: 1000,
    pollMs: 10,
    onAcquire: () => clearCalls.push('acquire'),
    onRelease: () => clearCalls.push('release'),
  });

  const lockPath = getNetemLockPath({ lockRoot });
  assert.ok(fs.existsSync(lockPath));
  assert.equal(clearCalls[0], 'acquire');

  await releaseNetemGuard(guard);

  assert.equal(clearCalls.at(-1), 'release');
  assert.equal(fs.existsSync(lockPath), false);
  fs.rmSync(lockRoot, { recursive: true, force: true });
});

test('netem guard removes stale lock before reacquire', async () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const lockPath = getNetemLockPath({ lockRoot });
  fs.mkdirSync(lockPath, { recursive: true });
  fs.writeFileSync(
    path.join(lockPath, 'owner.json'),
    `${JSON.stringify({
      pid: 999999,
      label: 'stale-owner',
      acquiredAt: new Date(Date.now() - 60_000).toISOString(),
    })}\n`
  );

  const guard = await acquireNetemGuard({
    label: 'stale-recovery',
    lockRoot,
    waitTimeoutMs: 1000,
    pollMs: 10,
    staleAfterMs: 100,
    onAcquire: () => {},
    onRelease: () => {},
  });

  assert.ok(fs.existsSync(lockPath));
  await releaseNetemGuard(guard);
  fs.rmSync(lockRoot, { recursive: true, force: true });
});

test('netem guard serializes competing acquisitions', async () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const events = [];

  const first = await acquireNetemGuard({
    label: 'first',
    lockRoot,
    waitTimeoutMs: 1000,
    pollMs: 10,
    onAcquire: () => events.push('first-acquire'),
    onRelease: () => events.push('first-release'),
  });

  const secondPromise = acquireNetemGuard({
    label: 'second',
    lockRoot,
    waitTimeoutMs: 1000,
    pollMs: 10,
    onAcquire: () => events.push('second-acquire'),
    onRelease: () => events.push('second-release'),
  });

  await sleep(50);
  assert.deepEqual(events, ['first-acquire']);

  await releaseNetemGuard(first);
  const second = await secondPromise;
  assert.deepEqual(events, ['first-acquire', 'first-release', 'second-acquire']);

  await releaseNetemGuard(second);
  assert.deepEqual(events, ['first-acquire', 'first-release', 'second-acquire', 'second-release']);
  fs.rmSync(lockRoot, { recursive: true, force: true });
});

test('netem guard preflight clears stale lock immediately', () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const lockPath = getNetemLockPath({ lockRoot });
  fs.mkdirSync(lockPath, { recursive: true });
  fs.writeFileSync(
    path.join(lockPath, 'owner.json'),
    `${JSON.stringify({
      pid: 999999,
      label: 'stale-owner',
      acquiredAt: new Date(Date.now() - 60_000).toISOString(),
    })}\n`
  );

  const result = preflightNetemGuard({
    lockRoot,
    staleAfterMs: 100,
  });

  assert.equal(result.clearedStale, true);
  assert.equal(fs.existsSync(lockPath), false);
  fs.rmSync(lockRoot, { recursive: true, force: true });
});

test('netem guard preflight fails fast on live conflicting owner by default', () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const lockPath = getNetemLockPath({ lockRoot });
  fs.mkdirSync(lockPath, { recursive: true });
  fs.writeFileSync(
    path.join(lockPath, 'owner.json'),
    `${JSON.stringify({
      pid: process.pid,
      label: 'live-owner',
      acquiredAt: new Date().toISOString(),
    })}\n`
  );

  assert.throws(
    () => preflightNetemGuard({ lockRoot, staleAfterMs: 60_000 }),
    /loopback netem guard busy: .*owner=live-owner/
  );

  fs.rmSync(lockRoot, { recursive: true, force: true });
});

test('netem guard preflight can force-clear live owner explicitly', () => {
  const lockRoot = fs.mkdtempSync(path.join(os.tmpdir(), 'netem-guard-test-'));
  const lockPath = getNetemLockPath({ lockRoot });
  const clearCalls = [];
  fs.mkdirSync(lockPath, { recursive: true });
  fs.writeFileSync(
    path.join(lockPath, 'owner.json'),
    `${JSON.stringify({
      pid: process.pid,
      label: 'live-owner',
      acquiredAt: new Date().toISOString(),
    })}\n`
  );

  const result = preflightNetemGuard({
    lockRoot,
    forceClearLive: true,
    onClear: () => clearCalls.push('clear'),
  });

  assert.equal(result.clearedLive, true);
  assert.deepEqual(clearCalls, ['clear']);
  assert.equal(fs.existsSync(lockPath), false);
  fs.rmSync(lockRoot, { recursive: true, force: true });
});
