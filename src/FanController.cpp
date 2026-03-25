/**
 * @file FanController.cpp
 * @brief Temperature-controlled fan management with PWM
 * @author Paul Versteeg
 * @date 2026
 *
 * Controls fan speed based on heatsink temperature.
 * Runs as a separate RTOS task on application core.
 *
 * Based on: https://youtu.be/YBNEXWp-gf0?si=BXFszrRLiTj59C2J
 */

#include "FanController.h"

#include "Config.h"
#include "DynamicLoad.h"

//=============================================================================
// FAN VARIABLES
//=============================================================================

volatile int tachCount = 0;  // not used at the moment, but declared for future use

//=============================================================================
// FAN CONTROLLER TASK (Core 1)
//=============================================================================

/**
 * @brief RTOS task for temperature-controlled fan management
 *
 * Controls fan speed based on heatsink temperature:
 * - ≤30°C: Fan off
 * - 30-60°C: Proportional speed control
 * - >60°C: Maximum speed
 *
 * @param pvParameters Unused RTOS parameter
 */
void fanController(void* pvParameters) {
    // boot message with core info
    Serial.printf("- Fan controller task started, running on core %d\r\n", xPortGetCoreID());

    int fan_pwm = 0;
    int _temp = 0;

    while (true) {
        // Read temperature (into local copy to reduce clobbering)
        portENTER_CRITICAL(&mutex);
        _temp = int(temperature);
        portEXIT_CRITICAL(&mutex);

        // Temperature control logic
        if (_temp <= 30) {
            // Below 30°C: Turn fan off
            fan_pwm = 0;
            ledcWrite(FAN_PWM_CHANNEL, fan_pwm);
        } else if (_temp > 30 && _temp <= 60) {
            // 30-60°C: Proportional control
            fan_pwm = map(_temp, 0, 80, 0, pow(2, FAN_PWM_RESOLUTION) - 1);
            ledcWrite(FAN_PWM_CHANNEL, fan_pwm);
        } else {
            // Above 60°C: Maximum speed
            fan_pwm = 255;
            ledcWrite(FAN_PWM_CHANNEL, fan_pwm);
        }

        // Update fan speed every 2 seconds
        vTaskDelay(pdMS_TO_TICKS(2000));

        // Optional RPM measurement (currently disabled)
        /*
        unsigned long start_time = millis();
        tachCount = 0;
        while ((millis() - start_time) < 1000) {
            // Count tach transitions during 1 second
        }
        int fan_rpm = tachCount * 30;  // RPM formula: count * 60 / 2

        // Debug output
        Serial.print("Heatsink temp = ");
        Serial.print(_temp);
        Serial.print("  Fan PWM = ");
        Serial.print(fan_pwm);
        Serial.print(" Speed = ");
        Serial.print(fan_rpm);
        Serial.println(" rpm");
        */
    }
}

//=============================================================================
// TACHOMETER ISR
//=============================================================================

/**
 * @brief Interrupt service routine for fan tachometer
 *
 * Counts rising edges on the fan tachometer output.
 * Currently not actively used for RPM measurement.
 */
void TachCounter_ISR() {
    tachCount++;
}
