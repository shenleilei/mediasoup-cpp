import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

export const scenariosPath = path.join(__dirname, 'scenarios', 'sweep_cases.json');

export function loadScenarioCatalog() {
  return JSON.parse(fs.readFileSync(scenariosPath, 'utf8'));
}

export function filterScenarioCatalog(
  scenarios,
  { selectedCaseIds = null, includeExtended = false } = {}
) {
  const selectedIds = selectedCaseIds
    ? (selectedCaseIds instanceof Set ? selectedCaseIds : new Set(selectedCaseIds))
    : null;

  if (selectedIds && selectedIds.size > 0) {
    return scenarios.filter(caseDef => selectedIds.has(caseDef.caseId));
  }

  return scenarios.filter(caseDef => includeExtended || caseDef.extended !== true);
}
