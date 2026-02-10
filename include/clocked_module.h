#ifndef CLOCKED_MODULE_H
#define CLOCKED_MODULE_H

#include <stdint.h>

/**
 * Clocked Module Base Class
 * 
 * Base class for all step-based sequencer and generative modules.
 * Modules derive from this class and implement timing-driven callbacks.
 * 
 * The ClockRuntime manages transport state and calls modules at the
 * correct step boundaries based on their ticksPerStep() configuration.
 */

// Step context passed to module callbacks
struct StepContext {
  uint32_t tick;          // Current MIDI-clock tick count (monotonic while running)
  uint16_t bpm_x10;       // Fixed point tempo (e.g. 1200 = 120.0 BPM)
  uint8_t  ppqn;          // Pulses per quarter note (24 for MIDI clock)
  uint32_t barIndex;      // Computed from time signature (default 4/4)
  uint32_t tickInBar;     // 0..ticksPerBar-1
  uint32_t stepIndex;     // Absolute step count (monotonic)
  uint16_t stepInBar;     // 0..stepsPerBar-1
  uint16_t ticksPerStep;  // Module's chosen ticks/step
  bool isBarStart;        // True if this step is at the start of a bar
  
  StepContext() : tick(0), bpm_x10(1200), ppqn(24), barIndex(0),
                  tickInBar(0), stepIndex(0), stepInBar(0),
                  ticksPerStep(6), isBarStart(false) {}
};

// Parameter IDs (common across modules, modules can extend)
enum ModuleParamId : uint16_t {
  PARAM_BPM = 0,
  PARAM_SWING = 1,
  PARAM_GATE_LENGTH = 2,
  PARAM_CHANNEL = 3,
  // Modules define their own starting from 100
  PARAM_MODULE_BASE = 100
};

/**
 * ClockedModule - Base class for all clocked modules
 */
class ClockedModule {
public:
  virtual ~ClockedModule() {}
  
  // ========== Identity ==========
  
  /**
   * Type identifier for factory registration
   * Must be unique across all module types
   */
  virtual const char* typeId() const = 0;
  
  /**
   * Human-readable display name
   */
  virtual const char* displayName() const = 0;
  
  // ========== Lifecycle ==========
  
  /**
   * Initialize module state
   * Called when module is created or loaded
   */
  virtual void init() = 0;
  
  /**
   * Reset module to initial state
   * Called when transport stops or user requests reset
   */
  virtual void reset() = 0;
  
  /**
   * Called when transport transitions to RUNNING state
   * Optional hook for modules to prepare for playback
   */
  virtual void onTransportStart() {}
  
  /**
   * Called when transport transitions out of RUNNING state
   * Optional hook for modules to clean up (e.g., note-offs)
   */
  virtual void onTransportStop() {}
  
  // ========== Timing Configuration ==========
  
  /**
   * Get the number of MIDI clock ticks per step for this module
   * Must divide 24 evenly (e.g., 6 for 1/16 notes, 12 for 1/8 notes)
   * Default: 6 ticks (1/16 note resolution)
   */
  virtual uint16_t ticksPerStep() const { return 6; }
  
  /**
   * Whether this module's playhead should advance while muted
   * Default: true (module stays in sync even when muted)
   */
  virtual bool advanceWhileMuted() const { return true; }
  
  // ========== Realtime Callback ==========
  
  /**
   * Called on each step boundary for this module
   * This is the primary callback where modules generate MIDI output
   * 
   * @param ctx Step context with timing information
   */
  virtual void onStep(const StepContext& ctx) = 0;
  
  // ========== Parameter Management ==========
  
  /**
   * Set a module parameter
   * @param paramId Parameter identifier
   * @param value Parameter value (interpretation depends on paramId)
   */
  virtual void setParam(uint16_t paramId, int32_t value) = 0;
  
  /**
   * Get a module parameter value
   * @param paramId Parameter identifier
   * @return Parameter value
   */
  virtual int32_t getParam(uint16_t paramId) const = 0;
  
  // ========== Persistence (Optional) ==========
  
  /**
   * Serialize module state
   * Override to save module-specific state
   * Format is module-defined
   */
  virtual void serialize(uint8_t* buffer, size_t maxSize, size_t& outSize) const {
    outSize = 0;
    (void)buffer;
    (void)maxSize;
  }
  
  /**
   * Deserialize module state
   * Override to restore module-specific state
   * @return true if successful
   */
  virtual bool deserialize(const uint8_t* buffer, size_t size) {
    (void)buffer;
    (void)size;
    return true;
  }
};

/**
 * Module Factory - for creating module instances by type ID
 * Modules register themselves with this factory
 */
class ModuleFactory {
public:
  using CreateFn = ClockedModule* (*)();
  
  static ModuleFactory& instance();
  
  // Register a module type
  void registerModule(const char* typeId, CreateFn createFn);
  
  // Create a module instance by type ID
  ClockedModule* create(const char* typeId);
  
  // Get list of registered type IDs
  const char** getTypeIds(size_t& outCount) const;
  
private:
  ModuleFactory() : count_(0) {}
  
  static constexpr size_t kMaxModuleTypes = 32;
  struct Entry {
    const char* typeId;
    CreateFn createFn;
  };
  
  Entry entries_[kMaxModuleTypes];
  size_t count_;
};

/**
 * Helper macro to register a module type
 * Use in module implementation file:
 * 
 * REGISTER_MODULE(MyModule, "my_module")
 */
#define REGISTER_MODULE(ClassName, TypeId) \
  namespace { \
    ClockedModule* create##ClassName() { return new ClassName(); } \
    struct ClassName##Registrar { \
      ClassName##Registrar() { \
        ModuleFactory::instance().registerModule(TypeId, create##ClassName); \
      } \
    }; \
    static ClassName##Registrar g_##ClassName##Registrar; \
  }

#endif // CLOCKED_MODULE_H
