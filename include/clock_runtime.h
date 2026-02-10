#ifndef CLOCK_RUNTIME_H
#define CLOCK_RUNTIME_H

#include "clocked_module.h"
#include <stdint.h>
#include <stddef.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Clock Runtime - Centralized transport and clock management
 * 
 * Manages:
 * - Transport state machine (STOPPED/PENDING_START/RUNNING/PENDING_STOP)
 * - BPM (single source of truth)
 * - MIDI clock pulse generation (24 PPQN)
 * - Step boundary computation and module dispatch
 * - Start/stop quantization
 * - Swing timing
 */

// Transport states
enum class TransportState : uint8_t {
  STOPPED = 0,        // No clock, no advancement
  PENDING_START,      // Waiting for quantized start boundary
  RUNNING,            // Clock active, modules receiving steps
  PENDING_STOP        // Waiting for quantized stop boundary
};

// Quantization modes
enum class QuantizeMode : uint8_t {
  IMMEDIATE = 0,      // No quantization
  NEXT_STEP,          // Quantize to next step
  NEXT_BAR,           // Quantize to next bar (default for start)
  END_OF_BAR          // Quantize to end of current bar (default for stop)
};

// Time signature (currently fixed 4/4, provisioned for future)
struct TimeSignature {
  uint8_t numerator;
  uint8_t denominator;
  
  TimeSignature() : numerator(4), denominator(4) {}
};

/**
 * ClockRuntime - Main clock and transport controller
 */
class ClockRuntime {
public:
  static constexpr uint8_t kPPQN = 24;  // MIDI clock standard
  static constexpr uint8_t kDefaultTicksPerStep = 6;  // 1/16 note
  static constexpr uint16_t kMinBPM = 40;
  static constexpr uint16_t kMaxBPM = 300;
  
  ClockRuntime();
  ~ClockRuntime();
  
  // ========== Initialization ==========
  
  void init();
  void shutdown();
  
  // ========== Transport Control ==========
  
  /**
   * Request transport start
   * Will transition to PENDING_START, then RUNNING at quantized boundary
   */
  void requestStart();
  
  /**
   * Request transport stop
   * Will transition to PENDING_STOP, then STOPPED at quantized boundary
   */
  void requestStop();
  
  /**
   * Cancel pending start (e.g., user pressed stop during PENDING_START)
   */
  void cancelStart();
  
  /**
   * Immediate stop (no quantization)
   */
  void forceStop();
  
  /**
   * Reset transport and all modules to initial state
   */
  void reset();
  
  // ========== Transport State ==========
  
  TransportState getState() const { return state_; }
  bool isRunning() const { return state_ == TransportState::RUNNING; }
  bool isStopped() const { return state_ == TransportState::STOPPED; }
  bool isPending() const { 
    return state_ == TransportState::PENDING_START || 
           state_ == TransportState::PENDING_STOP; 
  }
  
  // ========== Timing ==========
  
  /**
   * Get current tick count (monotonic while running)
   */
  uint32_t getCurrentTick() const { return currentTick_; }
  
  /**
   * Get current BPM (fixed-point: bpm_x10 / 10.0)
   */
  uint16_t getBPM() const { return bpm_; }
  
  /**
   * Request BPM change
   * Runtime validates and applies immediately or at boundary
   */
  void requestTempo(uint16_t bpm);
  
  /**
   * Get swing amount (0-50%)
   */
  uint8_t getSwing() const { return swingPercent_; }
  
  /**
   * Set swing amount (0-50%)
   */
  void setSwing(uint8_t percent);
  
  // ========== Quantization Settings ==========
  
  void setStartQuantize(QuantizeMode mode) { startQuantize_ = mode; }
  void setStopQuantize(QuantizeMode mode) { stopQuantize_ = mode; }
  
  QuantizeMode getStartQuantize() const { return startQuantize_; }
  QuantizeMode getStopQuantize() const { return stopQuantize_; }
  
  // ========== Module Management ==========
  
  /**
   * Register a module to receive step callbacks
   * @return slot ID (or -1 on failure)
   */
  int registerModule(ClockedModule* module, uint8_t midiChannel = 0);
  
  /**
   * Unregister a module
   */
  void unregisterModule(int slotId);
  
  /**
   * Set module mute state
   */
  void setModuleMute(int slotId, bool mute);
  
  /**
   * Set module enabled state
   * Disabled modules don't advance or receive callbacks
   */
  void setModuleEnabled(int slotId, bool enabled);
  
  /**
   * Get module at slot
   */
  ClockedModule* getModule(int slotId);
  
  // ========== Update (called from clock task) ==========
  
  /**
   * Process clock tick (called from uClock callback or external clock)
   * @param tick Current tick count (if external) or 0 to auto-increment
   */
  void processTick(uint32_t tick = 0);
  
private:
  // Slot structure (embedded in runtime for now, will extract to SlotEngine later)
  struct Slot {
    ClockedModule* module;
    uint8_t midiChannel;
    bool mute;
    bool enabled;
    uint32_t lastStepTick;
    uint32_t stepIndex;
    
    Slot() : module(nullptr), midiChannel(0), mute(false), enabled(true),
             lastStepTick(0), stepIndex(0) {}
  };
  
  static constexpr size_t kMaxSlots = 8;
  
  // State
  TransportState state_;
  uint32_t currentTick_;
  uint32_t startPendingAtTick_;
  uint32_t stopPendingAtTick_;
  uint16_t bpm_;
  uint8_t swingPercent_;
  TimeSignature timeSignature_;
  QuantizeMode startQuantize_;
  QuantizeMode stopQuantize_;
  
  // Slots
  Slot slots_[kMaxSlots];
  size_t slotCount_;
  SemaphoreHandle_t slotsMutex_;
  
  // Internal methods
  void transitionToRunning();
  void transitionToStopped();
  bool shouldStartNow(uint32_t tick);
  bool shouldStopNow(uint32_t tick);
  void dispatchStepToModules(uint32_t tick);
  void sendTransportMessage(TransportState newState);
  uint32_t getTicksPerBar() const;
  bool isBarStart(uint32_t tick) const;
  bool isStepBoundary(uint32_t tick, uint16_t ticksPerStep) const;
  uint32_t applySwing(uint32_t tick, uint16_t ticksPerStep) const;
};

// Global instance
extern ClockRuntime clockRuntime;

#endif // CLOCK_RUNTIME_H
