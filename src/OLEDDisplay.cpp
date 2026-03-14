/**
 * @file OLEDDisplay.cpp
 * @brief OLED display update functions for SSD1351 128x128 RGB display
 * @author Paul Versteeg
 * @date 2026
 *
 * This task runs on Core 1 (app_cpu) and updates the display every 100ms.
 * Critical measurements are shown immediately, less critical values every 500ms.
 */

#include "OLEDDisplay.h"

#include "BatteryMode.h"
#include "Config.h"
#include "DynamicLoad.h"

//=============================================================================
// LOCAL DISPLAY VARIABLES
//=============================================================================

// Update rate control
int slower_update = 0;  // Counter to delay less critical readings

// Local copies of globals to minimize cross-core access
double _encoderPos = 0;
int _encoderRes = 0;
double _DAC = 0;
double _voltage = 0;
double prev_voltage = 0.01;
double _current = 0;
double prev_current = 0.01;
double _dutPower = 0;
double prev_dutPower = 0.01;
double _temperature = 0;

//=============================================================================
// FORWARD DECLARATIONS
//=============================================================================

void send_encoder();
void send_dac();
void send_neg_pol();
void send_volt();
void send_current();
void send_power();
void send_temp();
void send_state();
void send_mode();
void draw_degree_symbol();
String lltoString(unsigned long ll);

//=============================================================================
// OLED UPDATE TASK (Core 1)
//=============================================================================

/**
 * @brief RTOS task for updating OLED display
 *
 * Updates display every 100ms with critical values (encoder, DAC, state, mode).
 * Updates less critical values (voltage, current, power, temp) every 500ms.
 *
 * @param pvParameters Unused RTOS parameter
 */
