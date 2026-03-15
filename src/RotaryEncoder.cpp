/**
 * @file RotaryEncoder.cpp
 * @brief Rotary encoder input handling with ISR and button processing
 * @author Paul Versteeg
 * @date 2026
 *
 * Based on https://github.com/mo-thunderz/RotaryEncoder
 * ISR runtime: ~1.5µs
 */

#include "RotaryEncoder.h"

#include "Config.h"
#include "DynamicLoad.h"

//=============================================================================
// ENCODER VARIABLES
//=============================================================================

// Timing for acceleration
unsigned long lastIncReadTime = micros();
unsigned long lastDecReadTime = micros();

// Encoder position and resolution
volatile unsigned long encoderPos = 0;
int encoderRes = 10;  // Multiplier: toggles between 10mA and 100mA step resolution

// Maximum encoder position (16-bit DAC resolution)
unsigned long maxEncPos = (int)(pow(2, 16) - 1);

// Button state variables (timing constants are in Config.h)
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

//=============================================================================
// ROTARY ENCODER ISR
//=============================================================================

/**
 * @brief Interrupt service routine for rotary encoder
 *
 * Handles both A and B pin changes.
 * Signals the process_encoder task to handle the update.
 */
void IRAM_ATTR read_encoder() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Signal the waiting task to process the encoder change
    xSemaphoreGiveFromISR(xSemaphore, &xHigherPriorityTaskWoken);

    // Yield to highest priority task if needed
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

//=============================================================================
// ENCODER PROCESSING TASK (Core 0)
//=============================================================================

/**
 * @brief RTOS task to process rotary encoder activity
 *
 * Runs on the main core. Gets signaled by the ISR to start processing.
 * Implements acceleration for fast rotation.
 *
 * @param pvParameters Unused RTOS parameter
 */
void process_encoder(void* pvParameters) {
    // boot message with core info
    Serial.print("- Encoder task started, running on core ");
    Serial.println(xPortGetCoreID());

    // Create binary semaphore for ISR signaling
    xSemaphore = xSemaphoreCreateBinary();

    // Attach interrupts on Core 0 (where this task runs) for efficient signaling
    attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);
    Serial.println("- Encoder ISRs attached on core 0");

    while (true) {
        // Wait for ISR signal
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            // Lookup table for encoder state machine
            static uint8_t old_AB = 3;
            static int8_t encval = 0;
            static const int8_t enc_states[] = {
                0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

            // Remember previous state and add current state
            old_AB <<= 2;

            // Read encoder pins (swap A and B if encoder turns wrong way)
            if (digitalRead(ENC_A)) old_AB |= 0x01;
            if (digitalRead(ENC_B)) old_AB |= 0x02;

            // Update encoder value from lookup table
            encval += enc_states[(old_AB & 0x0f)];

            // Four steps forward
            if (encval > 3) {
                int changevalue = 1;

                // Acceleration for fast rotation
                if ((micros() - lastIncReadTime) < pauseLength) {
                    changevalue = fastIncrement * changevalue;
                }
                lastIncReadTime = micros();

                // Read shared variables with mutex protection
                portENTER_CRITICAL(&mutex);
                double local_dutPower = dutPower;
                double local_temperature = temperature;
                portEXIT_CRITICAL(&mutex);

                // Only increment when below power and temperature limits
                if ((local_dutPower < maxPower) && (local_temperature < maxTemp)) {
                    // Enter critical section for encoderPos update
                    portENTER_CRITICAL(&mutex);
                    // Near the end, increment by 1
                    if (encoderPos > maxEncPos - 10) {
                        if (encoderPos < maxEncPos) {
                            encoderPos = encoderPos + 1;
                        } else {
                            encoderPos = maxEncPos;
                        }
                    } else {
                        encoderPos = encoderPos + changevalue;
                    }

                    // Don't exceed limit
                    if (encoderPos >= maxEncPos) {
                        encoderPos = maxEncPos;
                    }

                    // Exit critical section
                    portEXIT_CRITICAL(&mutex);
                }
                encval = 0;
            }

            // Four steps backward
            if (encval < -3) {
                int changevalue = 1;

                // Acceleration for fast rotation
                if ((micros() - lastDecReadTime) < pauseLength) {
                    changevalue = fastIncrement * changevalue;
                }
                lastDecReadTime = micros();

                // Enter critical section for encoderPos update
                portENTER_CRITICAL(&mutex);

                // Handle unsigned value underflow carefully
                if (encoderPos < 10) {
                    if (encoderPos > 1) {
                        encoderPos = encoderPos - 1;
                    } else {
                        encoderPos = 0;
                    }
                } else {
                    encoderPos = encoderPos - changevalue;
                }

                // Don't go negative
                if (encoderPos <= 1) {
                    encoderPos = 0;
                }

                // Exit critical section
                portEXIT_CRITICAL(&mutex);
                encval = 0;
            }
        }
    }
}
