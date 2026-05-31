import fs from 'node:fs';
import path from 'node:path';

export const MAX_ARCHIVED_REPORT_RUNS = 100;

function isTimestampedArchiveDir(dirent) {
  return (
    dirent?.isDirectory?.() &&
    /^\d{4}-\d{2}-\d{2}T\d{2}[-:]\d{2}[-:]\d{2}/.test(dirent.name)
  );
}

function listTimestampedArchiveDirs(archiveRootPath) {
  if (!fs.existsSync(archiveRootPath)) {
    return [];
  }

  return fs
    .readdirSync(archiveRootPath, { withFileTypes: true })
    .filter(isTimestampedArchiveDir)
    .map(dirent => dirent.name)
    .sort();
}

export function pruneTimestampedArchiveDirs(
  archiveRootPath,
  maxRuns = MAX_ARCHIVED_REPORT_RUNS
) {
  if (!Number.isFinite(maxRuns) || maxRuns < 1) {
    return [];
  }

  const dirs = listTimestampedArchiveDirs(archiveRootPath);
  const toDelete = dirs.slice(0, Math.max(0, dirs.length - maxRuns));

  for (const name of toDelete) {
    fs.rmSync(path.join(archiveRootPath, name), { recursive: true, force: true });
  }

  return toDelete;
}
