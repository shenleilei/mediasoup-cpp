import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const helperDir = path.dirname(new URL(import.meta.url).pathname);
const localBrowserCacheRoot = path.join(helperDir, '.browsers');
const bundledChromeRoot = path.join(helperDir, 'chrome');

function findLocalBrowserCacheCandidates() {
  const candidates = [];

  if (!fs.existsSync(localBrowserCacheRoot)) {
    return candidates;
  }

  const browserRoots = fs.readdirSync(localBrowserCacheRoot, { withFileTypes: true })
    .filter(entry => entry.isDirectory())
    .map(entry => path.join(localBrowserCacheRoot, entry.name));

  for (const browserRoot of browserRoots) {
    const nestedDirs = fs.readdirSync(browserRoot, { withFileTypes: true })
      .filter(entry => entry.isDirectory())
      .map(entry => path.join(browserRoot, entry.name));

    for (const nestedDir of nestedDirs) {
      const knownCandidates = [
        path.join(nestedDir, 'chrome-linux64', 'chrome'),
        path.join(nestedDir, 'chrome-linux', 'chrome'),
        path.join(nestedDir, 'headless_shell-linux64', 'headless_shell'),
        path.join(nestedDir, 'headless_shell-linux', 'headless_shell'),
      ];

      for (const candidate of knownCandidates) {
        if (fs.existsSync(candidate)) {
          candidates.push(candidate);
        }
      }
    }
  }

  return candidates;
}

function findBundledChromeCandidates() {
  const candidates = [];

  if (!fs.existsSync(bundledChromeRoot)) {
    return candidates;
  }

  const browserRoots = fs.readdirSync(bundledChromeRoot, { withFileTypes: true })
    .filter(entry => entry.isDirectory())
    .map(entry => path.join(bundledChromeRoot, entry.name));

  for (const browserRoot of browserRoots) {
    const knownCandidates = [
      path.join(browserRoot, 'chrome-linux64', 'chrome'),
      path.join(browserRoot, 'chrome-linux', 'chrome'),
      path.join(browserRoot, 'headless_shell-linux64', 'headless_shell'),
      path.join(browserRoot, 'headless_shell-linux', 'headless_shell'),
    ];

    for (const candidate of knownCandidates) {
      if (fs.existsSync(candidate)) {
        candidates.push(candidate);
      }
    }
  }

  return candidates;
}

const DEFAULT_CHROMIUM_CANDIDATES = [
  process.env.CHROME_BIN,
  ...findLocalBrowserCacheCandidates(),
  ...findBundledChromeCandidates(),
  '/usr/lib64/chromium-browser/headless_shell',
  '/usr/bin/chromium-browser',
  '/usr/bin/chromium',
  '/usr/bin/google-chrome',
  '/usr/bin/google-chrome-stable',
].filter(Boolean);

function looksLikeSnapWrapper(contents) {
  return contents.includes('/snap/bin/chromium') ||
    contents.includes('requires the chromium snap to be installed');
}

function isUsableBrowserExecutable(candidate) {
  if (!candidate || !fs.existsSync(candidate)) {
    return false;
  }

  try {
    const stat = fs.statSync(candidate);
    if (!stat.isFile()) {
      return false;
    }
  } catch {
    return false;
  }

  try {
    const contents = fs.readFileSync(candidate, 'utf8');
    if (looksLikeSnapWrapper(contents)) {
      return false;
    }
  } catch {
    // Binary executable, continue.
  }

  try {
    execFileSync(candidate, ['--version'], {
      stdio: ['ignore', 'pipe', 'pipe'],
      timeout: 3000,
    });
    return true;
  } catch {
    return false;
  }
}

export function resolveChromiumExecutable(candidates = DEFAULT_CHROMIUM_CANDIDATES) {
  for (const candidate of candidates) {
    if (isUsableBrowserExecutable(candidate)) {
      return candidate;
    }
  }

  throw new Error(
    `Usable browser was not found. Checked: ${candidates.join(', ')}. ` +
    `Set CHROME_BIN to a real Chromium/Chrome binary or install headless_shell.`
  );
}