void updateOledDisplay(void* pvParameters) {
    Serial.print("- Display task started, running on core ");
    Serial.println(xPortGetCoreID());

    while (true) {
        // Copy global variables from main core to minimize access time
        portENTER_CRITICAL(&mutex);
        _encoderPos = encoderPos;
        _encoderRes = encoderRes;
        _DAC = DAC;
        portEXIT_CRITICAL(&mutex);

        // Do not display regular values during battery mode setup
        if (!stop_oled_vars) {
            digitalWrite(LED, !digitalRead(LED));  // Heartbeat indicator

            // Display critical values every cycle
            send_encoder();
            send_dac();

            nfetState = !digitalRead(NFET_OFF);
            send_state();
            send_mode();

            // Display less critical values every 500ms (5 cycles)
            if (slower_update == 5) {
                // Copy remaining values
                portENTER_CRITICAL(&mutex);
                _voltage = dispVoltage;
                _current = dispCurrent;
                _dutPower = dutPower;
                _temperature = temperature;
                portEXIT_CRITICAL(&mutex);

                if (_voltage < 0) {
                    // Warn about negative voltages (reverse polarity)
                    send_neg_pol();
                } else {
                    send_volt();
                }

                send_current();
                send_power();
                send_temp();

                slower_update = 0;
            } else {
                slower_update++;
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);  // Update every 100ms
    }
}

//=============================================================================
// DISPLAY FUNCTIONS
//=============================================================================

/**
 * @brief Display encoder setting converted to appropriate units
 *
 * Shows set point in:
 * - CC mode: mA (integer)
 * - CV mode: V (2 decimals)
 * - CP mode: W (1 decimal)
 * - CR mode: R (1 or 2 decimals)
 * - BT mode: mA (target current)
 */
void send_encoder() {
    double displayVal = 0;
    int suffix = 0;  // Number of decimal digits

    switch (mode) {
        case current:                                // Constant Current Mode
            displayVal = _encoderPos * _encoderRes;  // 10mA per click
            suffix = 0;
            break;
        case voltage:                          // Constant Voltage Mode
            displayVal = _encoderPos / 100.0;  // 10mV per click
            suffix = 2;
            break;
        case power:                           // Constant Power Mode
            displayVal = _encoderPos / 10.0;  // 100mW per click
            suffix = 1;
            break;
        case resistance:  // Constant Resistance Mode
            if (_encoderPos >= 100) {
                suffix = 1;  // 1 decimal above 10R
            } else {
                suffix = 2;  // 2 decimals below 10R
            }
            displayVal = _encoderPos / 10.0;  // 100mR per click
            break;
        case battery:  // Battery Mode
            displayVal = target_mA;
            suffix = 0;
            break;
    }

    // Clear field for value area
    tft.fillRect(digit_3, s_line + 3, 65, 17, BLACK);
    tft.setTextColor(BLUE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);

    // Mode-specific position offset
    // CC: shift right (+10px), CV: shift left (-10px), CP/CR: no change
    int x_offset = 0;
    if (mode == current) {
        x_offset = 10;  // Shift right one char position
    } else if (mode == voltage) {
        x_offset = -10;  // Shift left one char position
    }

    // Display the encoder value with mode-specific positioning
    if (displayVal >= 10000) {
        tft.setCursor(digit_3 + x_offset, s_line + 17);
    } else if (displayVal >= 1000) {
        tft.setCursor(digit_4 + x_offset, s_line + 17);
    } else if (displayVal >= 100) {
        tft.setCursor(digit_5 + x_offset, s_line + 17);
    } else if (displayVal >= 10) {
        tft.setCursor(digit_6 + x_offset, s_line + 17);
    } else {
        // < 10
        tft.setCursor(digit_7 + x_offset, s_line + 17);
    }
    tft.print(displayVal, suffix);
}

/**
 * @brief Helper to convert unsigned long to string
 * @param ll Value to convert
 * @return String representation
 */
String lltoString(unsigned long ll) {
    String result = "";
    do {
        result = String((int)(ll % 10)) + result;
        ll /= 10;
    } while (ll != 0);
    return result;
}

/**
 * @brief Display DAC setting (for debugging)
 * Shows current DAC value (0-65535)
 */
void send_dac() {
    tft.fillRect(digit_0, m_line + 5, 55, 17, BLACK);
    tft.setTextColor(WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);

    String number = lltoString(_DAC);
    tft.setCursor(digit_0, m_line + 20);
    tft.print(number);
}

/**
 * @brief Display warning for invalid/negative voltage
 * Indicates reverse polarity or no DUT connected
 */
void send_neg_pol() {
    tft.setTextSize(1);
    tft.fillRect(0, v_line - 25, 112, 27, BLACK);

    tft.setCursor(ldigit_2 + 10, v_line);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(RED);
    tft.print("INVALID");
}

/**
 * @brief Display DUT voltage with dynamic formatting
 *
 * Shows voltage in red if below minVoltage threshold.
 * Automatically adjusts position and decimals based on magnitude.
 */
void send_volt() {
    if (prev_voltage != _voltage) {  // Only update on change
        prev_voltage = _voltage;

        tft.setTextColor(BLUE);
        tft.setFont(&FreeSans18pt7b);
        tft.setTextSize(1);
        tft.fillRect(0, v_line - 25, 112, 27, BLACK);

        // Round to display precision to fix positioning logic
        double rounded_v = round(_voltage * 1000) / 1000;

        if (_voltage < minVoltage) {
            // Warning color for low voltage
            tft.setTextColor(RED);
            tft.setCursor(ldigit_2, v_line);
            tft.print(String(_voltage, 3));
        } else if (rounded_v < 10) {
            // < 10V: shift right
            tft.setCursor(ldigit_2, v_line);
            tft.print(String(_voltage, 3));
        } else if (rounded_v < 100) {
            // 10V - 99.9V
            tft.setCursor(ldigit_1, v_line);
            tft.print(String(_voltage, 3));
        } else {
            // >= 100V: reduce decimals
            tft.setCursor(ldigit_1, v_line);
            tft.print(String(_voltage, 2));
        }

        // Redraw "V" suffix (kludge for rounding precision issues)
        tft.fillRect(110, v_line - 25, 28, 27, BLACK);
        tft.setTextColor(WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);
        tft.setCursor(sdigit_6, v_line);
        tft.println("V");
    }
}

/**
 * @brief Display DUT current with dynamic formatting
 *
 * Automatically adjusts position and decimals based on magnitude.
 * Negative values are clamped to zero.
 */
void send_current() {
    if (prev_current != _current) {  // Only update on change
        prev_current = _current;

        tft.setTextColor(GREEN);
        tft.setFont(&FreeSans18pt7b);
        tft.setTextSize(1);
        tft.fillRect(0, a_line - 25, 112, 27, BLACK);

        if (_current < 0) {
            _current = 0;  // Negative values mess up the display
        }

        // Round to display precision to fix positioning logic
        double rounded_a = round(_current * 1000) / 1000;

        if (rounded_a >= 10) {
            // >= 10A
            tft.setCursor(ldigit_1, a_line);
            tft.print(String(_current, 3));
        } else {
            // < 10A: shift right
            tft.setCursor(ldigit_2, a_line);
            tft.print(String(_current, 3));
        }

        // Redraw "A" suffix (kludge for rounding precision issues)
        tft.fillRect(110, a_line - 25, 28, 27, BLACK);
        tft.setTextColor(WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);
        tft.setCursor(sdigit_6, a_line);
        tft.println("A");
    }
}

/**
 * @brief Display power dissipation with color warnings
 *
 * - White: < 100W
 * - Yellow: 100-150W (heads-up)
 * - Red: > 150W (warning)
 */
void send_power() {
    if (prev_dutPower != _dutPower) {  // Only update on change
        prev_dutPower = _dutPower;

        tft.fillRect(digit_6 + 6, p_line + 3, 61, 17, BLACK);
        tft.setTextColor(WHITE);
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);

        // Color warnings for high power
        if (_dutPower > 100) {
            tft.setTextColor(YELLOW);  // Heads-up
        }
        if (_dutPower > 150) {
            tft.setTextColor(BR_RED);  // Warning
        }

        // Display with appropriate decimals
        tft.setCursor(digit_7, p_line + 17);
        if (_dutPower < 10) {
            tft.print(String(_dutPower, 2));  // 2 decimals below 10W
        } else if (_dutPower < 100) {
            tft.print(String(_dutPower, 1));  // 1 decimal 10-99W
        } else {
            tft.print(String(_dutPower, 0));  // No decimals above 100W
        }

        // Redraw "W" suffix
        tft.setTextColor(WHITE);
        tft.setCursor(sdigit_6 - 4, p_line + 17);
        tft.print("W");
    }
}

