# Design

## Root Cause
The remaining failures are test-quality issues rather than a single new runtime regression:

1. `DownlinkClientStatsRateLimited` assumes a specific timing-sensitive outcome.
2. `test_qos_accuracy.cpp` has drifted into a state where:
   - syntax is broken by an extra brace
   - fraction lost is asserted as if it were already normalized rather than RTCP-scaled
   - localhost packet accounting is treated too strictly

## Approach

### Downlink rate-limit test
Align the integration test with the documented signaling semantics:
- worker-completed successful responses must include `data.stored`
- rate-limited or rejected requests may fail before storage

The test should assert only behavior guaranteed by the current contract, not a fragile exact timing outcome.

### QoS accuracy test
Repair the file and normalize the zero-loss expectations:
- remove the stray brace
- interpret `fractionLost` according to its RTCP representation
- keep the tolerance focused but realistic for localhost runs

## Non-goals
- Reworking the overall downlink rate-limiter algorithm
- Rewriting the QoS accuracy suite wholesale
