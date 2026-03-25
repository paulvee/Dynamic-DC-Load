/**
 * @file BatteryMode.cpp
 * @brief Battery discharge test mode implementation
 * @author Paul Versteeg
 * @date 2026
 *
 * Based on design from John Lowen <john@vwlowen.co.uk>
 * http://www.vwlowen.co.uk/arduino/battery-tester/battery-tester.htm
 *
 * See blog: https://www.paulvdiyblogs.net/2019/03/a-pretty-universal-battery-cell-tester.html
 *
 * Because John is no longer with us, we have redesigned and ported the code from Delphi-7 Pascal
 * to Python so it's easier to maintain and extend by us. There have been many changes to this code,
 * like added battaery chemistries, automated setting the measurement up, added a recovery mode,
 * added failsafes autoranging graphics, and the app can now start the Dynamic Load directly in
 * the BT mode.
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
    // boot message with core info
    Serial.printf("- Battery mode task started, running on core %d\r\n", xPortGetCoreID());

    Serial.println("- Battery task suspended by itself");
    vTaskSuspend(NULL);  // Wait until resumed by mode selection
    Serial.println("- Battery mode task resumed");

    // Subscribe to watchdog when battery mode becomes active
    // this will terminate the DL when the communication fails or the app has been terminated
    // to prevent the DL from running indefinitely and potentially damaging the battery or device
    // under test
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
                    // that the AUTO_BT command is used when the app starts the DL directly in battery
                    // mode, so it needs to be handled before reading parameters
                    if (Serial.peek() == 'A') {  // Might be "AUTO_BT"
                        String cmd = Serial.readStringUntil('\n');
                        cmd.trim();
                        if (cmd == "AUTO_BT") {
                            Serial.println("ACK_BT");
                            Serial.flush();  // Ensure ACK_BT is transmitted immediately
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

                    // Read parameters with periodic watchdog resets
                    // (each parseInt/parseFloat can block up to 1 second)
                    target_mA = Serial.parseInt();
                    esp_task_wdt_reset();
                    cutoff_voltage = Serial.parseFloat();
                    esp_task_wdt_reset();
                    time_limit = Serial.parseInt();
                    esp_task_wdt_reset();
                    sampleTime = Serial.parseInt() * 1000;
                    esp_task_wdt_reset();
                    recovery_time_minutes = Serial.parseInt();  // Recovery monitoring time
                    esp_task_wdt_reset();
                    cancel = Serial.parseInt();  // 0 at start, 999 to abort
                    esp_task_wdt_reset();

                    // Validate parameters
                    if (target_mA <= 0 || target_mA > 10000) target_mA = 100;                                // Default 100mA
                    if (cutoff_voltage < 0.5 || cutoff_voltage > 100) cutoff_voltage = 0.5;                  // Default 0.5V minimum
                    if (sampleTime < 1000) sampleTime = 1000;                                                // Minimum 1 second
                    if (recovery_time_minutes < 1 || recovery_time_minutes > 30) recovery_time_minutes = 5;  // Default 5 minutes
                }
                Serial.flush();

                // Display received values for 2 seconds
                tft.fillScreen(BLACK);
                tft.setCursor(digit_1, line_2);
                tft.print("Cur: ");
                tft.print(target_mA);
                tft.print(" mA");
                tft.setCursor(digit_1, line_3);
                tft.print("Cut: ");
                tft.print(cutoff_voltage, 2);
                tft.print(" V");
                tft.setCursor(digit_1, line_4);
                tft.print("T-limit: ");
                tft.print(time_limit);
                tft.print(" s");
                tft.setCursor(digit_1, 80);
                tft.print("RecTime: ");
                tft.print(recovery_time_minutes);
                tft.print("m");

                esp_task_wdt_reset();  // Reset before 2s delay
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

        Serial.println("DEBUG: Starting baseline monitoring");

        // Initialize timing counter for entire test (baseline + ramp-up + measurement)
        startMillisec = millis();
        mAh_soFar = 0.0;
        last_hours = 0;
        stop_oled_vars = false;
        in_recovery_mode = false;
        recovery_start_millis = 0;
        millisCalc_mAh = millis();  // Initialize mAh calculation timer

        // ===== 5-SECOND BASELINE MONITORING (NO LOAD) =====
        // Monitor cell voltage for 5 seconds before applying load
        // Provides baseline voltage reading and allows battery to stabilize
        Serial.print("MSGSTBaseline monitoring (5s)...MSGEND");
        unsigned long baseline_start = millis();
        unsigned long last_baseline_send = millis();

        while ((millis() - baseline_start) < 5000) {
            // Read voltage without load
            double local_dutV;
            portENTER_CRITICAL(&mutex);
            local_dutV = dutV;  // Unfiltered voltage for baseline
            portEXIT_CRITICAL(&mutex);

            // Send baseline data to PC every 500ms with proper timestamps
            if (millis() - last_baseline_send >= 500) {
                getTime();
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
                message += "!0!0!";
                message += local_dutV;
                message += "GRAPHVEND";
                Serial.print(message);
                last_baseline_send = millis();
            }

            // Reset watchdog to prevent timeout during baseline monitoring
            esp_task_wdt_reset();

            vTaskDelay(50 / portTICK_PERIOD_MS);

            // Check for test cancellation during baseline
            if (!batteryModeActive || !appPresence) {
                Serial.print("MSGSTBaseline monitoring cancelled.MSGEND");
                end_of_test = true;
                break;
            }
        }

        Serial.println("DEBUG: Baseline complete");
        Serial.print("DEBUG: end_of_test = ");
        Serial.println(end_of_test);

        // Only proceed to ramp-up if test not cancelled
        if (!end_of_test) {
            Serial.print("MSGSTStarting current ramp-up...MSGEND");
            Serial.println("DEBUG: Entering ramp-up");
        } else {
            Serial.println("DEBUG: Skipping ramp-up, end_of_test = true");
        }

        // Calculate approximate target DAC for smooth discharge current ramp-up
        // Prevents current spike by ramping: 25% -> 50% -> 75% -> 100%
        // maxCurrent_10A (64000) corresponds to 10.2A (10200mA)
        // Further down and in the main code we check that we stay within the maximum
        // Power limits, so we can allow the user to set a target current up to 8A, but the DAC
        // will be clamped to 4A when the total voltage exceeds 40V to stay within
        // the 150W hardware limit
        long target_DAC = (long)((target_mA * 64000.0) / 10200.0);
        if (target_DAC > 64000) target_DAC = 64000;

        // 4-step ramp-up to avoid current spike (500ms per step = 2 seconds total)
        // Send data packet after each step so ramp is visible in graph
        // Check cutoff voltage during ramp-up to prevent damage to weak batteries
        // Only execute if test not cancelled during baseline
        if (!end_of_test) {
            // Step 1: 25%
            portENTER_CRITICAL(&mutex);
            DAC = target_DAC / 4;
            portEXIT_CRITICAL(&mutex);
            dac.write(DAC);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            getTime();
            write_to_pc();
            esp_task_wdt_reset();

            // Check cutoff voltage after step 1
            double local_dutV_step1;
            portENTER_CRITICAL(&mutex);
            local_dutV_step1 = dispVoltage;
            portEXIT_CRITICAL(&mutex);
            if (local_dutV_step1 < cutoff_voltage) {
                Serial.print("MSGSTCutoff during ramp-up (step 1) - Aborting testMSGEND");
                end_of_test = true;
            }
        }

        if (!end_of_test) {
            // Step 2: 50%
            portENTER_CRITICAL(&mutex);
            DAC = target_DAC / 2;
            portEXIT_CRITICAL(&mutex);
            dac.write(DAC);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            getTime();
            write_to_pc();
            esp_task_wdt_reset();

            // Check cutoff voltage after step 2
            double local_dutV_step2;
            portENTER_CRITICAL(&mutex);
            local_dutV_step2 = dispVoltage;
            portEXIT_CRITICAL(&mutex);
            if (local_dutV_step2 < cutoff_voltage) {
                Serial.print("MSGSTCutoff during ramp-up (step 2) - Aborting testMSGEND");
                end_of_test = true;
            }
        }

        if (!end_of_test) {
            // Step 3: 75%
            portENTER_CRITICAL(&mutex);
            DAC = (target_DAC * 3) / 4;
            portEXIT_CRITICAL(&mutex);
            dac.write(DAC);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            getTime();
            write_to_pc();
            esp_task_wdt_reset();

            // Check cutoff voltage after step 3
            double local_dutV_step3;
            portENTER_CRITICAL(&mutex);
            local_dutV_step3 = dispVoltage;
            portEXIT_CRITICAL(&mutex);
            if (local_dutV_step3 < cutoff_voltage) {
                Serial.print("MSGSTCutoff during ramp-up (step 3) - Aborting testMSGEND");
                end_of_test = true;
            }
        }

        if (!end_of_test) {
            // Step 4: 100%
            portENTER_CRITICAL(&mutex);
            DAC = target_DAC;
            portEXIT_CRITICAL(&mutex);
            dac.write(DAC);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            getTime();
            write_to_pc();
            esp_task_wdt_reset();

            // Check cutoff voltage after step 4
            double local_dutV_step4;
            portENTER_CRITICAL(&mutex);
            local_dutV_step4 = dispVoltage;
            portEXIT_CRITICAL(&mutex);
            if (local_dutV_step4 < cutoff_voltage) {
                Serial.print("MSGSTCutoff during ramp-up (step 4) - Aborting testMSGEND");
                end_of_test = true;
            } else {
                Serial.print("DEBUG: Ramp-up complete, DAC = ");
                Serial.println(DAC);
            }
        }

        millis_PC_wait = millis();  // Initialize for regular measurement loop

        // Initialize hardware regulation state for new test
        bool power_limit_active = false;
        long last_effective_target_mA = -1;  // Force DAC update on first iteration

        // Main measurement loop
        while (!end_of_test && batteryModeActive && appPresence) {
            // Read shared variables with mutex protection
            // Use filtered voltage (dispVoltage) for cutoff detection to avoid glitches and jitter
            double local_dutV, local_dispCurrent;
            portENTER_CRITICAL(&mutex);
            local_dutV = dispVoltage;         // Filtered with 16-sample moving average
            local_dispCurrent = dispCurrent;  // Filtered with 16-sample moving average
            portEXIT_CRITICAL(&mutex);

            // ===== POWER LIMIT PROTECTION (150W hardware limit) =====
            // Enforce current limit based on voltage to prevent exceeding 150W
            // Below 40V: allow up to 8A (8000mA)
            // Above 40V: limit to 4A (4000mA) to stay under 160W
            // Hardware feedback loop regulates current - we only set DAC when power limit changes

            long effective_target_mA = target_mA;
            if (local_dutV >= 40.0 && target_mA > 4000) {
                effective_target_mA = 4000;
                if (!power_limit_active) {
                    Serial.println("Power limit: Current reduced to 4A (voltage >= 40V)");
                    power_limit_active = true;
                }
            } else {
                if (power_limit_active && local_dutV < 39.0) {
                    // Hysteresis: restore full current when voltage drops below 39V
                    Serial.println("Power limit: Current restored");
                    power_limit_active = false;
                }
            }

            // ===== HARDWARE CURRENT REGULATION =====
            // Update DAC only when power limit changes (like CC mode with rotary encoder)
            // Hardware feedback loop maintains the current automatically
            if (effective_target_mA != last_effective_target_mA) {
                long target_DAC = (long)((effective_target_mA * 64000.0) / 10200.0);
                if (target_DAC > 64000) target_DAC = 64000;

                portENTER_CRITICAL(&mutex);
                DAC = target_DAC;
                portEXIT_CRITICAL(&mutex);
                dac.write(DAC);

                last_effective_target_mA = effective_target_mA;
            }

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
            // Use effective_target_mA to account for power limiting
            if ((!end_of_test) && (!in_recovery_mode) && ((local_dispCurrent * 1000) > (effective_target_mA * 1.25))) {
                Serial.print("MSGSTError - High mAMSGEND");
                termination_message_sent = true;
                end_of_test = true;
            }

            // Check time limit (only during discharge, not during recovery)
            if ((!end_of_test) && (!in_recovery_mode) && (tMins > time_limit)) {
                Serial.print("MSGSTTime ExceededMSGEND");
                termination_message_sent = true;
                timed_out = true;
                end_of_test = true;
            }

            // Check communication watchdog (60 second timeout)
            // Aborts test if PC app disconnects, is terminated or crashes
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

            // Check for manual stop, cancellation, or heartbeat
            if (Serial.available() > 0) {
                // Update communication watchdog - received data from PC
                lastSerialActivity = millis();

                // Read incoming data - could be "STOP", "999" (cancel), or "HB" (heartbeat)
                String cmd = Serial.readStringUntil('\n');
                cmd.trim();

                // Check for stop command - enter recovery mode gracefully
                if (cmd == "STOP") {
                    // Only process if not already in recovery mode
                    if (!in_recovery_mode) {
                        digitalWrite(NFET_OFF, HIGH);  // Turn off load
                        nfetState = !digitalRead(NFET_OFF);
                        in_recovery_mode = true;
                        recovery_start_millis = millis();
                        Serial.print("MSGSTUser stopped - Recovery StartedMSGEND");
                        // Don't update OLED - CC display shows BT OFF status automatically
                        // Don't set end_of_test - continue monitoring during recovery
                    }
                }
                // Check for cancel command - immediate termination
                else if (cmd == "999" || cmd.toInt() == 999) {
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
 * Current sent as integer to avoid regional (. or ,)decimal issues.
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
