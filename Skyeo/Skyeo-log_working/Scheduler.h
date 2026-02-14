#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <Arduino.h>
#include "Config.h"

// ============================================
// SKYEO - Integrated Scheduler
// Time-based shade control (no NTP)
// ============================================

// Kommandos
#define SCHED_CMD_UP 0
#define SCHED_CMD_DOWN 1
#define SCHED_CMD_MY 2
#define SCHED_CMD_TARGET 3

// Tage als Bitmaske
#define SCHED_DAY_SUN 0x01
#define SCHED_DAY_MON 0x02
#define SCHED_DAY_TUE 0x04
#define SCHED_DAY_WED 0x08
#define SCHED_DAY_THU 0x10
#define SCHED_DAY_FRI 0x20
#define SCHED_DAY_SAT 0x40
#define SCHED_DAY_ALL 0x7F
#define SCHED_DAY_WEEKDAY 0x3E
#define SCHED_DAY_WEEKEND 0x41

class Scheduler {
public:
    static void begin();
    static void loop();
    
    // Schedule Management
    static bool addSchedule(const Schedule& sched);
    static bool updateSchedule(uint8_t index, const Schedule& sched);
    static bool deleteSchedule(uint8_t index);
    static bool getSchedule(uint8_t index, Schedule& sched);
    static uint8_t getScheduleCount();
    
    // Time (relative to boot)
    static void setTime(uint32_t seconds); // Set start time
    static uint32_t getTime(); // Seconds since boot/start
    static uint8_t getHour();
    static uint8_t getMinute();
    static uint8_t getDayOfWeek(); // 0=Sun, 1=Mon, ..., 6=Sat
    
    // Enable/Disable
    static void setEnabled(uint8_t index, bool enabled);
    static bool isEnabled(uint8_t index);
    
    // Execute
    static void executeSchedule(const Schedule& sched);
    
    // Date calculation (YYYYMMDD)
    static uint32_t getTodayDate();

private:
    static uint32_t bootTime; // Timestamp at boot (set by user or 0)
    static uint32_t lastCheck;
    static uint32_t checkCounter; // Für Tag-Berechnung
    
    static bool shouldRun(const Schedule& sched);
    static uint8_t getCurrentDayOfWeek();
};

#endif // SCHEDULER_H
