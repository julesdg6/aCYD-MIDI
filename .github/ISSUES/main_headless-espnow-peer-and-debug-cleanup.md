Title: main_headless - ESP-NOW peer handling, high-frequency debug prints, and BLE PIN handling

Affected files:
- src/main_headless.cpp

Description
-----------
Three issues affect `main_headless.cpp`:

1. After `esp_now_add_peer(&peerInfo)` fails, `espNowInitialized` is not set to false, causing subsequent `esp_now_send()` calls to try sending to an unregistered peer.
2. High-frequency `Serial.println("MIDI Clock")` calls inside clock tick paths cause blocking jitter and should be removed or gated by a debug flag.
3. BLE Security PIN is hardcoded with `setStaticPIN(123456)` — insecure for production.

Suggested fixes
---------------
- On `esp_now_add_peer()` failure, set `espNowInitialized = false` and avoid calling `esp_now_send()` or switch to a broadcast mode that does not require peer registration.
- Remove the per-tick `Serial.println` or wrap it behind a `kDebugSerial` flag or a tick counter to print sparsely (e.g., every N ticks).
- Replace the hardcoded BLE PIN with a configurable value: read from a config file or generate a runtime PIN (cryptographically random), and only log it under development mode. Document this change.

Severity: Medium

Notes
-----
These fixes reduce jitter and improve security; splitting into separate PRs (esp-now fail handling + debug print gating + PIN config) is recommended.