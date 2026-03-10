/**
 * @file Debug.cpp
 * @brief Debug utilities for serial monitor output
 * @author Paul Versteeg
 * @date 2024
 *
 * Optional debugging functions for development and troubleshooting.
 * Uncomment send_to_monitor() call in mainLoop to enable.
 */

#include "Debug.h"

#include "DynamicLoad.h"

/**
 * @brief Send measurements to Arduino IDE serial monitor
 *
 * Displays: voltage, current, power, DAC, resistance, temperature,
 * encoder position, and set current.
 *
 * To use: Uncomment the call to this function in mainLoop()
 */
void send_to_monitor() {
    // Read all shared variables with mutex protection
    double local_dutV, local_shuntV, local_dutPower, local_dutResistance, local_temperature;
    uint16_t local_DAC, local_set_current;
    unsigned long local_encoderPos;

    portENTER_CRITICAL(&mutex);
    local_dutV = dutV;
    local_shuntV = shuntV;
    local_dutPower = dutPower;
    local_DAC = DAC;
    local_dutResistance = dutResistance;
    local_temperature = temperature;
    local_encoderPos = encoderPos;
    local_set_current = set_current;
    portEXIT_CRITICAL(&mutex);

    Serial.print(local_dutV, 2);
    Serial.print(" V\t");

    Serial.print(local_shuntV, 2);
    Serial.print(" A\t");

    Serial.print(local_dutPower, 2);
    Serial.print(" Watt\t");

    Serial.print(local_DAC);
    Serial.print("\t");

    Serial.print("resistance: ");
    Serial.print(local_dutResistance);
    Serial.print("\t");

    Serial.print(local_temperature);
    Serial.print("  temp in C\t");

    Serial.print(local_encoderPos);
    Serial.print(" encoderPos\t");

    Serial.print(local_set_current);
    Serial.println(" setCurrent");
}
