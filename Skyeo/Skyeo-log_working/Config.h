#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

// ============================================
// SKYEO - Configuration Management
// ============================================

#define MAX_SHADES 8
#define MAX_SCHEDULES 16

// Shade-Typen
enum ShadeType {
    SHADE_ROLLER = 0,    // Standard Rolladen
    SHADE_BLIND = 1,     // Jalousie (mit Tilt)
    SHADE_AWNING = 2     // Markise
};

// Shade-Struktur
struct ShadeConfig {
    uint8_t id;
    char name[32];
    ShadeType type;
    uint32_t remoteAddress;  // 24-Bit Somfy-Adresse
    uint16_t rollingCode;
    uint16_t upTime;         // Fahrzeit hoch (ms)
    uint16_t downTime;       // Fahrzeit runter (ms)
    uint16_t tiltTime;       // Kippzeit (für Jalousien)
    bool enabled;
};

// Zeitplan-Struktur
struct Schedule {
    uint32_t id;
    uint8_t shadeId;
    uint8_t hour;           // 0-23
    uint8_t minute;         // 0-59
    uint8_t days;           // Bitmaske: Bit 0=So, 1=Mo, ..., 6=Sa
    uint8_t command;        // 0=Up, 1=Down, 2=My, 3=Target
    uint8_t target;         // Position 0-100 (nur für Target)
    bool enabled;
    uint32_t lastRun;       // YYYYMMDD Format
};

// WiFi-Konfiguration
struct WiFiConfig {
    char ssid[64];
    char password[64];
    bool dhcp;
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    bool configured;
};

// System-Konfiguration
class Config {
public:
    static bool begin();
    static void end();
    
    // Shade Management
    static bool loadShade(uint8_t id, ShadeConfig& shade);
    static bool saveShade(const ShadeConfig& shade);
    static bool deleteShade(uint8_t id);
    static uint8_t getShadeCount();
    
    // Schedule Management
    static bool loadSchedule(uint8_t index, Schedule& schedule);
    static bool saveSchedule(uint8_t index, const Schedule& schedule);
    static bool deleteSchedule(uint8_t index);
    static uint8_t getScheduleCount();
    static int findFreeScheduleSlot();
    
    // WiFi
    static bool loadWiFi(WiFiConfig& wifi);
    static bool saveWiFi(const WiFiConfig& wifi);
    static bool hasWiFiConfig();
    
    // System
    static void setFirstRun(bool first);
    static bool isFirstRun();
    static void resetAll();
    
    // Rolling Code
    static uint16_t getRollingCode(uint8_t shadeId);
    static void setRollingCode(uint8_t shadeId, uint16_t code);
    
private:
    static Preferences prefs;
};

#endif // CONFIG_H
