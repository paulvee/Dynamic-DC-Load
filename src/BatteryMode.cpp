/**
 * @file BatteryMode.cpp
 * @brief Battery discharge test mode implementation
 * @author Paul Versteeg
 * @date 2024
 *
 * Based on design from John Lowen <john@vwlowen.co.uk>
 * http://www.vwlowen.co.uk/arduino/battery-tester/battery-tester.htm
 *
 * See blog: https://www.paulvdiyblogs.net/2019/03/a-pretty-universal-battery-cell-tester.html
 */

#include "BatteryMode.h"

#include <esp_task_wdt.h>

#include "Config.h"
#include "DynamicLoad.h"
#include "OLEDDisplay.h"

//=============================================================================
// BATTERY MODE VARIABLES - All declared extern in BatteryMode.h
// The actual definitions are in main.cpp since they're shared across modules
//=============================================================================

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================

void write_to_pc();
void getTime();

//=============================================================================
// BATTERY MODE TASK (Core 1)
//=============================================================================

/**
 * @brief RTOS task for battery discharge testing
 *
 * Two main segments:
 * 1. Setup: Receive parameters from PC app
 * 2. Measurement: Execute discharge test and send data to PC
 *
 * Task is suspended at boot and resumed when battery mode is selected.
 *
 * @param pvParameters Unused RTOS parameter
 */
