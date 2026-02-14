#include "Scheduler.h"
#include "Somfy.h"
#include "ConfigSettings.h"
#include <LittleFS.h>

// Externe Deklaration der globalen somfy Instanz
extern SomfyShadeController somfy;

SchedulerClass scheduler;

// ========== KONSTRUKTOR ==========
SchedulerClass::SchedulerClass() {
    lastCheck = 0;
    nextId = 1;
}

// ========== INITIALISIERUNG ==========
void SchedulerClass::begin() {
    Serial.println("========================================");
    Serial.println("Scheduler: Initialisiere...");
    loadFromFile();
    Serial.printf("Scheduler: %d Zeitpläne geladen\n", schedules.size());
    printAllSchedules();
    Serial.println("========================================");
}

// ========== HAUPT-LOOP ==========
void SchedulerClass::loop() {
    time_t now;
    time(&now);
    
    // Prüfe nur einmal pro Minute (60 Sekunden)
    if (now - lastCheck >= 60) {
        lastCheck = now;
        checkSchedules();
    }
}

// ========== SCHEDULE-PRÜFUNG ==========
void SchedulerClass::checkSchedules() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    uint8_t currentDay = timeinfo.tm_wday;      // 0=So, 1=Mo, ..., 6=Sa
    uint8_t currentHour = timeinfo.tm_hour;     // 0-23
    uint8_t currentMinute = timeinfo.tm_min;    // 0-59
    
    // FIX: Berechne heutiges Datum im YYYYMMDD Format
    uint32_t today = getTodayDate(&timeinfo);
    
    Serial.printf("Scheduler Check: %02d:%02d, Tag=%d (0=So), Datum=%u\n",
                  currentHour, currentMinute, currentDay, today);
    
    for (auto& schedule : schedules) {
        // Skip deaktivierte Zeitpläne
        if (!schedule.enabled) {
            continue;
        }
        
        // FIX: Prüfe Wochentag mit Bitmask
        bool dayMatches = (schedule.days & (1 << currentDay)) != 0;
        if (!dayMatches) {
            continue;
        }
        
        // Prüfe Uhrzeit
        bool timeMatches = (schedule.hour == currentHour && schedule.minute == currentMinute);
        if (!timeMatches) {
            continue;
        }
        
        // FIX: Prüfe ob heute bereits ausgeführt
        if (schedule.lastRun == today) {
            Serial.printf("  → Zeitplan ID %u bereits heute ausgeführt\n", schedule.id);
            continue;
        }
        
        // ✓ Alle Bedingungen erfüllt - Ausführen!
        Serial.println("========================================");
        Serial.printf(">>> TRIGGER: Zeitplan ID %u <<<\n", schedule.id);
        Serial.printf("    Shade: %s (ID %d)\n", schedule.shadeName, schedule.shadeId);
        Serial.printf("    Command: %s\n", schedule.command);
        Serial.printf("    Zeit: %02d:%02d\n", schedule.hour, schedule.minute);
        Serial.printf("    Tag: %d, Bitmask: 0x%02X\n", currentDay, schedule.days);
        Serial.println("========================================");
        
        executeSchedule(schedule);
        
        // FIX: Markiere als heute ausgeführt
        schedule.lastRun = today;
        save();
    }
}

// ========== HEUTE-DATUM BERECHNEN (FIX) ==========
uint32_t SchedulerClass::getTodayDate(struct tm* timeinfo) {
    // Format: YYYYMMDD
    // Beispiel: 9. Februar 2025 → 20250209
    uint32_t year = timeinfo->tm_year + 1900;  // tm_year ist Jahre seit 1900
    uint32_t month = timeinfo->tm_mon + 1;     // tm_mon ist 0-11
    uint32_t day = timeinfo->tm_mday;          // tm_mday ist 1-31
    
    return year * 10000 + month * 100 + day;
}

// ========== SOLLTE AUSGEFÜHRT WERDEN (HILFSFUNKTION) ==========
bool SchedulerClass::shouldRun(Schedule& schedule, struct tm* timeinfo) {
    if (!schedule.enabled) return false;
    
    uint8_t currentDay = timeinfo->tm_wday;
    uint8_t currentHour = timeinfo->tm_hour;
    uint8_t currentMinute = timeinfo->tm_min;
    
    // Prüfe Tag
    if (!(schedule.days & (1 << currentDay))) return false;
    
    // Prüfe Zeit
    if (schedule.hour != currentHour || schedule.minute != currentMinute) return false;
    
    // Prüfe ob heute schon ausgeführt
    uint32_t today = getTodayDate(timeinfo);
    if (schedule.lastRun == today) return false;
    
    return true;
}

