# Tasks

1. [x] Define the shared FFmpeg adapter boundary
Outcome:
- change docs clearly separate FFmpeg adapters from media protocol helpers and runtime orchestration
Verification:
- `requirements.md` and `design.md` agree on ownership and scope

2. [x] Add the `common/ffmpeg` foundation
Outcome:
- shared wrappers exist for error handling, resource ownership, input/output formats, codecs, and bitstream filters
Verification:
- root build and standalone `client/` build compile with the new shared layer

3. [x] Migrate plain-client FFmpeg operations
Outcome:
- `client/main.cpp` no longer directly owns the main FFmpeg lifecycle management for input/codec/bsf resources
Verification:
- `plain-client` builds and targeted regressions remain green

4. [x] Migrate recorder FFmpeg operations
Outcome:
- recorder output/mux ownership and packet-write lifecycle move to shared wrappers while recorder policy stays local
Verification:
- recorder-related tests remain green

5. [x] Review remaining threaded-path raw FFmpeg ownership
Outcome:
- any leftover direct FFmpeg lifecycle management is either migrated or explicitly recorded as follow-up debt
Verification:
- change docs reflect final truth

## Remaining Direct FFmpeg Touches

- codec/context field configuration inside caller-owned encoder setup lambdas remains local by design
- recorder extradata construction (`av_mallocz` + codecpar fields) remains local because it is recorder-specific H264 mux policy, not generic FFmpeg lifecycle ownership
