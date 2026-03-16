/**
 * @file FanController.h
 * @brief Temperature-controlled fan management
 * @author Paul Versteeg
 */

#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <Arduino.h>

// Fan controller task (RTOS)
void fanController(void* pvParameters);

// Tachometer ISR
void TachCounter_ISR();

// Fan control variables
extern volatile int tachCount;

#endif  // FAN_CONTROLLER_H