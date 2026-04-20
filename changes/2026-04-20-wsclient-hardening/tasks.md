# Tasks

## 1. Define And Protect The Public API

- [x] 1.1 Privatize mutable `WsClient` internals and add explicit notification registration
  Files: `client/WsClient.h`, `client/WsClient.cpp`, `client/PlainClientApp.cpp`
  Verification: `cmake --build build --target mediasoup_tests -- -j2`

## 2. Fix WebSocket Wire Handling

- [x] 2.1 Make outgoing frame writes complete fully or fail explicitly
  Files: `client/WsClient.cpp`, affected tests
  Verification: `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

- [x] 2.2 Handle Ping/Pong and fragmented text frames correctly
  Files: `client/WsClient.cpp`, `tests/test_ws_client.cpp`
  Verification: `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

- [x] 2.3 Reject oversized incoming frames safely
  Files: `client/WsClient.cpp`, `tests/test_ws_client.cpp`
  Verification: `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

## 3. Guard Against Regression

- [x] 3.1 Extend focused `WsClient` unit coverage for the fixed paths
  Files: `tests/test_ws_client.cpp`
  Verification:
  - remote-close async failure remains covered
  - Ping triggers Pong
  - fragmented text response is reassembled
  - oversized frame closes/fails promptly

## 4. Delivery Gates

- [x] 4.1 Run the final targeted build/test set
  Verification:
  - `cmake --build build --target mediasoup_tests -- -j2`
  - `ctest --test-dir build --output-on-failure -R '^mediasoup_tests$'`

- [x] 4.2 Review `docs/aicoding/DELIVERY_CHECKLIST.md`
  Verification: all applicable items are resolved or recorded explicitly

## 5. Review

- [x] 5.1 Run self-review using `docs/aicoding/REVIEW.md`
  Verification: protocol correctness, failure handling, and client integration impact are covered
