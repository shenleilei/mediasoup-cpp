# Bugfix Plan: Repository Review Follow-ups

## Background
The `Repository Review Report (2026-04-20)` identified several findings. The high priority ones have been resolved, but there are a few medium priority ones left:
1. Recorder timestamp conversion can overflow signed 32-bit delta on long sessions (Impact: recording corruption for >6h recordings).
2. Recording output filenames are only second-granularity (Impact: overwriting recordings during rapid reconnects).
3. Room routing trust boundary is weak: user-controlled `clientIp` is accepted directly (Impact: untrusted clients can spoof their origin to manipulate room placement).

## Requirements
- Fix the recorder timestamp conversion logic to prevent overflow using 64-bit integer arithmetic.
- Adjust the filename generation for recordings to be more unique (e.g. by incorporating milliseconds or random components).
- Ensure the signaling HTTP and WS layers do not blindly trust the `clientIp` parameter over the actual socket peer address unless specifically configured to trust a reverse proxy header (like `X-Forwarded-For`).
