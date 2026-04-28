# Design

1. Recorder Timestamp Conversion:
   - Investigate `src/Recorder.cpp` where audio/video RTP timestamp deltas are cast to `int32_t`.
   - Update variable types to `int64_t` or equivalent so that deltas do not overflow.

2. Recording Filenames:
   - Investigate `src/RoomRecordingHelpers.cpp`. Currently it relies on `epoch_seconds`.
   - Change the suffix generation to include milliseconds using `std::chrono::system_clock::now()` instead of just `epoch_seconds`, e.g., `epoch_milliseconds`.

3. Trust Boundary for IP routing:
   - For HTTP signaling (`src/SignalingServerHttp.cpp`), the `clientIp` URL query param should be ignored by default, or only accepted if a configuration flag allows it, instead prioritizing the actual `req->get_remote_address()`.
   - For WS signaling (`src/SignalingRequestDispatcher.h`), same rules apply. The fallback mechanism should be fixed to rely on actual socket remote address rather than allowing spoofing via `data.clientIp`.
