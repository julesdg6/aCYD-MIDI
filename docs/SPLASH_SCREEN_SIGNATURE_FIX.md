# Splash Screen Signature Fix

## Issue
Function signature mismatch between `showSplashScreen` declaration and definition was causing linker errors.

## Error Message
```
Undefined reference to `showSplashScreen(String const&)`
```

## Root Cause
The function signature must match exactly between the header declaration and the implementation definition. In C++, default parameter values should only appear in the function declaration, not in the definition.

## Solution

### Correct Implementation

**Header File** (`include/splash_screen.h`):
```cpp
void showSplashScreen(const String &status = String(), unsigned long delayMs = 800);
```

**Implementation File** (`src/splash_screen.cpp`):
```cpp
void showSplashScreen(const String &status, unsigned long delayMs) {
    // Implementation
}
```

### Key Points

1. **Default Parameters**: Default values (`= String()`, `= 800`) appear ONLY in the header declaration
2. **Parameter Types**: Must match exactly between declaration and definition
3. **Parameter Count**: Both declaration and definition must have the same number of parameters
4. **Const-ness**: The `const` qualifier and `&` reference must match

### Common Mistakes to Avoid

❌ **WRONG** - Defaults in implementation:
```cpp
// Header
void showSplashScreen(const String &status = String(), unsigned long delayMs = 800);

// Implementation (WRONG - defaults repeated)
void showSplashScreen(const String &status = String(), unsigned long delayMs = 800) {
```

❌ **WRONG** - Missing parameter:
```cpp
// Header
void showSplashScreen(const String &status = String(), unsigned long delayMs = 800);

// Implementation (WRONG - missing second parameter)
void showSplashScreen(const String &status) {
```

❌ **WRONG** - Type mismatch:
```cpp
// Header
void showSplashScreen(const String &status, unsigned long delayMs);

// Implementation (WRONG - missing const or reference)
void showSplashScreen(String status, unsigned long delayMs) {
```

### Usage Examples

All these calls are valid with the correct implementation:

```cpp
// Provide both parameters
showSplashScreen("Booting...", 400);

// Use default for status (empty String)
showSplashScreen(String(), 500);

// Could also use default delay (if no second arg)
// showSplashScreen("Starting");  // Valid; commented out as an example

// Could use both defaults (if no args)
// showSplashScreen();            // Valid; commented out as an example
```

Note: the commented examples above are valid call forms because `showSplashScreen` declares default parameters in the header; they are left commented to avoid accidental compilation in this sample.

## Verification

To verify the fix is correct:
1. Check that header and implementation parameter counts match
2. Check that parameter types match exactly (including const and &)
3. Ensure default values only appear in header
4. Confirm all call sites use appropriate number of arguments

## Build Test

The code should compile and link successfully for all build targets:
- `esp32-2432S028R`
- `esp32-2432S028Rv2`
- `esp32-3248S035C`
- `esp32-3248S035R`

## References

- C++ Standard: Default arguments should only appear in function declarations, not definitions
- Arduino String class documentation
- ESP32 build system documentation
