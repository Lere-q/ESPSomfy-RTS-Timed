#include "Utils.h"
#include <Preferences.h>
#include <esp_task_wdt.h>

// ============================================
// SKYEO - Utility Functions Implementation
// ============================================

void ltrim(char* str) {
    if (!str) return;
    int s = 0, j, k = 0;
    int e = strlen(str);
    while (s < e && (str[s] == ' ' || str[s] == '\n' || str[s] == '\r' || str[s] == '\t' || str[s] == '"')) s++;
    if (s > 0) {
        for (j = s; j < e; j++) {
            str[k] = str[j];
            k++;
        }
        str[k] = '\0';
    }
}

void rtrim(char* str) {
    if (!str) return;
    int e = strlen(str) - 1;
    while (e >= 0 && (str[e] == ' ' || str[e] == '\n' || str[e] == '\r' || str[e] == '\t' || str[e] == '"')) {
        str[e] = '\0';
        e--;
    }
}

void trim(char* str) {
    ltrim(str);
    rtrim(str);
}

String jsonEscape(const String& str) {
    String result;
    result.reserve(str.length() + 10);
    for (size_t i = 0; i < str.length(); i++) {
        char c = str[i];
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

String formatTime(uint32_t seconds) {
    char buf[9];
    uint8_t h = seconds / 3600;
    uint8_t m = (seconds % 3600) / 60;
    uint8_t s = seconds % 60;
    sprintf(buf, "%02d:%02d:%02d", h, m, s);
    return String(buf);
}

String formatDuration(uint32_t seconds) {
    char buf[20];
    if (seconds < 60) {
        sprintf(buf, "%ds", seconds);
    } else if (seconds < 3600) {
        sprintf(buf, "%dm %ds", seconds / 60, seconds % 60);
    } else {
        sprintf(buf, "%dh %dm", seconds / 3600, (seconds % 3600) / 60);
    }
    return String(buf);
}

uint16_t nextRollingCode(uint8_t shadeId) {
    uint16_t code = loadRollingCode(shadeId);
    code++;
    if (code == 0) code = 1; // 0 nicht erlaubt
    saveRollingCode(shadeId, code);
    return code;
}

void saveRollingCode(uint8_t shadeId, uint16_t code) {
    Preferences prefs;
    char key[16];
    sprintf(key, "rc_%d", shadeId);
    prefs.begin("skyeo", false);
    prefs.putUShort(key, code);
    prefs.end();
}

uint16_t loadRollingCode(uint8_t shadeId) {
    Preferences prefs;
    char key[16];
    sprintf(key, "rc_%d", shadeId);
    prefs.begin("skyeo", true);
    uint16_t code = prefs.getUShort(key, 0);
    prefs.end();
    return code;
}

void feedWatchdog() {
    #ifdef CONFIG_ESP_TASK_WDT
    esp_task_wdt_reset();
    #endif
}

void setupWatchdog(uint32_t timeout) {
    #ifdef CONFIG_ESP_TASK_WDT
    esp_task_wdt_init(timeout / 1000, true);
    esp_task_wdt_add(NULL);
    #endif
}
