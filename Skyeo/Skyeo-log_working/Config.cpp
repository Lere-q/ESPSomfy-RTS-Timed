#include "Config.h"

// ============================================
// SKYEO - Configuration Management Implementation
// ============================================

Preferences Config::prefs;

bool Config::begin() {
    // Preferences wird bei jedem Zugriff geöffnet/geschlossen
    return true;
}

void Config::end() {
    // Nothing to do
}

// ============================================
// Shade Management
// ============================================

bool Config::loadShade(uint8_t id, ShadeConfig& shade) {
    prefs.begin("skyeo", true);
    char key[16];
    sprintf(key, "shade_%d", id);
    
    if (!prefs.isKey(key)) {
        prefs.end();
        return false;
    }
    
    size_t len = prefs.getBytes(key, &shade, sizeof(ShadeConfig));
    prefs.end();
    
    return len == sizeof(ShadeConfig);
}

bool Config::saveShade(const ShadeConfig& shade) {
    prefs.begin("skyeo", false);
    char key[16];
    sprintf(key, "shade_%d", shade.id);
    
    size_t written = prefs.putBytes(key, &shade, sizeof(ShadeConfig));
    prefs.end();
    
    return written == sizeof(ShadeConfig);
}

bool Config::deleteShade(uint8_t id) {
    prefs.begin("skyeo", false);
    char key[16];
    sprintf(key, "shade_%d", id);
    prefs.remove(key);
    prefs.end();
    return true;
}

uint8_t Config::getShadeCount() {
    uint8_t count = 0;
    prefs.begin("skyeo", true);
    for (uint8_t i = 0; i < MAX_SHADES; i++) {
        char key[16];
        sprintf(key, "shade_%d", i);
        if (prefs.isKey(key)) count++;
    }
    prefs.end();
    return count;
}

// ============================================
// Schedule Management
// ============================================

bool Config::loadSchedule(uint8_t index, Schedule& schedule) {
    prefs.begin("skyeo", true);
    char key[16];
    sprintf(key, "sched_%d", index);
    
    if (!prefs.isKey(key)) {
        prefs.end();
        return false;
    }
    
    size_t len = prefs.getBytes(key, &schedule, sizeof(Schedule));
    prefs.end();
    
    return len == sizeof(Schedule);
}

bool Config::saveSchedule(uint8_t index, const Schedule& schedule) {
    prefs.begin("skyeo", false);
    char key[16];
    sprintf(key, "sched_%d", index);
    
    size_t written = prefs.putBytes(key, &schedule, sizeof(Schedule));
    prefs.end();
    
    return written == sizeof(Schedule);
}

bool Config::deleteSchedule(uint8_t index) {
    prefs.begin("skyeo", false);
    char key[16];
    sprintf(key, "sched_%d", index);
    prefs.remove(key);
    prefs.end();
    return true;
}

uint8_t Config::getScheduleCount() {
    uint8_t count = 0;
    prefs.begin("skyeo", true);
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        char key[16];
        sprintf(key, "sched_%d", i);
        if (prefs.isKey(key)) count++;
    }
    prefs.end();
    return count;
}

int Config::findFreeScheduleSlot() {
    prefs.begin("skyeo", true);
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        char key[16];
        sprintf(key, "sched_%d", i);
        if (!prefs.isKey(key)) {
            prefs.end();
            return i;
        }
    }
    prefs.end();
    return -1;
}

// ============================================
// WiFi Configuration
// ============================================

bool Config::loadWiFi(WiFiConfig& wifi) {
    prefs.begin("skyeo", true);
    
    if (!prefs.isKey("wifi_ssid")) {
        prefs.end();
        return false;
    }
    
    prefs.getString("wifi_ssid", wifi.ssid, sizeof(wifi.ssid));
    prefs.getString("wifi_pass", wifi.password, sizeof(wifi.password));
    wifi.dhcp = prefs.getBool("wifi_dhcp", true);
    wifi.configured = prefs.getBool("wifi_cfg", false);
    
    if (!wifi.dhcp) {
        wifi.ip.fromString(prefs.getString("wifi_ip", "0.0.0.0"));
        wifi.gateway.fromString(prefs.getString("wifi_gw", "0.0.0.0"));
        wifi.subnet.fromString(prefs.getString("wifi_sn", "255.255.255.0"));
    }
    
    prefs.end();
    return true;
}

bool Config::saveWiFi(const WiFiConfig& wifi) {
    prefs.begin("skyeo", false);
    
    prefs.putString("wifi_ssid", wifi.ssid);
    prefs.putString("wifi_pass", wifi.password);
    prefs.putBool("wifi_dhcp", wifi.dhcp);
    prefs.putBool("wifi_cfg", wifi.configured);
    
    if (!wifi.dhcp) {
        prefs.putString("wifi_ip", wifi.ip.toString());
        prefs.putString("wifi_gw", wifi.gateway.toString());
        prefs.putString("wifi_sn", wifi.subnet.toString());
    }
    
    prefs.end();
    return true;
}

bool Config::hasWiFiConfig() {
    prefs.begin("skyeo", true);
    bool has = prefs.getBool("wifi_cfg", false);
    prefs.end();
    return has;
}

// ============================================
// System
// ============================================

void Config::setFirstRun(bool first) {
    prefs.begin("skyeo", false);
    prefs.putBool("first_run", first);
    prefs.end();
}

bool Config::isFirstRun() {
    prefs.begin("skyeo", true);
    bool first = prefs.getBool("first_run", true);
    prefs.end();
    return first;
}

void Config::resetAll() {
    prefs.begin("skyeo", false);
    prefs.clear();
    prefs.end();
}

// ============================================
// Rolling Code
// ============================================

uint16_t Config::getRollingCode(uint8_t shadeId) {
    prefs.begin("skyeo", true);
    char key[16];
    sprintf(key, "rc_%d", shadeId);
    uint16_t code = prefs.getUShort(key, 0);
    prefs.end();
    return code;
}

void Config::setRollingCode(uint8_t shadeId, uint16_t code) {
    prefs.begin("skyeo", false);
    char key[16];
    sprintf(key, "rc_%d", shadeId);
    prefs.putUShort(key, code);
    prefs.end();
}
