#ifndef CLOCK_TIMING_DEBUG_H
#define CLOCK_TIMING_DEBUG_H

#include "clock_manager.h"
#include <Arduino.h>

/**
 * Clock Timing Debug Utilities
 * 
 * Provides helper functions for debugging and monitoring MIDI clock timing accuracy.
 * These utilities can be called from the UI or serial console to verify timing performance.
 */

namespace ClockTimingDebug {

/**
 * Print detailed timing statistics to Serial
 * Call this periodically (e.g., every 5 seconds) to monitor timing accuracy
 */
inline void printTimingStats() {
  uint32_t minUs, maxUs, avgUs;
  clockManagerGetTimingStats(minUs, maxUs, avgUs);
  
  if (avgUs == 0) {
    Serial.println("[ClockTiming] No timing data available yet");
    return;
  }
  
  uint32_t jitterUs = maxUs - minUs;
  float jitterPercent = (avgUs > 0) ? (jitterUs * 100.0f / avgUs) : 0.0f;
  
  Serial.println("========== MIDI Clock Timing Statistics ==========");
  Serial.printf("Interval (µs): min=%u, max=%u, avg=%u\n", minUs, maxUs, avgUs);
  Serial.printf("Jitter: %u µs (%.2f%% of average)\n", jitterUs, jitterPercent);
  
  // Calculate expected interval for current BPM
  extern uint16_t sharedBPM;
  uint64_t expectedUs = (60000000ULL / sharedBPM) / CLOCK_TICKS_PER_QUARTER;
  int32_t errorUs = (int32_t)avgUs - (int32_t)expectedUs;
  float errorPercent = (expectedUs > 0) ? (errorUs * 100.0f / expectedUs) : 0.0f;
  
  Serial.printf("Expected interval: %llu µs @ %u BPM\n", expectedUs, sharedBPM);
  Serial.printf("Timing error: %+d µs (%+.3f%%)\n", errorUs, errorPercent);
  
  // Calculate effective BPM based on measured interval
  if (avgUs > 0) {
    float effectiveBPM = 60000000.0f / (avgUs * CLOCK_TICKS_PER_QUARTER);
    Serial.printf("Effective BPM: %.2f (target: %u)\n", effectiveBPM, sharedBPM);
  }
  
  Serial.println("==================================================");
}

/**
 * Calculate and return effective BPM based on timing statistics
 * Returns 0.0 if no timing data available
 */
inline float getEffectiveBPM() {
  uint32_t minUs, maxUs, avgUs;
  clockManagerGetTimingStats(minUs, maxUs, avgUs);
  
  if (avgUs == 0) {
    return 0.0f;
  }
  
  return 60000000.0f / (avgUs * CLOCK_TICKS_PER_QUARTER);
}

/**
 * Check if timing accuracy is within acceptable range
 * Returns true if jitter is <1% and timing error is <0.5%
 */
inline bool isTimingAccurate() {
  uint32_t minUs, maxUs, avgUs;
  clockManagerGetTimingStats(minUs, maxUs, avgUs);
  
  if (avgUs == 0) {
    return false;  // No data yet
  }
  
  extern uint16_t sharedBPM;
  uint64_t expectedUs = (60000000ULL / sharedBPM) / CLOCK_TICKS_PER_QUARTER;
  
  // Check jitter (<1% of average)
  uint32_t jitterUs = maxUs - minUs;
  if (jitterUs * 100 > avgUs) {
    return false;
  }
  
  // Check timing error (<0.5%)
  int32_t errorUs = abs((int32_t)avgUs - (int32_t)expectedUs);
  if (errorUs * 200 > (int32_t)expectedUs) {
    return false;
  }
  
  return true;
}

/**
 * Get a human-readable timing quality string
 * Returns: "Excellent", "Good", "Fair", "Poor", or "No Data"
 */
inline const char* getTimingQuality() {
  uint32_t minUs, maxUs, avgUs;
  clockManagerGetTimingStats(minUs, maxUs, avgUs);
  
  if (avgUs == 0) {
    return "No Data";
  }
  
  extern uint16_t sharedBPM;
  uint64_t expectedUs = (60000000ULL / sharedBPM) / CLOCK_TICKS_PER_QUARTER;
  
  uint32_t jitterUs = maxUs - minUs;
  float jitterPercent = (avgUs > 0) ? (jitterUs * 100.0f / avgUs) : 0.0f;
  
  int32_t errorUs = abs((int32_t)avgUs - (int32_t)expectedUs);
  float errorPercent = (expectedUs > 0) ? (errorUs * 100.0f / expectedUs) : 0.0f;
  
  // Excellent: <0.1% error, <0.5% jitter
  if (errorPercent < 0.1f && jitterPercent < 0.5f) {
    return "Excellent";
  }
  
  // Good: <0.5% error, <1% jitter
  if (errorPercent < 0.5f && jitterPercent < 1.0f) {
    return "Good";
  }
  
  // Fair: <2% error, <5% jitter
  if (errorPercent < 2.0f && jitterPercent < 5.0f) {
    return "Fair";
  }
  
  // Poor: everything else
  return "Poor";
}

/**
 * Example usage in loop() or UI update:
 * 
 * static uint32_t lastDebugTime = 0;
 * if (millis() - lastDebugTime > 5000) {
 *   lastDebugTime = millis();
 *   ClockTimingDebug::printTimingStats();
 *   
 *   const char* quality = ClockTimingDebug::getTimingQuality();
 *   Serial.printf("Clock quality: %s\n", quality);
 * }
 */

}  // namespace ClockTimingDebug

#endif  // CLOCK_TIMING_DEBUG_H
