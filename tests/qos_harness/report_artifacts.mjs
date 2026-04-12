import fs from 'node:fs';
import path from 'node:path';

export const REPORT_RELATIVE_PATHS = {
  archiveRoot: 'docs/archive/uplink-qos-runs',
  fullMatrixJson: 'docs/generated/uplink-qos-matrix-report.json',
  fullCaseMarkdown: 'docs/uplink-qos-case-results.md',
  targetedMatrixJson: 'docs/generated/uplink-qos-matrix-report.targeted.json',
  targetedCaseMarkdown: 'docs/generated/uplink-qos-case-results.targeted.md',
};

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function readJson(filePath) {
  try {
    return JSON.parse(fs.readFileSync(filePath, 'utf8'));
  } catch {
    return null;
  }
}

function readMarkdownGeneratedAt(filePath) {
  try {
    const source = fs.readFileSync(filePath, 'utf8');
    const match = source.match(/生成时间：`([^`]+)`/);
    return match?.[1] ?? null;
  } catch {
    return null;
  }
}

function pushUnique(items, value) {
  if (!value) {
    return;
  }
  if (!items.includes(value)) {
    items.push(value);
  }
}

export function sanitizeArchiveTimestamp(timestamp) {
  return String(timestamp).replace(/:/g, '-');
}

export function getRunTypeForSelectedCases(selectedCases) {
  return selectedCases ? 'targeted' : 'full';
}

export function getRunTypeFromSelectedCaseIds(selectedCaseIds) {
  return !selectedCaseIds || selectedCaseIds === 'all' ? 'full' : 'targeted';
}

export function getReportSetPaths(repoRoot, runType) {
  const full = runType !== 'targeted';
  const matrixJsonRelativePath = full
    ? REPORT_RELATIVE_PATHS.fullMatrixJson
    : REPORT_RELATIVE_PATHS.targetedMatrixJson;
  const caseMarkdownRelativePath = full
    ? REPORT_RELATIVE_PATHS.fullCaseMarkdown
    : REPORT_RELATIVE_PATHS.targetedCaseMarkdown;

  return {
    runType: full ? 'full' : 'targeted',
    matrixJsonPath: path.join(repoRoot, matrixJsonRelativePath),
    matrixJsonRelativePath,
    caseMarkdownPath: path.join(repoRoot, caseMarkdownRelativePath),
    caseMarkdownRelativePath,
    archiveRootPath: path.join(repoRoot, REPORT_RELATIVE_PATHS.archiveRoot),
  };
}

export function readArtifactGeneratedAt(filePath) {
  if (!fs.existsSync(filePath)) {
    return null;
  }
  if (filePath.endsWith('.json')) {
    return readJson(filePath)?.generatedAt ?? null;
  }
  if (filePath.endsWith('.md')) {
    return readMarkdownGeneratedAt(filePath);
  }
  return null;
}

export function getArchiveDir(repoRoot, generatedAt) {
  return path.join(
    repoRoot,
    REPORT_RELATIVE_PATHS.archiveRoot,
    sanitizeArchiveTimestamp(generatedAt)
  );
}

function loadArchiveMetadata(metadataPath) {
  const existing = readJson(metadataPath);
  if (existing && typeof existing === 'object') {
    return existing;
  }
  return {};
}

export function archiveKnownArtifacts(repoRoot, artifacts, metadata = {}) {
  const groupedArtifacts = new Map();

  for (const artifact of artifacts) {
    if (!artifact?.path || !artifact?.relativePath) {
      continue;
    }
    if (!fs.existsSync(artifact.path)) {
      continue;
    }

    const generatedAt = artifact.generatedAt ?? readArtifactGeneratedAt(artifact.path);
    if (!generatedAt) {
      continue;
    }

    const group = groupedArtifacts.get(generatedAt) ?? [];
    group.push({ ...artifact, generatedAt });
    groupedArtifacts.set(generatedAt, group);
  }

  const archived = [];

  for (const [generatedAt, group] of groupedArtifacts.entries()) {
    const archiveDir = getArchiveDir(repoRoot, generatedAt);
    const metadataPath = path.join(archiveDir, 'metadata.json');
    const archiveMetadata = loadArchiveMetadata(metadataPath);
    const nextMetadata = {
      generatedAt,
      updatedAt: new Date().toISOString(),
      runTypes: Array.isArray(archiveMetadata.runTypes) ? archiveMetadata.runTypes : [],
      sourceScripts: Array.isArray(archiveMetadata.sourceScripts)
        ? archiveMetadata.sourceScripts
        : [],
      selectedCaseIds:
        metadata.selectedCaseIds ?? archiveMetadata.selectedCaseIds ?? null,
      artifacts:
        archiveMetadata.artifacts && typeof archiveMetadata.artifacts === 'object'
          ? archiveMetadata.artifacts
          : {},
    };

    ensureDir(archiveDir);

    for (const artifact of group) {
      const archivePath = path.join(archiveDir, artifact.relativePath);
      ensureDir(path.dirname(archivePath));
      fs.copyFileSync(artifact.path, archivePath);
      if (artifact.kind) {
        nextMetadata.artifacts[artifact.kind] = artifact.relativePath;
      }
      pushUnique(nextMetadata.runTypes, artifact.runType ?? metadata.runType);
      pushUnique(
        nextMetadata.sourceScripts,
        artifact.sourceScript ?? metadata.sourceScript
      );
    }

    fs.writeFileSync(metadataPath, `${JSON.stringify(nextMetadata, null, 2)}\n`);
    archived.push({ generatedAt, archiveDir });
  }

  return archived;
}

export function backupLatestReportSet(repoRoot, runType, metadata = {}) {
  const reportPaths = getReportSetPaths(repoRoot, runType);

  return archiveKnownArtifacts(
    repoRoot,
    [
      {
        path: reportPaths.matrixJsonPath,
        relativePath: reportPaths.matrixJsonRelativePath,
        kind: 'matrixJson',
        runType,
      },
      {
        path: reportPaths.caseMarkdownPath,
        relativePath: reportPaths.caseMarkdownRelativePath,
        kind: 'caseMarkdown',
        runType,
      },
    ],
    { runType, ...metadata }
  );
}

export function archiveCurrentReportSet(
  repoRoot,
  { generatedAt, runType, selectedCaseIds, includeMatrixJson = true, includeCaseMarkdown = true, sourceScript }
) {
  const reportPaths = getReportSetPaths(repoRoot, runType);
  const artifacts = [];

  if (includeMatrixJson) {
    artifacts.push({
      path: reportPaths.matrixJsonPath,
      relativePath: reportPaths.matrixJsonRelativePath,
      kind: 'matrixJson',
      generatedAt,
      runType,
      sourceScript,
    });
  }
  if (includeCaseMarkdown) {
    artifacts.push({
      path: reportPaths.caseMarkdownPath,
      relativePath: reportPaths.caseMarkdownRelativePath,
      kind: 'caseMarkdown',
      generatedAt,
      runType,
      sourceScript,
    });
  }

  return archiveKnownArtifacts(repoRoot, artifacts, {
    runType,
    selectedCaseIds,
    sourceScript,
  });
}

export function getRunTypeForMatrixInput(repoRoot, matrixReportPath) {
  const resolvedPath = path.resolve(matrixReportPath);
  const fullPath = path.join(repoRoot, REPORT_RELATIVE_PATHS.fullMatrixJson);
  const targetedPath = path.join(repoRoot, REPORT_RELATIVE_PATHS.targetedMatrixJson);

  if (resolvedPath === fullPath) {
    return 'full';
  }
  if (resolvedPath === targetedPath) {
    return 'targeted';
  }
  return null;
}

export function defaultCaseReportOutputPath(repoRoot, matrixReportPath) {
  const runType = getRunTypeForMatrixInput(repoRoot, matrixReportPath);
  if (runType) {
    return getReportSetPaths(repoRoot, runType).caseMarkdownPath;
  }

  return path.join(path.dirname(path.resolve(matrixReportPath)), 'uplink-qos-case-results.md');
}