void batteryMode(void* pvParameters) {
    Serial.print("- Battery mode task started, running on core ");
    Serial.println(xPortGetCoreID());

    Serial.println("- Battery task suspended by itself");
    vTaskSuspend(NULL);  // Wait until resumed by mode selection
    Serial.println("- Battery mode task resumed");

    // Subscribe to watchdog when battery mode becomes active
    esp_task_wdt_add(NULL);
    Serial.println("- Battery task added to watchdog");

    while (true) {
        appPresence = true;
        end_of_test = false;
        bool termination_message_sent = false;  // Track if termination message already sent

        // ===== SETUP PHASE =====
        // Run only once per discharge measurement
        while (battSetup && batteryModeActive) {
            stop_oled_vars = true;  // Pause regular OLED updates

            // Display waiting message
            tft.fillScreen(BLACK);
            tft.setTextColor(WHITE);
            tft.setCursor(digit_1, line_2);
            tft.print("Battery Test");
            tft.setCursor(digit_1, line_4);
            tft.print("Waiting for");
            tft.setCursor(digit_1, line_5);
            tft.print("PC settings...");

            // Clear receive buffer
            while (Serial.available()) {
                Serial.read();
            }

            appPresence = true;
            start = millis();

            // Wait for parameters from PC app with timeout
            while (Serial.available() == 0 && batteryModeActive) {
                if (millis() - start > timeout_millis) {
                    appPresence = false;
                    Serial.flush();
                    break;
                }
                // Reset watchdog during wait period
                esp_task_wdt_reset();
                vTaskDelay(100 / portTICK_PERIOD_MS);  // Small delay to prevent tight loop
            }

            // Receive parameters if available
            if (appPresence && battSetup && batteryModeActive) {
                if (Serial.available() > 0) {
                    // Update communication watchdog - received test parameters
                    lastSerialActivity = millis();

                    // Read first parameter - but check if it's AUTO_BT command first
                    // If Python app sends AUTO_BT before parameters, we need to acknowledge
                    if (Serial.peek() == 'A') {  // Might be "AUTO_BT"
                        String cmd = Serial.readStringUntil('\n');
                        cmd.trim();
                        if (cmd == "AUTO_BT") {
                            Serial.println("ACK_BT");
                            lastSerialActivity = millis();
                            
                            // Wait for actual parameters
                            start = millis();
                            while (Serial.available() == 0 && batteryModeActive) {
                                if (millis() - start > timeout_millis) {
                                    appPresence = false;
                                    Serial.flush();
                                    break;
                                }
                                esp_task_wdt_reset();
                                vTaskDelay(100 / portTICK_PERIOD_MS);
                            }
                        }
                        // If not AUTO_BT, data was consumed - skip to other params
                        // This shouldn't happen in normal operation
                    }

                    target_mA = Serial.parseInt();
                    cutoff_voltage = Serial.parseFloat();
                    time_limit = Serial.parseInt();
                    sampleTime = Serial.parseInt() * 1000;
                    Serial.parseInt();                          // Unused (was kP)
                    Serial.parseFloat();                        // Unused (was offset)
                    Serial.parseInt();                          // Unused (was tolerance)
                    Serial.parseInt();                          // Unused (was beep)
                    recovery_time_minutes = Serial.parseInt();  // Recovery monitoring time
                    cancel = Serial.parseInt();                 // 0 at start, 999 to abort

                    // Validate parameters
                    if (target_mA <= 0 || target_mA > 10000) target_mA = 100;                                // Default 100mA
                    if (cutoff_voltage < 0.5 || cutoff_voltage > 100) cutoff_voltage = 0.5;                  // Default 0.5V minimum
                    if (sampleTime < 1000) sampleTime = 1000;                                                // Minimum 1 second
                    if (recovery_time_minutes < 1 || recovery_time_minutes > 30) recovery_time_minutes = 5;  // Default 5 minutes
                }
                Serial.flush();

                // Display received values for 2 seconds
                tft.fillScreen(BLACK);
                tft.setCursor(0, 10);
                tft.print("Cur: ");
                tft.print(target_mA);
                tft.print(" mA");
                tft.setCursor(0, 30);
                tft.print("Cut: ");
                tft.print(cutoff_voltage, 2);
                tft.print(" V");
                tft.setCursor(0, 50);
                tft.print("T-limit: ");
                tft.print(time_limit);
                tft.print(" s");
                tft.setCursor(0, 70);
                tft.print("Recovery: ");
                tft.print(recovery_time_minutes);
                tft.print(" min");
                vTaskDelay(2000 / portTICK_PERIOD_MS);

                // Apply parameters with mutex protection
                portENTER_CRITICAL(&mutex);
                minVoltage = cutoff_voltage;
                portEXIT_CRITICAL(&mutex);
                digitalWrite(DUT_PWR, HIGH);
                break;
            }
            Serial.flush();
        }  // End of setup phase

        battSetup = false;

        // ===== MEASUREMENT PHASE =====
        if (!battSetup) {
            setup_oled();  // Prepare OLED for measurement display
        }

        // Ensure DAC/current state is reset for a fresh test
        portENTER_CRITICAL(&mutex);
        DAC = 0;
        portEXIT_CRITICAL(&mutex);
        dac.write(0);

        // Clear any accumulated current history so ramp starts from zero
        avgCurrent.reset();

        digitalWrite(NFET_OFF, LOW);  // Turn on NFETs
        nfetState = !digitalRead(NFET_OFF);

        // Calculate approximate target DAC for smooth ramp-up
        // Prevents current spike by ramping: 25% -> 50% -> 75% -> 100%
        // maxCurrent_10A (64000) corresponds to 10.2A (10200mA)
        long target_DAC = (long)((target_mA * 64000.0) / 10200.0);
        if (target_DAC > 64000) target_DAC = 64000;

        // 4-step ramp-up to avoid current spike (total ~200ms)
        portENTER_CRITICAL(&mutex);
        DAC = target_DAC / 4;  // 25%
        portEXIT_CRITICAL(&mutex);
        dac.write(DAC);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        portENTER_CRITICAL(&mutex);
        DAC = target_DAC / 2;  // 50%
        portEXIT_CRITICAL(&mutex);
        dac.write(DAC);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        portENTER_CRITICAL(&mutex);
        DAC = (target_DAC * 3) / 4;  // 75%
        portEXIT_CRITICAL(&mutex);
        dac.write(DAC);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        portENTER_CRITICAL(&mutex);
        DAC = target_DAC;  // 100%
        portEXIT_CRITICAL(&mutex);
        dac.write(DAC);
        vTaskDelay(50 / portTICK_PERIOD_MS);

        mAh_soFar = 0.0;
        startMillisec = millis();
        last_hours = 0;
        stop_oled_vars = false;
        in_recovery_mode = false;
        recovery_start_millis = 0;
        millis_PC_wait = millis();  // Initialize - delay first data packet by full sample interval
        millisCalc_mAh = millis();  // Initialize mAh calculation timer

        // Main measurement loop
        while (!end_of_test && batteryModeActive && appPresence) {
            // Read shared variables with mutex protection
            // Use filtered voltage (dispVoltage) for cutoff detection to avoid jitter
            double local_dutV, local_dispCurrent;
            portENTER_CRITICAL(&mutex);
            local_dutV = dispVoltage;         // Filtered with 16-sample moving average
            local_dispCurrent = dispCurrent;  // Filtered with 16-sample moving average
            portEXIT_CRITICAL(&mutex);

            // ===== CURRENT REGULATION (Multi-stage fixed-step control) =====
            // Calculate current error in mA (target vs actual)
            long actual_mA = (long)(local_dispCurrent * 1000);
            long current_error_mA = target_mA - actual_mA;  // Positive = need more current
            long current_error_abs = abs(current_error_mA);

            // Multi-stage step sizes based on error magnitude
            int step = 0;
            if (current_error_abs > 200) {
                step = 100;  // Large error: aggressive correction
            } else if (current_error_abs > 50) {
                step = 50;  // Medium error: moderate correction
            } else if (current_error_abs > 10) {
                step = 20;  // Small error: gentle correction
            } else if (current_error_abs > 2) {
                step = 5;  // Very small error: fine correction
            }
            // else: within 2mA deadband, no adjustment needed

            // Apply correction
            portENTER_CRITICAL(&mutex);
            if (current_error_mA > 0 && step > 0) {
                // Need more current - increase DAC
                DAC = DAC + step;
            } else if (current_error_mA < 0 && step > 0) {
                // Too much current - decrease DAC
                if (DAC > step) {
                    DAC = DAC - step;
                } else {
                    DAC = 0;
                }
            }

            // Clamp DAC to valid range (0 to 64000 for 10A max)
            if (DAC > 64000) DAC = 64000;
            portEXIT_CRITICAL(&mutex);
            dac.write(DAC);

            // Check cutoff voltage - enter recovery mode
            if ((!end_of_test) && (!in_recovery_mode) && (local_dutV < cutoff_voltage)) {
                digitalWrite(NFET_OFF, HIGH);  // Turn off load
                nfetState = !digitalRead(NFET_OFF);
                in_recovery_mode = true;
                recovery_start_millis = millis();
                cutoff_voltage_reached = true;
                Serial.print("MSGSTCutoff Voltage - Recovery StartedMSGEND");
                // Don't set end_of_test - continue monitoring during recovery
            }

            // Check recovery timeout
            if (in_recovery_mode && !end_of_test) {
                unsigned long recovery_elapsed_ms = millis() - recovery_start_millis;
                unsigned long recovery_timeout_ms = (unsigned long)recovery_time_minutes * 60000;

                if (recovery_elapsed_ms >= recovery_timeout_ms) {
                    Serial.print("MSGSTRecovery CompletedMSGEND");
                    termination_message_sent = true;
                    end_of_test = true;
                }
            }

            // Check current overshoot (>25%) - only during active discharge, not during recovery
            if ((!end_of_test) && (!in_recovery_mode) && ((local_dispCurrent * 1000) > (target_mA * 1.25))) {
                Serial.print("MSGSTError - High mAMSGEND");
                termination_message_sent = true;
                end_of_test = true;
            }

            // Check time limit
            if ((!end_of_test) && (tMins > time_limit)) {
                Serial.print("MSGSTTime ExceededMSGEND");
                termination_message_sent = true;
                timed_out = true;
                end_of_test = true;
            }

            // Check communication watchdog (60 second timeout)
            // Aborts test if PC app disconnects or crashes
            if ((!end_of_test) && (millis() - lastSerialActivity > COMM_WATCHDOG_TIMEOUT)) {
                stop_oled_vars = true;
                Serial.print("MSGSTComm Timeout - PC DisconnectedMSGEND");
                termination_message_sent = true;
                Serial.flush();

                tft.fillScreen(BLACK);
                tft.setTextColor(WHITE);
                tft.setCursor(digit_1, line_2);
                tft.print("Battery Test");
                tft.setCursor(digit_1, line_4);
                tft.print("Comm Timeout");
                tft.setCursor(digit_1, line_5);
                tft.print("PC Disconnected");
                vTaskDelay(3000 / portTICK_PERIOD_MS);

                end_of_test = true;
            }

            // Check for manual cancellation or heartbeat
            if (Serial.available() > 0) {
                // Update communication watchdog - received data from PC (cancel or heartbeat)
                lastSerialActivity = millis();

                // Read incoming data - could be cancel (999) or heartbeat ("HB")
                String cmd = Serial.readStringUntil('\n');
                cmd.trim();

                // Check for cancel command
                if (cmd == "999" || cmd.toInt() == 999) {
                    stop_oled_vars = true;
                    Serial.print("MSGSTUser cancelledMSGEND");
                    termination_message_sent = true;
                    Serial.flush();

                    tft.fillScreen(BLACK);
                    tft.setTextColor(WHITE);
                    tft.setCursor(digit_1, line_2);
                    tft.print("Battery Test");
                    tft.setCursor(digit_1, line_4);
                    tft.print("Cancelled...");
                    vTaskDelay(2000 / portTICK_PERIOD_MS);

                    end_of_test = true;
                }
                // Silently ignore heartbeat ("HB") and other commands
            }

            // Update PC app with data
            if (!end_of_test) {
                getTime();

                // Calculate mAh (read filtered current with protection)
                if (millis() > millisCalc_mAh + 1000) {
                    double this_hours = (millis() - startMillisec) / (1000.0 * 3600.0);

                    portENTER_CRITICAL(&mutex);
                    double current_filtered = dispCurrent;
                    portEXIT_CRITICAL(&mutex);

                    mAh_soFar = mAh_soFar + ((this_hours - last_hours) * current_filtered * 1000);
                    last_hours = this_hours;
                    millisCalc_mAh = millis();
                }

                // Send data at sample interval
                if (millis() > millis_PC_wait + sampleTime) {
                    write_to_pc();
                    millis_PC_wait = millis();
                }
            }

            // Reset watchdog during measurement
            esp_task_wdt_reset();

            if (!end_of_test) {
                vTaskDelay(sampleTime / portTICK_PERIOD_MS);
            }
        }  // End of measurement loop

        // ===== POST-MEASUREMENT CLEANUP =====

        // If test ended, prepare for restart
        if (end_of_test && batteryModeActive) {
            digitalWrite(NFET_OFF, HIGH);
            nfetState = !digitalRead(NFET_OFF);
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            stop_oled_vars = true;
            tft.fillScreen(BLACK);
            tft.setTextColor(WHITE);
            tft.setCursor(digit_1, line_2);
            tft.print("Battery Test");
            tft.setCursor(digit_1, line_4);
            tft.print("Restarting...");
            vTaskDelay(2000 / portTICK_PERIOD_MS);

            battSetup = true;  // Run setup again
        }

        // If mode changed, exit gracefully
        if (!batteryModeActive) {
            // Ensure NFETs are turned off when exiting battery mode (manual exit via button)
            digitalWrite(NFET_OFF, HIGH);
            nfetState = false;

            // Only send mode ended message if no other termination message was sent
            if (!termination_message_sent) {
                Serial.print("MSGSTBT Mode EndedMSGEND");
                Serial.flush();
                Serial.println("");
            }

            stop_oled_vars = false;

            // Write shared variables with mutex protection
            portENTER_CRITICAL(&mutex);
            encoderPos = 0;
            DAC = 0;
            minVoltage = 1.0;
            portEXIT_CRITICAL(&mutex);
            // Note: mode should be changed by the main loop, not here

            // Remove from watchdog before suspending
            esp_task_wdt_delete(NULL);
            Serial.println("- Battery task removed from watchdog");

            vTaskSuspend(NULL);  // Suspend until reactivated
        }
    }  // End of task while loop
}

