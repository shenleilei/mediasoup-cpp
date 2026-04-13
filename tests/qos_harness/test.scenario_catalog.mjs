import assert from 'node:assert/strict';
import test from 'node:test';

import {
  filterScenarioCatalog,
  loadScenarioCatalog,
} from './scenario_catalog.mjs';

test('default scenario selection keeps T9-T11 in the main gate', () => {
  const scenarios = loadScenarioCatalog();
  const selected = filterScenarioCatalog(scenarios);

  assert(selected.length > 0);
  assert(selected.every(caseDef => caseDef.extended !== true));
  assert(!selected.some(caseDef => caseDef.caseId === 'B4'));
  assert(!selected.some(caseDef => caseDef.caseId === 'B5'));
   assert(!selected.some(caseDef => caseDef.caseId === 'BW2'));
  assert(selected.some(caseDef => caseDef.caseId === 'T9'));
  assert(selected.some(caseDef => caseDef.caseId === 'T10'));
  assert(selected.some(caseDef => caseDef.caseId === 'T11'));
});

test('includeExtended selection keeps both default and remaining extended cases', () => {
  const scenarios = loadScenarioCatalog();
  const selected = filterScenarioCatalog(scenarios, { includeExtended: true });

  assert(selected.some(caseDef => caseDef.caseId === 'B2'));
  assert(selected.some(caseDef => caseDef.caseId === 'B4'));
  assert(selected.some(caseDef => caseDef.caseId === 'T9'));
});

test('explicit case selection can still target remaining extended cases without includeExtended', () => {
  const scenarios = loadScenarioCatalog();
  const selected = filterScenarioCatalog(scenarios, {
    selectedCaseIds: new Set(['B4', 'B5', 'BW2']),
  });

  assert.deepEqual(
    selected.map(caseDef => caseDef.caseId),
    ['B4', 'B5', 'BW2']
  );
});
