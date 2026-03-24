/**
 * @file DynamicLoad.h
 * @brief Main declarations for Dynamic Load system
 * @author Paul Versteeg
 * @date 2026
 */

#ifndef DYNAMIC_LOAD_H
#define DYNAMIC_LOAD_H

#include <ADS1X15.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <Arduino.h>
#include <DAC8571.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <SPI.h>
#include <Wire.h>
#include <ezButton.h>

#include "Config.h"
#include "MovingAverage.h"  // for shared filters

// Forward declarations
// Note: MovingAverage is a template class, included in main.cpp

//=============================================================================
// OPERATING MODES
//=============================================================================

enum OperatingMode {
    current,     // CC - Constant Current
    voltage,     // CV - Constant Voltage
    power,       // CP - Constant Power
    resistance,  // CR - Constant Resistance
    battery      // BT - Battery Test Mode
};

extern const char* modeStrings[];
extern const char* suffixStrings[];

//=============================================================================
// GLOBAL HARDWARE OBJECTS
//=============================================================================

extern ADS1115 ADS;
extern DAC8571 dac;
extern Adafruit_SSD1351 tft;
extern ezButton button;

//=============================================================================
// GLOBAL STATE VARIABLES
//=============================================================================

// Operating mode
extern OperatingMode mode;
extern volatile int prev_mode;

// NFET state
extern bool nfetState;

// ADC gain control
extern uint8_t adcGain;
extern uint8_t old_adcGain;

// Measurements (volatile for RTOS protection)
extern volatile double temperature;
extern volatile double dutV;           // DUT voltage
extern volatile double dispVoltage;    // Averaged voltage for display
extern volatile long rawV;             // Raw ADC value for voltage
extern volatile long rawV_avg;         // Averaged raw voltage
extern volatile double shuntV;         // Shunt voltage (current)
extern volatile double dispCurrent;    // Averaged current for display
extern volatile long shuntVraw;        // Raw shunt value
extern volatile long rawI_avg;         // Averaged raw current
extern volatile double dutPower;       // Power in Watts
extern volatile double dutResistance;  // Calculated resistance

// Moving average filters defined in main.cpp
extern MovingAverage<long, 16> avgVoltage;
extern MovingAverage<long, 16> avgCurrent;
extern MovingAverage<long, 16> avgTemperature;

// Set points
extern double set_current;
extern double set_voltage;
extern double set_power;
extern double set_resistance;

// DAC control
extern volatile uint16_t DAC;
extern volatile uint16_t prev_DAC;

// Regulation deltas
extern double powerDelta;
extern double voltDelta;
extern int currentDelta;

// Safety limits
extern long maxCurrent;
extern long maxVoltage;
extern long maxPower;
extern long maxTemp;
extern long minVoltage;

// Calibration constants (static/const)
extern const double voltage_ref;
extern const double dc_cal_factor;
extern double currOffset;

// Runtime calibration values (loaded from DL_Cal_Values.ini)
extern double dutVcalib;          // DUT voltage calibration factor
extern double DAC_ADC_TOLERANCE;  // DAC-ADC calibration point in mV (measured voltage at calibration point)
extern double shuntVcalib;        // Current display calibration factor
extern double cvCalFactor;        // CV mode trigger point calibration

//=============================================================================
// RTOS TASK HANDLES
//=============================================================================

extern TaskHandle_t BThandle;
extern SemaphoreHandle_t xSemaphore;
extern portMUX_TYPE mutex;

//=============================================================================
// FUNCTION DECLARATIONS
//=============================================================================

// Main tasks
void mainLoop(void* pvParameters);
void process_encoder(void* pvParameters);
void updateOledDisplay(void* pvParameters);

// Measurement functions
void readDutV();
void readShunt();
void readTemp();
void checkValidValues();

// Communication functions
void send_volt();
void send_current();
void send_power();
void send_encoder();
void send_dac();
void send_mode();
void send_temp();
void send_to_monitor();

// Utility functions
void empty_avg_pool();

// Battery mode functions (from BatteryMode.h)
extern void batteryMode(void* pvParameters);

// Fan controller functions (from FanController.h)
extern void fanController(void* pvParameters);
extern void TachCounter_ISR();  // not used yet, but declared for future use

// OLED display functions (from OLEDDisplay.h)
extern void setup_oled();
extern void oled_prep();

// Rotary encoder functions (from RotaryEncoder.h)
extern void read_encoder();
extern volatile unsigned long encoderPos;
extern int encoderRes;

#endif  // DYNAMIC_LOAD_H
