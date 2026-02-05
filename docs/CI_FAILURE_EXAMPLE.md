# Example CI Failure Issue

This document shows what a CI failure issue created by the automated system will look like.

---

## 🔴 Automated CI Build Failure Report

**Workflow:** Build ESP32 firmware (PlatformIO)  
**Run ID:** [#8765432109](https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109)  
**Run Number:** #123  
**Triggered by:** @developer-username  
**Event:** push  
**Branch/Ref:** `refs/heads/main`  
**Commit:** `abc1234`  

---

### 📋 Failure Summary

```
====================================
CI BUILD FAILURE REPORT
====================================

Workflow: Build ESP32 firmware (PlatformIO)
Run ID: 8765432109
Run Number: 123
Triggered by: developer-username
Event: push
Ref: refs/heads/main
Commit: abc1234567890abcdef1234567890abcdef1234
Repository: julesdg6/aCYD-MIDI

====================================
FAILURE CONTEXT
====================================

The build job failed during execution.

Build Matrix Information:
- Multiple environments were being built
- Check the logs URL below for specific environment failures

====================================
LOGS AND ARTIFACTS
====================================

Full workflow logs available at:
https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109

To view detailed logs:
1. Visit the workflow run URL above
2. Click on the failed job(s) in the matrix
3. Review the build output for specific error messages

====================================
NEXT STEPS
====================================

1. Review the workflow logs at the URL above
2. Check for compilation errors, dependency issues, or test failures
3. Fix the identified issues
4. Re-run the workflow or push a new commit

====================================

NOTE: Full job logs will be fetched via GitHub API in the next step.
====================================
```

---

## Failed Job Details

### Job: build (esp32-2432S028Rv2)
- **Status:** completed
- **Conclusion:** failure
- **Started:** 2024-01-15T14:23:45Z
- **Completed:** 2024-01-15T14:28:12Z
- **Job URL:** https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109/job/1234567890

#### Full Job Logs:
```
2024-01-15T14:23:46.1234567Z ##[group]Run pio run -e esp32-2432S028Rv2
2024-01-15T14:23:46.1234567Z pio run -e esp32-2432S028Rv2
2024-01-15T14:23:46.1234567Z shell: /usr/bin/bash -e {0}
2024-01-15T14:23:46.1234567Z ##[endgroup]
2024-01-15T14:23:47.8901234Z Processing esp32-2432S028Rv2 (platform: espressif32; board: esp32dev; framework: arduino)
2024-01-15T14:23:48.5678901Z -------------------------------------------------------------------------
2024-01-15T14:23:49.2345678Z Verbose mode can be enabled via `-v, --verbose` option
2024-01-15T14:23:49.9012345Z CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32dev.html
2024-01-15T14:23:50.5678901Z PLATFORM: Espressif 32 (6.3.2) > Espressif ESP32 Dev Module
2024-01-15T14:23:51.2345678Z HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash
2024-01-15T14:23:51.9012345Z DEBUG: Current (cmsis-dap) External (cmsis-dap, esp-bridge, esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
2024-01-15T14:23:52.5678901Z PACKAGES: 
2024-01-15T14:23:53.2345678Z  - framework-arduinoespressif32 @ 3.20009.0 (2.0.9)
2024-01-15T14:23:53.9012345Z  - tool-esptoolpy @ 1.40501.0 (4.5.1)
2024-01-15T14:23:54.5678901Z  - toolchain-xtensa-esp32 @ 8.4.0+2021r2-patch5
2024-01-15T14:24:01.2345678Z LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
2024-01-15T14:24:02.9012345Z LDF Modes: Finder ~ chain, Compatibility ~ soft
2024-01-15T14:24:05.5678901Z Found 42 compatible libraries
2024-01-15T14:24:06.2345678Z Scanning dependencies...
2024-01-15T14:24:08.9012345Z Dependency Graph
2024-01-15T14:24:09.5678901Z |-- esp32_smartdisplay @ 2.0.6
2024-01-15T14:24:10.2345678Z |-- BLE @ 2.0.0
2024-01-15T14:25:45.8901234Z Building in release mode
2024-01-15T14:26:12.3456789Z Compiling .pio/build/esp32-2432S028Rv2/src/main.cpp.o
2024-01-15T14:27:34.5678901Z src/main.cpp: In function 'void setup()':
2024-01-15T14:27:34.5678902Z src/main.cpp:123:45: error: 'undefinedFunction' was not declared in this scope
2024-01-15T14:27:34.5678903Z   123 |     result = undefinedFunction(someParameter);
2024-01-15T14:27:34.5678904Z       |              ^~~~~~~~~~~~~~~~~
2024-01-15T14:27:34.5678905Z src/main.cpp:127:10: error: 'unknownVariable' was not declared in this scope
2024-01-15T14:27:34.5678906Z   127 |     if (unknownVariable > 0) {
2024-01-15T14:27:34.5678907Z       |         ^~~~~~~~~~~~~~~
2024-01-15T14:27:35.1234567Z *** [.pio/build/esp32-2432S028Rv2/src/main.cpp.o] Error 1
2024-01-15T14:28:12.7890123Z ===================================== [FAILED] Took 267.65 seconds =====================================
2024-01-15T14:28:12.7890124Z ##[error]Process completed with exit code 1.
```

### Job: build (esp32-2432S028Rv2-uart2)
- **Status:** completed
- **Conclusion:** failure
- **Started:** 2024-01-15T14:23:45Z
- **Completed:** 2024-01-15T14:28:10Z
- **Job URL:** https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109/job/1234567891

#### Full Job Logs:
```
[Similar compilation output showing the same errors...]
```

---

### 🔗 Useful Links

- **[View Full Workflow Run](https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109)** - Complete logs and details
- **[View Artifacts](https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109#artifacts)** - Build artifacts (if any)
- **[Repository](https://github.com/julesdg6/aCYD-MIDI)**

---

### 🛠️ How to Debug

1. Review the job logs above (if available)
2. Click the "View Full Workflow Run" link for complete logs
3. Expand the failed job(s) in the build matrix
4. Look for error messages, missing dependencies, or test failures
5. Fix the issues and re-run or push a new commit

---

*This issue was automatically created by the CI failure reporting system.*

---

## Labels

This issue will automatically have the following labels:
- `ci-failure` - Indicates this is a CI/CD failure
- `automated` - Indicates this was created automatically

---

## If Linked to Parent Issue

If this workflow was triggered with a parent issue number (e.g., issue #42), the following comment will be posted on that parent issue:

---

## 🔗 Automated CI Failure Sub-Issue Created

A build failure was detected during workflow execution.

**Failure Issue:** #456  
**Workflow Run:** [#123](https://github.com/julesdg6/aCYD-MIDI/actions/runs/8765432109)

Please review the [failure issue](https://github.com/julesdg6/aCYD-MIDI/issues/456) for complete logs and debugging information.

---

This creates a clear parent→child relationship for tracking related work.
