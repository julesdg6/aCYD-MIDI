#ifndef MODULE_DRUM_SEQ_CLOCKED_H
#define MODULE_DRUM_SEQ_CLOCKED_H

#include "clocked_module.h"

/**
 * Example Drum Sequencer using ClockedModule framework
 * 
 * This is a reference implementation demonstrating how to:
 * - Derive from ClockedModule
 * - Implement onStep() callback
 * - Use StepContext for timing
 * - Register with ModuleFactory
 * 
 * This module will eventually replace the existing module_sequencer_mode
 * once all modules are migrated to the new framework.
 */

class DrumSeqClocked : public ClockedModule {
public:
  static constexpr size_t kNumTracks = 4;
  static constexpr size_t kNumSteps = 16;
  
  DrumSeqClocked();
  virtual ~DrumSeqClocked();
  
  // ========== ClockedModule Interface ==========
  
  const char* typeId() const override { return "drum_seq_clocked"; }
  const char* displayName() const override { return "Drum Sequencer (Clocked)"; }
  
  void init() override;
  void reset() override;
  void onTransportStart() override;
  void onTransportStop() override;
  
  uint16_t ticksPerStep() const override { return 6; }  // 1/16 notes
  bool advanceWhileMuted() const override { return true; }
  
  void onStep(const StepContext& ctx) override;
  
  void setParam(uint16_t paramId, int32_t value) override;
  int32_t getParam(uint16_t paramId) const override;
  
  void serialize(uint8_t* buffer, size_t maxSize, size_t& outSize) const override;
  bool deserialize(const uint8_t* buffer, size_t size) override;
  
  // ========== Module-specific API ==========
  
  /**
   * Toggle a step in the pattern
   */
  void toggleStep(size_t track, size_t step);
  
  /**
   * Get pattern state
   */
  bool getStep(size_t track, size_t step) const;
  
  /**
   * Clear all patterns
   */
  void clearAll();
  
  /**
   * Get current step (for UI highlighting)
   */
  size_t getCurrentStep() const { return currentStep_; }
  
private:
  // Pattern storage
  bool pattern_[kNumTracks][kNumSteps];
  
  // Playback state
  size_t currentStep_;
  
  // MIDI notes for each track (TR-808 style)
  static constexpr uint8_t kDrumNotes[kNumTracks] = {36, 38, 42, 46};
  
  // Module parameters
  enum Params {
    PARAM_GATE_LEN = PARAM_MODULE_BASE,
    PARAM_VELOCITY
  };
  
  uint8_t gateLength_;  // In ticks
  uint8_t velocity_;
};

#endif // MODULE_DRUM_SEQ_CLOCKED_H
