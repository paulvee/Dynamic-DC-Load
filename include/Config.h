/**
 * @file Config.h
 * @brief Configuration, pins, and calibration constants for Dynamic Load
 * @author Paul Versteeg
 * @version 7.0.2
 * @date 2024
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Firmware version
const String FW_VERSION = "7.0.2";

//=============================================================================
// PIN DEFINITIONS
//=============================================================================

#define LED 2         // Built-in LED - used as a heart beat
#define DSO_TRIG 4    // Optional: to trace real-time activity on a scope
#define DUT_PWR 13    // Relay to turn DUT power on/off - default is off
#define CV_MODE 15    // Activating the CV mode
#define DC 16         // OLED DC
#define RES 17        // OLED RES
#define CS 5          // OLED CS
#define SCL 18        // OLED SCL
#define ADC_RDY 19    // ADC ready pin - not used at the moment
#define I2C_SDA 21    // I2C bus
#define I2C_SCL 22    // I2C bus
#define SDA 23        // OLED SDA/MOSI
#define NFET_OFF 25   // Turn the NFET's on/off - default is off
#define FAN_PWM 26    // Driving the temperature controlled fan
#define ENC_A 27      // Rotary encoder pin A
#define ENC_B 32      // Rotary encoder pin B
#define ENC_BUT 33    // Rotary encoder push button
#define FAN_TACHO 34  // Tacho output from fan - not used at the moment

//=============================================================================
// I2C ADDRESSES
//=============================================================================

// Note: ADS1115_ADDRESS is already defined in ADS1X15.h library
#define DAC8571_ADDRESS 0x4C  // DAC8571 address with A0 grounded

//=============================================================================
// OLED DISPLAY DEFINITIONS
//=============================================================================

// Screen dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128

// Vertical line positions (down from the top)
#define v_line 25   // Volt line
#define a_line 56   // Amp line
#define p_line 58   // Mode + Power line
#define s_line 87   // Set line + Set suffix
#define m_line 106  // DAC/Menu/Status/Temp line

#define line_1 0  // Vertical line position in pixels
#define line_2 20
#define line_3 40
#define line_4 60
#define line_5 80
#define line_6 100

// Horizontal starting positions for FreeSans18pt large digits
#define ldigit_1 6    // 10.000 or 100.00
#define ldigit_2 25   //  1.000
#define sdigit_6 114  // V, A and W suffix position

// Horizontal starting positions for FreeSans9pt digits
#define digit_0 0
#define digit_1 10
#define digit_2 20
#define digit_3 30
#define digit_4 40
#define digit_5 50
#define digit_6 60
#define digit_7 70
#define digit_8 80
#define digit_9 90
#define digit_10 100

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define L_BLUE 0x0010
#define RED 0xF800
#define BR_RED 0xF810
#define GREEN 0x07E0
#define DK_GREEN 0x03E0
#define CYAN 0x07FF
#define ORANGE 0xFD20
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define L_GREY 0xC618
#define D_GREY 0x7BEF
#define GR_YELLOW 0xAFE5

//=============================================================================
// CALIBRATION CONSTANTS
//=============================================================================

// Voltage calibration
const double dutVcalib = 1.0;  // Optional DUT voltage calibration factor

// Current calibration
const double DUTCurrent = 400.00;   // DAC-ADC calibration point (mV)
const double shuntVcalib = 2.5000;  // Current display calibration factor

// CV mode calibration
const double cvCalFactor = 1.054333;  // CV cut-in voltage calibration factor

//=============================================================================
// ENCODER SETTINGS
//=============================================================================

const int pauseLength = 25000;                // Microseconds for acceleration detection
const int fastIncrement = 10;                 // Speed multiplier for fast rotation
const double setCurrCal = 1.25;               // Current calibration factor
const int EncoderRes = 160 / 2 / setCurrCal;  // Encoder steps to 10mA per click

//=============================================================================
// CURRENT LIMIT DEFINITIONS
//=============================================================================

#define maxCurrent_1A 6400
#define maxCurrent_2A 12800
#define maxCurrent_4A 25600
#define maxCurrent_5A 32000
#define maxCurrent_10A 64000

//=============================================================================
// BUTTON TIMING
//=============================================================================

const int SHORT_PRESS_TIME = 1000;  // Less than 1000ms
const int LONG_PRESS_TIME = 1000;   // Longer than 1000ms

//=============================================================================
// FAN CONTROLLER SETTINGS
//=============================================================================

const int FAN_PWM_FREQ = 25000;    // 25kHz PWM frequency
const int FAN_PWM_RESOLUTION = 8;  // 8-bit resolution (0-255)

//=============================================================================
// RTOS CORE ASSIGNMENTS
//=============================================================================

const int main_cpu = 0;  // Main core for time-critical tasks
const int app_cpu = 1;   // Application core for display/fan tasks

//=============================================================================
// SERIAL COMMUNICATION
//=============================================================================

const long SERIAL_BAUD = 9600;  // Battery test mode requires 9600 baud

#endif  // CONFIG_H
