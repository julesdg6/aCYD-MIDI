Title: BLESerial - preserve existing BLEServer callbacks (avoid clobbering)

Affected files:
- src/ble_serial.cpp
- include/ble_serial.h

Description
-----------
`BLESerial::begin()` calls `server->setCallbacks(new BLESerialServerCallbacks(this))`, which can clobber any existing server callbacks used elsewhere. This may break other components that rely on server-level events.

Suggested fixes
---------------
- Detect existing callbacks on the `BLEServer` before calling `setCallbacks()` and, if present, store them and delegate/connect them from the new `BLESerialServerCallbacks` (call the preserved callback methods inside `onConnect`/`onDisconnect`).
- Alternatively document that `BLESerial` requires exclusive ownership of server callbacks and ensure callers arrange for chaining if needed.

Severity: Low/Medium

Notes
-----
Chaining is the most robust approach but requires careful ownership/cleanup handling to avoid double-free or use-after-free when callbacks are replaced.