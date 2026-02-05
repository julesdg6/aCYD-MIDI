Title: remote_display - resource leak when WiFi init succeeds but not connected

Affected files:
- src/remote_display.cpp

Description
-----------
If `initWiFi()` succeeds but `isWiFiConnected()` is false, `remote_display.cpp` returns without releasing previously allocated `server` and `ws` objects, leaking resources.

Suggested fix
-------------
In the failure path after the `isWiFiConnected()` check, stop/close the WebServer/WebSocket, delete/free the objects, and set pointers to `nullptr` before returning. Ensure any allocated resources are cleaned up on all error paths.

Severity: Medium

Notes
-----
Ensure compatibility with the rest of the remote display shutdown/restart logic; consider RAII-style wrappers or smart pointers where possible.