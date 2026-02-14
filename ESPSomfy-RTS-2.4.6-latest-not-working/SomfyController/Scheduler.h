#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <time.h>

#define MAX_SCHEDULES 50

// ========== SCHEDULE STRUCTURE ==========
struct Schedule {
    uint32_t id;
    uint8_t shadeId;
    uint8_t hour;           // 0-23
    uint8_t minute;         // 0-59
    uint8_t days;           // Bitmaske: Bit 0=So, 1=Mo, ..., 6=Sa
    char command[12];       // "up", "down", "my", "stop"
    bool enabled;
    uint32_t lastRun;       // YYYYMMDD Format (z.B. 20250209)
    char shadeName[64];     // FIX: Erhöht von 32 auf 64 für längere Namen
};

// ========== SCHEDULER CLASS ==========
class SchedulerClass {
public:
    SchedulerClass();
    
    // Initialisierung & Loop
    void begin();
    void loop();
    
    // Speichern/Laden
    void save();
    void load();
    
    // Schedule Management
    bool addSchedule(Schedule& schedule);
    bool updateSchedule(uint32_t id, Schedule& schedule);
    bool deleteSchedule(uint32_t id);
    Schedule* getSchedule(uint32_t id);
    std::vector<Schedule> getAllSchedules();
    
    // Status
    void setEnabled(uint32_t id, bool enabled);
    bool isEnabled(uint32_t id);
    
    // JSON Serialisierung
    String toJSON();
    String scheduleToJSON(Schedule& schedule);
    
    // Debug
    void printSchedule(Schedule& s);
    void printAllSchedules();
    
private:
    std::vector<Schedule> schedules;
    time_t lastCheck;
    uint32_t nextId;
    
    // Interne Funktionen
    void checkSchedules();
    void executeSchedule(Schedule& schedule);
    bool shouldRun(Schedule& schedule, struct tm* timeinfo);
    uint32_t getTodayDate(struct tm* timeinfo);  // FIX: Neue Funktion
    
    // Datei-I/O
    void saveToFile();
    void loadFromFile();
};

extern SchedulerClass scheduler;

#endif // SCHEDULER_H