/**
 * @brief Display heatsink temperature with color warnings
 *
 * - White: < 50°C
 * - Yellow: 50-79°C (heads-up)
 * - Red: >= 80°C (warning)
 */
void send_temp() {
    tft.fillRect(digit_9, m_line + 5, 32, 17, BLACK);

    // Color warnings for temperature
    if (_temperature >= 60) {
        tft.setTextColor(YELLOW);  // Heads-up
    }
    if (_temperature >= 80) {
        tft.setTextColor(BR_RED);  // Alarming
    }
    if (_temperature < 50) {
        tft.setTextColor(WHITE);
    }

    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);

    // Shift left if > 99°C
    if (_temperature > 99) {
        tft.setCursor(digit_9, m_line + 20);
    } else {
        tft.setCursor(digit_10, m_line + 20);
    }

    tft.print(_temperature, 0);
}

/**
 * @brief Display output ON/OFF status
 * Shows NFET state in white (OFF) or yellow (ON)
 */
void send_state() {
    tft.fillRect(digit_3, p_line + 2, 35, 20, BLACK);
    tft.setTextColor(WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setCursor(digit_3, p_line + 17);

    if (!nfetState) {
        tft.print("OFF");
    } else {
        tft.setTextColor(YELLOW);
        tft.print("ON");
    }
}

/**
 * @brief Draw degree symbol for temperature display
 * Creates a 3x3 pixel square with hollow center
 */
void draw_degree_symbol() {
    // Draw 3x3 square
    for (int h = 124; h <= 126; h++) {
        for (int v = m_line + 8; v <= m_line + 10; v++) {
            tft.drawPixel(h, v, WHITE);
        }
    }
    // Hollow out the center
    tft.drawPixel(125, m_line + 9, BLACK);
}

/**
 * @brief Display current operating mode and set point suffix
 * Shows CC, CV, CP, CR, or BT at left of power line
 */
void send_mode() {
    tft.fillRect(0, p_line + 2, 30, 20, BLACK);
    tft.setTextColor(WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setCursor(0, p_line + 17);
    tft.print(modeStrings[mode]);

    // Update SET encoder suffix
    // Moved one char pos right to avoid clobbering decimals
    tft.fillRect(digit_9 + 6, s_line + 3, 40, 17, BLACK);
    // Different positioning for each mode
    if (mode == current || mode == battery) {
        tft.setCursor(digit_8 + 19, s_line + 17);  // mA: 1px more (was 18, now 19)
    } else if (mode == voltage) {
        tft.setCursor(digit_9 + 25, s_line + 17);  // V: 10px more (was 15, now 25) = 115
    } else if (mode == power) {
        tft.setCursor(digit_9 + 19, s_line + 17);  // W: 4px more (was 15, now 19) = 109
    } else if (mode == resistance) {
        tft.setCursor(digit_9 + 23, s_line + 17);  // R: 8px more (was 15, now 23) = 113
    }
    tft.print(suffixStrings[mode]);

    send_power();
}

//=============================================================================
// INITIALIZATION FUNCTIONS
//=============================================================================

/**
 * @brief Setup OLED for measurement display mode
 * Called at startup and when exiting battery mode
 */
void setup_oled() {
    Serial.print("setup_oled ");
    digitalWrite(NFET_OFF, HIGH);  // Turn output off

    tft.fillScreen(BLACK);

    // Clear mode & power line (important when returning from BT mode)
    tft.fillRect(digit_0, p_line + 3, 128, 17, BLACK);

    // Print "V" suffix
    tft.setTextColor(WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);
    tft.setCursor(sdigit_6, v_line);
    tft.println("V");

    // Print "A" suffix
    tft.setCursor(sdigit_6, a_line);
    tft.println("A");

    // Print "W" suffix
    tft.setCursor(sdigit_6 - 4, p_line + 17);
    tft.print("W");

    // Draw horizontal separator line
    for (int h = 0; h <= SCREEN_WIDTH; h++) {
        tft.drawPixel(h, p_line + 25, L_GREY);
    }

    // Print "Set" prefix
    tft.setCursor(digit_0, s_line + 17);
    tft.println("Set");

    // Print encoder suffix (setup)
    // Moved one char pos right to avoid clobbering decimals
    tft.fillRect(digit_9 + 6, s_line + 3, 40, 17, BLACK);
    // Different positioning for each mode
    if (mode == current || mode == battery) {
        tft.setCursor(digit_8 + 19, s_line + 17);  // mA: 1px more (was 18, now 19)
    } else if (mode == voltage) {
        tft.setCursor(digit_9 + 25, s_line + 17);  // V: 10px more (was 15, now 25) = 115
    } else if (mode == power) {
        tft.setCursor(digit_9 + 19, s_line + 17);  // W: 4px more (was 15, now 19) = 109
    } else if (mode == resistance) {
        tft.setCursor(digit_9 + 23, s_line + 17);  // R: 8px more (was 15, now 23) = 113
    }
    tft.print(suffixStrings[mode]);

    draw_degree_symbol();
    send_mode();

    // Copy global variables with mutex protection
    portENTER_CRITICAL(&mutex);
    _encoderPos = encoderPos;
    _encoderRes = encoderRes;
    _DAC = DAC;
    _voltage = dispVoltage;
    _current = dispCurrent;
    _dutPower = dutPower;
    _temperature = temperature;
    portEXIT_CRITICAL(&mutex);

    // Force initial display
    prev_voltage = 0.01;
    prev_current = 0.01;
    prev_dutPower = 0.01;

    empty_avg_pool();

    send_dac();
    send_volt();
    send_current();
    send_power();
    send_temp();

    Serial.println("done...");
}

/**
 * @brief Initial OLED splash screen
 * Shows welcome message with firmware version
 */
void oled_prep() {
    Serial.print("oled prep ");

    tft.fillScreen(BLACK);

    // Display welcome message
    tft.setTextColor(WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextSize(1);

    tft.setCursor(digit_2, v_line);
    tft.print(" DC Load");

    tft.setCursor(digit_0 + 5, a_line);
    tft.print("Version");

    tft.setCursor(digit_7, a_line);
    tft.print(FW_VERSION);

    tft.setCursor(digit_1, s_line);
    tft.print("Paulv & BudB");

    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Show for 2 seconds
    tft.fillScreen(BLACK);

    Serial.println("done...");
}
