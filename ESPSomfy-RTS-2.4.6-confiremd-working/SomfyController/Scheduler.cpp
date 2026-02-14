#include "Scheduler.h"
#include "Somfy.h"
#include "ConfigSettings.h"
#include <LittleFS.h>

// Externe Deklaration der globalen somfy Instanz
extern SomfyShadeController somfy;

SchedulerClass scheduler;

SchedulerClass::SchedulerClass() {
    lastCheck = 0;
    nextId = 1;
}

void SchedulerClass::begin() {
    Serial.println("Scheduler: Initialisiere...");
    loadFromFile();
    Serial.printf("Scheduler: %d Zeitpläne geladen\n", schedules.size());
}

void SchedulerClass::loop() {
    time_t now;
    time(&now);
    
    // Prüfe nur einmal pro Minute
    if (now - lastCheck >= 60) {
        lastCheck = now;
        checkSchedules();
    }
}

void SchedulerClass::checkSchedules() {
    time_t now;
    time(&now);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    uint8_t currentDay = timeinfo.tm_wday;  // 0=So, 1=Mo, ..., 6=Sa
    uint8_t currentHour = timeinfo.tm_hour;
    uint8_t currentMinute = timeinfo.tm_min;
    
    for (auto& schedule : schedules) {
        if (!schedule.enabled) continue;
        
        // Prüfe Wochentag
        if (!(schedule.days & (1 << currentDay))) continue;
        
        // Prüfe Uhrzeit
        if (schedule.hour != currentHour || schedule.minute != currentMinute) continue;
        
        // Prüfe ob heute bereits ausgeführt
        uint32_t today = timeinfo.tm_year * 10000 + timeinfo.tm_mon * 100 + timeinfo.tm_mday;
        if (schedule.lastRun == today) continue;
        
        Serial.printf("Scheduler: Führe aus - Shade %d, Command: %s\n", 
                      schedule.shadeId, schedule.command);
        
        executeSchedule(schedule);
        schedule.lastRun = today;
        save();
    }
}

void SchedulerClass::executeSchedule(Schedule& schedule) {
    // 1 Shade holen
    SomfyShade* shade = somfy.getShadeById(schedule.shadeId);
    if (!shade) {
        Serial.printf(
            "Scheduler: FEHLER - Shade %d nicht gefunden!\n",
            schedule.shadeId
        );
        return;
    }

    // 2 String → somfy_commands
    somfy_commands cmd = translateSomfyCommand(String(schedule.command));

    

    // 4 Logging
    Serial.printf(
        "Scheduler: Sende %s an Shade %d (%s)\n",
        translateSomfyCommand(cmd).c_str(),
        schedule.shadeId,
        shade->name
    );

    // 5 ✅ RICHTIGER SEND-AUFRUF
    shade->sendCommand(
        cmd,
        1   // repeat = 1 → stabiler RTS-Tastendruck
    );
}


bool SchedulerClass::addSchedule(Schedule& schedule) {
    if (schedules.size() >= MAX_SCHEDULES) {
        Serial.println("Scheduler: Maximale Anzahl erreicht!");
        return false;
    }
    
    schedule.id = nextId++;
    schedule.lastRun = 0;
    schedules.push_back(schedule);
    
    Serial.printf("Scheduler: Hinzugefügt - ID %d, Shade %d, %02d:%02d\n",
                  schedule.id, schedule.shadeId, schedule.hour, schedule.minute);
    
    save();
    return true;
}

bool SchedulerClass::updateSchedule(uint32_t id, Schedule& schedule) {
    for (auto& s : schedules) {
        if (s.id == id) {
            schedule.id = id;
            s = schedule;
            save();
            return true;
        }
    }
    return false;
}

bool SchedulerClass::deleteSchedule(uint32_t id) {
    for (auto it = schedules.begin(); it != schedules.end(); ++it) {
        if (it->id == id) {
            schedules.erase(it);
            save();
            Serial.printf("Scheduler: Gelöscht - ID %d\n", id);
            return true;
        }
    }
    return false;
}

Schedule* SchedulerClass::getSchedule(uint32_t id) {
    for (auto& s : schedules) {
        if (s.id == id) return &s;
    }
    return nullptr;
}

std::vector<Schedule> SchedulerClass::getAllSchedules() {
    return schedules;
}

void SchedulerClass::setEnabled(uint32_t id, bool enabled) {
    Schedule* s = getSchedule(id);
    if (s) {
        s->enabled = enabled;
        save();
    }
}

bool SchedulerClass::isEnabled(uint32_t id) {
    Schedule* s = getSchedule(id);
    return s ? s->enabled : false;
}

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
        obj["time"] = String(s.hour) + ":" + (s.minute < 10 ? "0" : "") + String(s.minute);
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
    doc["time"] = String(schedule.hour) + ":" + (schedule.minute < 10 ? "0" : "") + String(schedule.minute);
    doc["days"] = schedule.days;
    doc["command"] = schedule.command;
    doc["enabled"] = schedule.enabled;
    doc["lastRun"] = schedule.lastRun;
    
    String json;
    serializeJson(doc, json);
    return json;
}

void SchedulerClass::save() {
    saveToFile();
}

void SchedulerClass::saveToFile() {
    File file = LittleFS.open("/schedules.dat", "w");
    if (!file) {
        Serial.println("Scheduler: Fehler beim Speichern!");
        return;
    }
    
    // Schreibe Anzahl
    uint16_t count = schedules.size();
    file.write((uint8_t*)&count, sizeof(count));
    file.write((uint8_t*)&nextId, sizeof(nextId));
    
    // Schreibe alle Schedules
    for (auto& s : schedules) {
        file.write((uint8_t*)&s, sizeof(Schedule));
    }
    
    file.close();
    Serial.printf("Scheduler: %d Zeitpläne gespeichert\n", count);
}

void SchedulerClass::loadFromFile() {
    schedules.clear();
    
    if (!LittleFS.exists("/schedules.dat")) {
        Serial.println("Scheduler: Keine gespeicherten Daten");
        return;
    }
    
    File file = LittleFS.open("/schedules.dat", "r");
    if (!file) {
        Serial.println("Scheduler: Fehler beim Laden!");
        return;
    }
    
    // Lese Anzahl
    uint16_t count;
    file.read((uint8_t*)&count, sizeof(count));
    file.read((uint8_t*)&nextId, sizeof(nextId));
    
    // Lese alle Schedules
    for (uint16_t i = 0; i < count; i++) {
        Schedule s;
        file.read((uint8_t*)&s, sizeof(Schedule));
        schedules.push_back(s);
    }
    
    file.close();
    Serial.printf("Scheduler: %d Zeitpläne geladen\n", count);
}

void SchedulerClass::load() {
    loadFromFile();
}