//=============================================================================
// HELPER FUNCTIONS
//=============================================================================

/**
 * @brief Send measurement data to PC app
 *
 * Format: GRAPHVS<days>:<hours>:<mins>:<secs>!<mAh>!<current>!<voltage>GRAPHVEND
 * Current sent as integer to avoid regional decimal issues.
 */
void write_to_pc() {
    // Read shared variables with mutex protection
    double local_dutV, local_shuntV;
    portENTER_CRITICAL(&mutex);
    local_dutV = dutV;
    local_shuntV = shuntV;
    portEXIT_CRITICAL(&mutex);

    String message = "GRAPHVS";
    message += days;
    message += ":";
    if (hours < 10) message += "0";
    message += hours;
    message += ":";
    if (mins < 10) message += "0";
    message += mins;
    message += ":";
    if (secs < 10) message += "0";
    message += secs;
    message += "!";
    message += mAh_soFar;
    message += "!";
    message += (int)(local_shuntV * 1000);
    message += "!";
    message += local_dutV;
    message += "GRAPHVEND";

    Serial.print(message);
    Serial.flush();
}

/**
 * @brief Calculate elapsed time
 *
 * Calculates days, hours, minutes, and seconds from start of test.
 */
void getTime() {
    const long day = 86400000;  // Milliseconds in a day
    const long hour = 3600000;  // Milliseconds in an hour
    const long minute = 60000;  // Milliseconds in a minute
    const long second = 1000;   // Milliseconds in a second

    unsigned long timeNow = millis() - startMillisec;
    tMins = timeNow / minute;
    days = timeNow / day;
    hours = (timeNow % day) / hour;
    mins = ((timeNow % day) % hour) / minute;
    secs = (((timeNow % day) % hour) % minute) / second;
}