// ========== SCHEDULE AUSFÜHREN (ORIGINAL-LOGIK BEIBEHALTEN) ==========
void SchedulerClass::executeSchedule(Schedule& schedule) {
    // 1. Shade holen
    SomfyShade* shade = somfy.getShadeById(schedule.shadeId);
    if (!shade) {
        Serial.printf(
            "Scheduler: FEHLER - Shade %d nicht gefunden!\n",
            schedule.shadeId
        );
        return;
    }

    // 2. String → somfy_commands
    somfy_commands cmd = translateSomfyCommand(String(schedule.command));

    // 3. Logging
    Serial.printf(
        "Scheduler: Sende %s an Shade %d (%s)\n",
        translateSomfyCommand(cmd).c_str(),
        schedule.shadeId,
        shade->name
    );

    // 4. ✅ RICHTIGER SEND-AUFRUF
    shade->sendCommand(
        cmd,
        1   // repeat = 1 → stabiler RTS-Tastendruck
    );
}

// ========== SCHEDULE HINZUFÜGEN ==========
bool SchedulerClass::addSchedule(Schedule& schedule) {
    if (schedules.size() >= MAX_SCHEDULES) {
        Serial.println("Scheduler: FEHLER - Maximale Anzahl erreicht!");
        return false;
    }
    
    // Setze automatisch generierte ID
    schedule.id = nextId++;
    schedule.lastRun = 0;  // Noch nie ausgeführt
    
    // Validierung
    if (schedule.hour > 23) {
        Serial.printf("Scheduler: FEHLER - Ungültige Stunde: %d\n", schedule.hour);
        return false;
    }
    if (schedule.minute > 59) {
        Serial.printf("Scheduler: FEHLER - Ungültige Minute: %d\n", schedule.minute);
        return false;
    }
    if (schedule.days == 0) {
        Serial.println("Scheduler: WARNUNG - Keine Tage ausgewählt!");
    }
    
    schedules.push_back(schedule);
    
    Serial.println("========================================");
    Serial.println("Scheduler: Neuer Zeitplan hinzugefügt:");
    printSchedule(schedule);
    Serial.println("========================================");
    
    save();
    return true;
}

// ========== SCHEDULE AKTUALISIEREN ==========
bool SchedulerClass::updateSchedule(uint32_t id, Schedule& schedule) {
    for (auto& s : schedules) {
        if (s.id == id) {
            // Behalte ID und lastRun
            schedule.id = id;
            uint32_t oldLastRun = s.lastRun;
            
            s = schedule;
            s.lastRun = oldLastRun;  // Behalte letzten Ausführungszeitpunkt
            
            Serial.printf("Scheduler: Zeitplan ID %u aktualisiert\n", id);
            printSchedule(s);
            
            save();
            return true;
        }
    }
    
    Serial.printf("Scheduler: FEHLER - Zeitplan ID %u nicht gefunden\n", id);
    return false;
}

// ========== SCHEDULE LÖSCHEN ==========
bool SchedulerClass::deleteSchedule(uint32_t id) {
    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) {
            Serial.printf("Scheduler: Lösche Zeitplan ID %u (%s)\n", 
                         id, it->shadeName);
            schedules.erase(it);
            save();
            return true;
        }
    }
    
    Serial.printf("Scheduler: FEHLER - Zeitplan ID %u nicht gefunden\n", id);
    return false;
}

