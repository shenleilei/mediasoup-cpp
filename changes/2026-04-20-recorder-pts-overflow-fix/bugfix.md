# Bugfix: Recorder PTS overflow on long sessions

## Symptom

Long-running recordings may produce non-monotonic or collapsed PTS values after several hours.

## Reproduction

1. Start a recorder session and keep RTP timestamps advancing for long duration.
2. Once RTP delta from base crosses signed 32-bit boundary, current code casts `(ts - baseTs)` to `int32_t`.
3. The value becomes negative and is clamped to `0`, flattening PTS progression.

## Observed behavior

- PTS may reset to `0` or stop advancing after long duration.
- Mux timeline can become corrupted for long recordings.

## Expected behavior

- PTS arithmetic should remain valid across the full unsigned 32-bit RTP delta range.
- Existing wraparound handling (`uint32` modulo subtraction) should remain intact.

## Suspected scope

- `src/Recorder.cpp` audio and video PTS delta calculation paths.
- Targeted regression tests in `tests/test_qos_unit.cpp`.

## Known non-affected behavior

- Short recordings well below signed 32-bit threshold.
- Existing RTP wraparound near `uint32` boundary for short deltas.

## Acceptance criteria

- Audio/video PTS delta no longer uses signed 32-bit cast.
- Unit tests cover:
  - crossing `0x7fffffff` boundary remains positive;
  - wraparound delta remains correct.
- `mediasoup_qos_unit_tests` and `mediasoup_tests` pass.

## Regression expectations

- No behavior regression in existing recorder wraparound path.
