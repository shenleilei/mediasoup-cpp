import path from 'node:path';

import { archiveKnownArtifacts } from './report_artifacts.mjs';

export const CPP_CLIENT_REPORT_RELATIVE_PATHS = {
  archiveRoot: 'docs/archive/uplink-qos-cpp-client-runs',
  fullMatrixJson: 'docs/generated/uplink-qos-cpp-client-matrix-report.json',
  fullCaseMarkdown: 'docs/plain-client-qos-case-results.md',
  targetedMatrixJson: 'docs/generated/uplink-qos-cpp-client-matrix-report.targeted.json',
  targetedCaseMarkdown: 'docs/generated/plain-client-qos-case-results.targeted.md',
};

export function getCppClientRunTypeForSelectedCases(selectedCases) {
  return selectedCases ? 'targeted' : 'full';
}

export function getCppClientReportSetPaths(repoRoot, runType) {
  const full = runType !== 'targeted';
  const matrixJsonRelativePath = full
    ? CPP_CLIENT_REPORT_RELATIVE_PATHS.fullMatrixJson
    : CPP_CLIENT_REPORT_RELATIVE_PATHS.targetedMatrixJson;
  const caseMarkdownRelativePath = full
    ? CPP_CLIENT_REPORT_RELATIVE_PATHS.fullCaseMarkdown
    : CPP_CLIENT_REPORT_RELATIVE_PATHS.targetedCaseMarkdown;

  return {
    runType: full ? 'full' : 'targeted',
    matrixJsonPath: path.join(repoRoot, matrixJsonRelativePath),
    matrixJsonRelativePath,
    caseMarkdownPath: path.join(repoRoot, caseMarkdownRelativePath),
    caseMarkdownRelativePath,
    archiveRootPath: path.join(repoRoot, CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot),
  };
}

export function inferCppClientRunType(repoRoot, matrixReportPath) {
  const resolvedPath = path.resolve(matrixReportPath);
  const fullPath = path.join(repoRoot, CPP_CLIENT_REPORT_RELATIVE_PATHS.fullMatrixJson);
  const targetedPath = path.join(repoRoot, CPP_CLIENT_REPORT_RELATIVE_PATHS.targetedMatrixJson);

  if (resolvedPath === fullPath) {
    return 'full';
  }
  if (resolvedPath === targetedPath) {
    return 'targeted';
  }
  return null;
}

export function backupLatestCppClientReportSet(repoRoot, runType, metadata = {}) {
  const reportPaths = getCppClientReportSetPaths(repoRoot, runType);

  return archiveKnownArtifacts(
    repoRoot,
    [
      {
        path: reportPaths.matrixJsonPath,
        relativePath: reportPaths.matrixJsonRelativePath,
        kind: 'matrixJson',
        runType,
        archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
      },
      {
        path: reportPaths.caseMarkdownPath,
        relativePath: reportPaths.caseMarkdownRelativePath,
        kind: 'caseMarkdown',
        runType,
        archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
      },
    ],
    {
      runType,
      archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
      ...metadata,
    }
  );
}

export function archiveCurrentCppClientReportSet(
  repoRoot,
  { generatedAt, runType, selectedCaseIds, includeCaseMarkdown = false, sourceScript }
) {
  const reportPaths = getCppClientReportSetPaths(repoRoot, runType);
  const artifacts = [
    {
      path: reportPaths.matrixJsonPath,
      relativePath: reportPaths.matrixJsonRelativePath,
      kind: 'matrixJson',
      generatedAt,
      runType,
      sourceScript,
      archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
    },
  ];

  if (includeCaseMarkdown) {
    artifacts.push({
      path: reportPaths.caseMarkdownPath,
      relativePath: reportPaths.caseMarkdownRelativePath,
      kind: 'caseMarkdown',
      generatedAt,
      runType,
      sourceScript,
      archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
    });
  }

  return archiveKnownArtifacts(repoRoot, artifacts, {
    runType,
    selectedCaseIds,
    sourceScript,
    archiveRootRelativePath: CPP_CLIENT_REPORT_RELATIVE_PATHS.archiveRoot,
  });
}