// ========== SCHEDULE ABRUFEN ==========
Schedule* SchedulerClass::getSchedule(uint32_t id) {
    for (auto& s : schedules) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

// ========== ALLE SCHEDULES ABRUFEN ==========
std::vector<Schedule> SchedulerClass::getAllSchedules() {
    return schedules;
}

// ========== AKTIVIEREN/DEAKTIVIEREN ==========
void SchedulerClass::setEnabled(uint32_t id, bool enabled) {
    Schedule* s = getSchedule(id);
    if (s) {
        s->enabled = enabled;
        Serial.printf("Scheduler: Zeitplan ID %u %s\n", 
                     id, enabled ? "AKTIVIERT" : "DEAKTIVIERT");
        save();
    }
}

bool SchedulerClass::isEnabled(uint32_t id) {
    Schedule* s = getSchedule(id);
    return s ? s->enabled : false;
}

// ========== JSON SERIALISIERUNG ==========
String SchedulerClass::toJSON() {
    DynamicJsonDocument doc(8192);
    JsonArray arr = doc.to<JsonArray>();
    
    for (auto& s : schedules) {
        JsonObject obj = arr.createNestedObject();
        obj["id"] = s.id;
        obj["shadeId"] = s.shadeId;
        obj["shadeName"] = s.shadeName;
        obj["hour"] = s.hour;
        obj["minute"] = s.minute;
        
        // FIX: Zeit immer im Format HH:MM
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", s.hour, s.minute);
        obj["time"] = timeStr;
        
        obj["days"] = s.days;
        obj["command"] = s.command;
        obj["enabled"] = s.enabled;
        obj["lastRun"] = s.lastRun;
    }
    
    String json;
    serializeJson(arr, json);
    return json;
}

String SchedulerClass::scheduleToJSON(Schedule& schedule) {
    DynamicJsonDocument doc(512);
    doc["id"] = schedule.id;
    doc["shadeId"] = schedule.shadeId;
    doc["shadeName"] = schedule.shadeName;
    doc["hour"] = schedule.hour;
    doc["minute"] = schedule.minute;
    
    // FIX: Zeit immer im Format HH:MM
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", schedule.hour, schedule.minute);
    doc["time"] = timeStr;
    
    doc["days"] = schedule.days;
    doc["command"] = schedule.command;
    doc["enabled"] = schedule.enabled;
    doc["lastRun"] = schedule.lastRun;
    
    String json;
    serializeJson(doc, json);
    return json;
}

// ========== DEBUG-AUSGABEN ==========
void SchedulerClass::printSchedule(Schedule& s) {
    Serial.printf("  ID: %u\n", s.id);
    Serial.printf("  Shade: %s (ID %d)\n", s.shadeName, s.shadeId);
    Serial.printf("  Zeit: %02d:%02d Uhr\n", s.hour, s.minute);
    Serial.printf("  Tage (Bitmask): 0x%02X (binär: ", s.days);
    
    // Binär-Ausgabe
    for (int i = 6; i >= 0; i--) {
        Serial.print((s.days & (1 << i)) ? '1' : '0');
    }
    Serial.print(") = ");
    
    // Tage ausschreiben
    const char* dayNames[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    bool first = true;
    for (int i = 0; i < 7; i++) {
        if (s.days & (1 << i)) {
            if (!first) Serial.print(", ");
            Serial.print(dayNames[i]);
            first = false;
        }
    }
    Serial.println();
    
    Serial.printf("  Befehl: %s\n", s.command);
    Serial.printf("  Status: %s\n", s.enabled ? "AKTIV" : "INAKTIV");
    Serial.printf("  Zuletzt ausgeführt: %u\n", s.lastRun);
}

void SchedulerClass::printAllSchedules() {
    if (schedules.empty()) {
        Serial.println("  (keine Zeitpläne vorhanden)");
        return;
    }
    
    for (auto& s : schedules) {
        Serial.println("----------------------------------------");
        printSchedule(s);
    }
    Serial.println("----------------------------------------");
}

// ========== SPEICHERN/LADEN ==========
void SchedulerClass::save() {
    saveToFile();
}

void SchedulerClass::load() {
    loadFromFile();
}

void SchedulerClass::saveToFile() {
    File file = LittleFS.open("/schedules.dat", "w");
    if (!file) {
        Serial.println("Scheduler: FEHLER beim Öffnen der Datei zum Speichern!");
        return;
    }
    
    // Schreibe Header
    uint16_t count = schedules.size();
    file.write((uint8_t*)&count, sizeof(count));
    file.write((uint8_t*)&nextId, sizeof(nextId));
    
    // Schreibe alle Schedules
    for (auto& s : schedules) {
        size_t written = file.write((uint8_t*)&s, sizeof(Schedule));
        if (written != sizeof(Schedule)) {
            Serial.printf("Scheduler: WARNUNG - Nur %d von %d Bytes geschrieben!\n",
                         written, sizeof(Schedule));
        }
    }
    
    file.close();
    Serial.printf("Scheduler: %d Zeitpläne in Datei gespeichert (%d Bytes)\n", 
                  count, sizeof(Schedule) * count);
}

void SchedulerClass::loadFromFile() {
    schedules.clear();
    
    if (!LittleFS.exists("/schedules.dat")) {
        Serial.println("Scheduler: Keine gespeicherten Zeitpläne vorhanden");
        nextId = 1;
        return;
    }
    
    File file = LittleFS.open("/schedules.dat", "r");
    if (!file) {
        Serial.println("Scheduler: FEHLER beim Öffnen der Datei zum Laden!");
        return;
    }
    
    // Lese Header
    uint16_t count;
    file.read((uint8_t*)&count, sizeof(count));
    file.read((uint8_t*)&nextId, sizeof(nextId));
    
    Serial.printf("Scheduler: Lade %d Zeitpläne...\n", count);
    
    // Lese alle Schedules
    for (uint16_t i = 0; i < count; i++) {
        Schedule s;
        size_t bytesRead = file.read((uint8_t*)&s, sizeof(Schedule));
        
        if (bytesRead == sizeof(Schedule)) {
            schedules.push_back(s);
        } else {
            Serial.printf("Scheduler: FEHLER beim Laden von Zeitplan %d (nur %d Bytes)\n",
                         i, bytesRead);
        }
    }
    
    file.close();
    Serial.printf("Scheduler: %d Zeitpläne erfolgreich geladen\n", schedules.size());
}