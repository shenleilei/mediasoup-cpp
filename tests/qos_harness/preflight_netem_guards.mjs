import {
  describeNetemGuard,
  preflightNetemGuard,
} from './netem_guard.mjs';

const args = process.argv.slice(2);
const ifaceArg = args.find(arg => arg.startsWith('--iface='));
const iface = ifaceArg ? ifaceArg.slice('--iface='.length) : 'lo';
const forceClearLive = args.includes('--force-clear-live');

try {
  const result = preflightNetemGuard({ iface, forceClearLive });
  if (result.clearedStale) {
    console.error(`[netem-preflight] cleared stale guard on ${iface}`);
  }
  if (result.clearedLive) {
    console.error(`[netem-preflight] force-cleared live guard on ${iface}`);
  }
  if (result.after?.exists) {
    console.error(`[netem-preflight] guard still present: ${describeNetemGuard(result.after)}`);
    process.exit(1);
  }
  console.error(`[netem-preflight] ${iface} ready`);
} catch (error) {
  console.error(`[netem-preflight] ${error.message}`);
  process.exit(1);
}
