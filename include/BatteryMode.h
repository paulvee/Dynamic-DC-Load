/**
 * @file BatteryMode.h
 * @brief Battery discharge test mode declarations
 * @author Paul Versteeg
 */

#ifndef BATTERY_MODE_H
#define BATTERY_MODE_H

#include <Arduino.h>

// Battery mode task (RTOS)
void batteryMode(void* pvParameters);

// Battery test variables
extern bool battSetup;
extern bool end_of_test;
extern double mAh_soFar;
extern double current_mA;
extern int days, hours, mins, secs;
extern int tMins;
extern bool timed_out;
extern bool cutoff_voltage_reached;
extern bool batteryModeActive;
extern bool stop_oled_vars;

// Test parameters from PC
extern long target_mA;
extern double cutoff_voltage;
extern long time_limit;
extern unsigned long sampleTime;
extern int recovery_time_minutes;
extern int cancel;

// Recovery monitoring
extern bool in_recovery_mode;
extern unsigned long recovery_start_millis;

// Communication watchdog
extern volatile unsigned long lastSerialActivity;
extern const unsigned long COMM_WATCHDOG_TIMEOUT;

// Timing variables
extern unsigned long startMillisec;
extern unsigned long millis_PC_wait;
extern unsigned long millisCalc_mAh;
extern float last_hours;
extern unsigned long timeout_millis;
extern unsigned long start;
extern bool appPresence;

#endif  // BATTERY_MODE_H