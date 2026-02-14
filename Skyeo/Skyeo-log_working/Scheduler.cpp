#include "Scheduler.h"
#include "Somfy.h"
#include "Utils.h"

// ============================================
// SKYEO - Scheduler Implementation
// ============================================

uint32_t Scheduler::bootTime = 0;
uint32_t Scheduler::lastCheck = 0;
uint32_t Scheduler::checkCounter = 0;

void Scheduler::begin() {
    DEBUG_PRINTLN("[SKYEO] Scheduler initialisiert");
    lastCheck = millis();
    
    // Alle Zeitpläne laden
    uint8_t count = Config::getScheduleCount();
    DEBUG_PRINTF("[SKYEO] %d Zeitpläne geladen\n", count);
}

void Scheduler::loop() {
    uint32_t now = millis();
    
    // Prüfe nur einmal pro Minute (60000ms)
    if (now - lastCheck < 60000) return;
    lastCheck = now;
    checkCounter++;
    
    // Zeit aktualisieren
    uint8_t currentHour = getHour();
    uint8_t currentMinute = getMinute();
    
    DEBUG_PRINTF("[SKYEO] Scheduler Check: %02d:%02d (Tag %d)\n", 
                 currentHour, currentMinute, getDayOfWeek());
    
    // Alle Zeitpläne durchgehen
    for (uint8_t i = 0; i < MAX_SCHEDULES; i++) {
        Schedule sched;
        if (!Config::loadSchedule(i, sched)) continue;
        
        if (!sched.enabled) continue;
        
        // Prüfe Zeit
        if (sched.hour != currentHour || sched.minute != currentMinute) continue;
        
        // Prüfe Tag
        uint8_t today = getDayOfWeek();
        if (!(sched.days & (1 << today))) continue;
        
        // Prüfe ob heute bereits ausgeführt
        uint32_t todayDate = getTodayDate();
        if (sched.lastRun == todayDate) continue;
        
        // Ausführen!
        DEBUG_PRINTF("[SKYEO] Zeitplan ID %d ausgeführt: Shade %d, Cmd %d\n",
                     sched.id, sched.shadeId, sched.command);
        
        executeSchedule(sched);
        
        // Als heute ausgeführt markieren
        sched.lastRun = todayDate;
        Config::saveSchedule(i, sched);
    }
}

// ============================================
// Schedule Management
// ============================================

bool Scheduler::addSchedule(const Schedule& sched) {
    int index = Config::findFreeScheduleSlot();
    if (index < 0) {
        DEBUG_PRINTLN("[SKYEO] Kein freier Slot für Zeitplan!");
        return false;
    }
    
    Schedule s = sched;
    s.id = index;
    s.lastRun = 0;
    
    // Validierung
    if (s.hour > 23 || s.minute > 59) {
        DEBUG_PRINTLN("[SKYEO] Ungültige Zeit im Zeitplan!");
        return false;
    }
    
    if (s.days == 0) {
        DEBUG_PRINTLN("[SKYEO] Keine Tage im Zeitplan ausgewählt!");
        return false;
    }
    
    bool success = Config::saveSchedule(index, s);
    if (success) {
        DEBUG_PRINTF("[SKYEO] Zeitplan ID %d hinzugefügt\n", index);
    }
    return success;
}

bool Scheduler::updateSchedule(uint8_t index, const Schedule& sched) {
    if (index >= MAX_SCHEDULES) return false;
    
    Schedule existing;
    if (!Config::loadSchedule(index, existing)) return false;
    
    Schedule s = sched;
    s.id = index;
    s.lastRun = existing.lastRun; // Behalte lastRun bei
    
    return Config::saveSchedule(index, s);
}

bool Scheduler::deleteSchedule(uint8_t index) {
    if (index >= MAX_SCHEDULES) return false;
    Config::deleteSchedule(index);
    DEBUG_PRINTF("[SKYEO] Zeitplan ID %d gelöscht\n", index);
    return true;
}

bool Scheduler::getSchedule(uint8_t index, Schedule& sched) {
    if (index >= MAX_SCHEDULES) return false;
    return Config::loadSchedule(index, sched);
}

uint8_t Scheduler::getScheduleCount() {
    return Config::getScheduleCount();
}

// ============================================
// Time Management (relativ)
// ============================================

void Scheduler::setTime(uint32_t seconds) {
    bootTime = seconds;
    checkCounter = 0;
    DEBUG_PRINTF("[SKYEO] Zeit gesetzt: %s\n", formatTime(seconds).c_str());
}

uint32_t Scheduler::getTime() {
    // Zeit seit Setzen + vergangene Minuten seit Boot
    return bootTime + (checkCounter * 60) + ((millis() - lastCheck) / 1000);
}

uint8_t Scheduler::getHour() {
    uint32_t seconds = getTime();
    return (seconds / 3600) % 24;
}

uint8_t Scheduler::getMinute() {
    uint32_t seconds = getTime();
    return (seconds / 60) % 60;
}

uint8_t Scheduler::getDayOfWeek() {
    // Berechne Tag aus checkCounter (Minuten seit Start / 1440)
    // Tag 0 = Sonntag (wie tm_wday)
    uint32_t days = checkCounter / 1440; // 1440 Minuten = 1 Tag
    return days % 7;
}

uint32_t Scheduler::getTodayDate() {
    // YYYYMMDD Format - basierend auf checkCounter
    uint32_t days = checkCounter / 1440;
    // Simuliertes Datum: 2025-01-01 + days
    uint32_t year = 2025;
    uint32_t month = 1;
    uint32_t day = 1 + days;
    
    // Einfache Berechnung (nicht 100% korrekt für Monatswechsel, aber ausreichend)
    while (day > 31) {
        day -= 31;
        month++;
        if (month > 12) {
            month = 1;
            year++;
        }
    }
    
    return year * 10000 + month * 100 + day;
}

// ============================================
// Enable/Disable
// ============================================

void Scheduler::setEnabled(uint8_t index, bool enabled) {
    Schedule sched;
    if (!Config::loadSchedule(index, sched)) return;
    sched.enabled = enabled;
    Config::saveSchedule(index, sched);
}

bool Scheduler::isEnabled(uint8_t index) {
    Schedule sched;
    if (!Config::loadSchedule(index, sched)) return false;
    return sched.enabled;
}

// ============================================
// Execute Schedule
// ============================================

void Scheduler::executeSchedule(const Schedule& sched) {
    ShadeState* shade = Somfy.getShade(sched.shadeId);
    if (!shade) {
        DEBUG_PRINTF("[SKYEO] Shade %d nicht gefunden!\n", sched.shadeId);
        return;
    }
    
    switch (sched.command) {
        case SCHED_CMD_UP:
            Somfy.sendCommand(sched.shadeId, SOMFY_UP);
            break;
        case SCHED_CMD_DOWN:
            Somfy.sendCommand(sched.shadeId, SOMFY_DOWN);
            break;
        case SCHED_CMD_MY:
            Somfy.sendCommand(sched.shadeId, SOMFY_MY);
            break;
        case SCHED_CMD_TARGET:
            Somfy.sendPosition(sched.shadeId, sched.target);
            break;
        default:
            DEBUG_PRINTF("[SKYEO] Unbekannter Befehl: %d\n", sched.command);
    }
}
