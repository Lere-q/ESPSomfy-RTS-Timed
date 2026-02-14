#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

// ============================================
// SKYEO - Utility Functions
// Minimalistischer ESP32 Somfy Controller
// ============================================

#define SKYEO_VERSION "1.0.0"
#define SKYEO_NAME "Skyeo"

// Hilfsfunktionen für String-Verarbeitung
void trim(char* str);
void ltrim(char* str);
void rtrim(char* str);

// JSON Helper
String jsonEscape(const String& str);

// Zeit-Funktionen
String formatTime(uint32_t seconds);
String formatDuration(uint32_t seconds);

// Rolling Code Management
uint16_t nextRollingCode(uint8_t shadeId);
void saveRollingCode(uint8_t shadeId, uint16_t code);
uint16_t loadRollingCode(uint8_t shadeId);

// Watchdog Helper
void feedWatchdog();
void setupWatchdog(uint32_t timeout);

// Debug
#ifdef SKYEO_DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

#endif // UTILS_H
