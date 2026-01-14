// splash_screen.h
#pragma once
#include <Arduino.h>

// Show the splash screen with an optional status message.
// Passing an empty string (or omitting the argument) will show the default message.
void showSplashScreen(const String &status = String());
