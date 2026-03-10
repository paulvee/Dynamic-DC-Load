/**
 * @file RotaryEncoder.h
 * @brief Rotary encoder input handling
 * @author Paul Versteeg
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

// Encoder ISR
void read_encoder();

// Encoder processing task (RTOS)
void process_encoder(void* pvParameters);

// Encoder variables
extern volatile unsigned long encoderPos;
extern int encoderRes;
extern unsigned long maxEncPos;

// Button timing
extern const int SHORT_PRESS_TIME;
extern const int LONG_PRESS_TIME;
extern unsigned long pressedTime;
extern unsigned long releasedTime;
extern bool isPressing;
extern bool isLongDetected;

#endif  // ROTARY_ENCODER_H