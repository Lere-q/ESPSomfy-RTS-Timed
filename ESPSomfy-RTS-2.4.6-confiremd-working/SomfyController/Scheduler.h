#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <time.h>

#define MAX_SCHEDULES 50

struct Schedule {
    uint32_t id;
    uint8_t shadeId;
    uint8_t hour;
    uint8_t minute;
    uint8_t days;           // Bitmaske: Bit 0=So, 1=Mo, ..., 6=Sa
    char command[12];       // "up", "down", "my", "stop"
    bool enabled;
    uint32_t lastRun;       // Timestamp der letzten Ausführung
    char shadeName[32];
};

class SchedulerClass {
public:
    SchedulerClass();
    void begin();
    void loop();
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
    
private:
    std::vector<Schedule> schedules;
    time_t lastCheck;
    uint32_t nextId;
    
    void checkSchedules();
    void executeSchedule(Schedule& schedule);
    bool shouldRun(Schedule& schedule);
    void saveToFile();
    void loadFromFile();
    uint8_t getCurrentDay();
    String getCurrentTime();
};

extern SchedulerClass scheduler;

#endif
