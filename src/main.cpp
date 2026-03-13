/**
 * @file main.cpp
 * @brief Main entry point for Dynamic Load firmware
 * @author Paul Versteeg
 * @version 7.0.3
 * @date 2026
 *
 * This firmware runs only on an ESP32 DevKit1 with dual cores and FreeRTOS.
 *
 * Task distribution:
 * - Core 0 (main_cpu): Time-critical tasks (mainLoop, process_encoder)
 * - Core 1 (app_cpu): Display and fan control tasks
 *
 * Main loop time: ~18ms (important for CP and CR mode control)
 */

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "BatteryMode.h"
#include "Config.h"
#include "DynamicLoad.h"
#include "FanController.h"
#include "OLEDDisplay.h"
#include "RotaryEncoder.h"

// Moving average library - you'll need to add this to lib/
#include "MovingAverage.h"

// Watchdog timeout in seconds
#define WDT_TIMEOUT 5

//=============================================================================
// HARDWARE OBJECT INSTANCES
//=============================================================================

ADS1115 ADS(ADS1115_ADDRESS);
DAC8571 dac(DAC8571_ADDRESS);

Adafruit_SSD1351 tft = Adafruit_SSD1351(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &SPI,
    CS,
    DC,
    RES);

ezButton button(ENC_BUT);

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================

// Operating mode
OperatingMode mode = current;
volatile int prev_mode = current;
const char* modeStrings[] = {"CC", "CV", "CP", "CR", "BT"};
const char* suffixStrings[] = {"mA", "V", "W", "R", "mA"};

// State
bool nfetState = false;
uint8_t adcGain = 0;
uint8_t old_adcGain = 0;
int tempUpdate = 0;

// Measurements
volatile double temperature = 0;
volatile double dutV = 0;
volatile double dispVoltage = 0;
volatile long rawV = 0;
volatile long rawV_avg = 0;
volatile double shuntV = 0;
volatile double dispCurrent = 0;
volatile long shuntVraw = 0;
volatile long rawI_avg = 0;
volatile double dutPower = 0;
volatile double dutResistance = 0;

// Set points
double set_current = 0.0;
double set_voltage = 0.0;
double set_power = 0.0;
double set_resistance = 0.0;

// DAC
volatile uint16_t DAC = 0;
volatile uint16_t prev_DAC = 0;

// Regulation
double powerDelta = 0.0;
double voltDelta = 0.0;
int currentDelta = 0;

// Safety limits
long maxCurrent = 10.2;  // 10.2A
long maxVoltage = 105;   // 105V
long maxPower = 185;     // 185W
long maxTemp = 95;       // 95°C
long minVoltage = 0.7;   // 0.7V for NiCad/NiMH support

// Constants
const double voltage_ref = 4.096;
const double dc_cal_factor = 24.15;
double currOffset = 0.0;

// Battery mode variables
bool battSetup = false;
bool end_of_test = false;
int days = 0;
int hours = 0;
int mins = 0;
int secs = 0;
int tMins = 0;
double mAh_soFar = 0.0;
double current_mA = 0.0;
bool timed_out = false;
bool cutoff_voltage_reached = false;
unsigned long startMillisec = 0;
unsigned long millis_PC_wait = 0;
unsigned long millisCalc_mAh = 0;
float last_hours = 0.0;
bool batteryModeActive = false;
bool stop_oled_vars = false;
unsigned long timeout_millis = 20000;
unsigned long start = 0;
bool appPresence = true;
long target_mA = 0;
double cutoff_voltage = 0.0;
long time_limit = 0;
unsigned long sampleTime = 0;
int recovery_time_minutes = 5;
int cancel = 0;
bool in_recovery_mode = false;
unsigned long recovery_start_millis = 0;

// Communication watchdog - safety feature to abort test if PC disconnects
volatile unsigned long lastSerialActivity = 0;
const unsigned long COMM_WATCHDOG_TIMEOUT = 60000;  // 60 seconds in milliseconds

// RTOS
TaskHandle_t BThandle = NULL;
SemaphoreHandle_t xSemaphore;
portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED;

// Moving average filters (16 samples)
MovingAverage<long, 16> avgVoltage;
MovingAverage<long, 16> avgCurrent;
MovingAverage<long, 16> avgTemperature;

//=============================================================================
// SETUP FUNCTION
//=============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial);
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    Serial.print("\\n\\r\\n\\rDynamic DC Load - Version ");
    Serial.println(FW_VERSION);

    // Initialize OLED display
    Serial.println("Starting TFT");
    tft.begin();
    tft.setRotation(0);
    oled_prep();   // Splash screen
    setup_oled();  // Setup for measurements

    // Setup GPIO pins
    pinMode(LED, OUTPUT);
    pinMode(DSO_TRIG, OUTPUT);
    pinMode(DUT_PWR, OUTPUT);
    digitalWrite(DUT_PWR, LOW);
    Serial.print("Relay = ");
    Serial.println(digitalRead(DUT_PWR));

    pinMode(NFET_OFF, OUTPUT);
    digitalWrite(NFET_OFF, HIGH);  // Keep NFETs off
    nfetState = false;
    Serial.print("nfetState = ");
    Serial.println(nfetState);

    pinMode(CV_MODE, OUTPUT);
    digitalWrite(CV_MODE, LOW);  // CV mode not active

    // Initialize I2C
    Serial.print("Setup I2C, clock speed: ");
    Wire.begin();
    Wire.setClock(100000UL);  // 100kHz with 1K5 pull-ups
    Serial.println(ADS.getWireClock());

    // Setup ADS1115
    ADS.begin();
    ADS.setGain(0);  // Maximum input 6.114V
    adcGain = 0;
    ADS.setDataRate(7);  // Fastest (860 SPS)
    ADS.setMode(0);      // Continuous conversion
    ADS.readADC(2);      // Dummy read to trigger
    Serial.println("ADS1115 setup done");

    // Setup DAC8571
    dac.begin();
    Serial.print("DAC Address: ");
    Serial.println(dac.getAddress());

    // Setup rotary encoder pins
    pinMode(ENC_BUT, INPUT_PULLUP);
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
    // Note: ISR attachment moved to process_encoder task for correct core affinity

    // Setup fan controller
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, 0);  // Turn off initially
    attachInterrupt(digitalPinToInterrupt(FAN_TACHO), TachCounter_ISR, RISING);
    Serial.println("Fan setup done");

    // Set default mode
    mode = current;
    send_mode();
    send_dac();

    // Create RTOS tasks
    Serial.println("Starting tasks");
    BaseType_t xReturned;

    // Main loop task (Core 0, Priority 10)
    xReturned = xTaskCreatePinnedToCore(
        mainLoop, "mainLoop", 4096, NULL, 10, NULL, main_cpu);
    if (xReturned != pdPASS) {
        Serial.println("mainLoop task not created");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Encoder processing task (Core 0, Priority 11 - higher for immediate response)
    xReturned = xTaskCreatePinnedToCore(
        process_encoder, "process_encoder", 4096, NULL, 11, NULL, main_cpu);
    if (xReturned != pdPASS) {
        Serial.println("process_encoder task not created");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // OLED display task (Core 1, Priority 5, larger stack for graphics)
    xReturned = xTaskCreatePinnedToCore(
        updateOledDisplay, "updateOledDisplay", 8192, NULL, 5, NULL, app_cpu);
    if (xReturned != pdPASS) {
        Serial.println("updateOledDisplay task not created");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Fan controller task (Core 1, Priority 3)
    xReturned = xTaskCreatePinnedToCore(
        fanController, "fanController", 4096, NULL, 3, NULL, app_cpu);
    if (xReturned != pdPASS) {
        Serial.println("fanController task not created");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Battery mode task (Core 1, Priority 3)
    xReturned = xTaskCreatePinnedToCore(
        batteryMode, "batteryMode", 4096, NULL, 3, &BThandle, app_cpu);
    if (xReturned != pdPASS) {
        Serial.println("BatteryMode task not created");
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Configure Task Watchdog Timer for deadlock protection
    Serial.println("Configuring Watchdog Timer...");
    esp_task_wdt_init(WDT_TIMEOUT, true);  // 5 second timeout, enable panic
    esp_task_wdt_add(NULL);                // Add current task (setup) to WDT
    Serial.println("Watchdog Timer enabled");

    Serial.println("Setup finished...");
    Serial.println("Running...");

    // Remove setup task from watchdog before deletion
    esp_task_wdt_delete(NULL);

    // Delete setup task
    vTaskDelete(NULL);
}

//=============================================================================
// LOOP FUNCTION (not used with RTOS)
//=============================================================================

void loop() {
    // Empty - all functionality in RTOS tasks
}

//=============================================================================
// MAIN LOOP TASK (Core 0)
//=============================================================================

void mainLoop(void* pvParameters) {
    Serial.print("- Main task started, running on core ");
    Serial.println(xPortGetCoreID());

    // Subscribe this task to the watchdog
    esp_task_wdt_add(NULL);
    Serial.println("- Main task added to watchdog");

    // Local variables for button press detection
    unsigned long pressedTime = 0;
    unsigned long releasedTime = 0;
    bool isPressing = false;
    bool isLongDetected = false;

    while (true) {
        // Show loop time on DSO (optional)
        digitalWrite(DSO_TRIG, !digitalRead(DSO_TRIG));

        // Update button state
        button.loop();
        button.setDebounceTime(50);

        // Read measurements
        readDutV();
        readShunt();
        readTemp();
        checkValidValues();

        // Auto-switch to Battery Test mode on serial command
        // Python app sends "AUTO_BT\n" to trigger automatic mode switch
        // This doesn't interfere with firmware upload (esptool doesn't send this string)
        if (Serial.available() && mode != battery) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            if (cmd == "AUTO_BT") {
                // Update communication watchdog
                lastSerialActivity = millis();

                // Switch to battery test mode
                Serial.println("Auto-switching to Battery Test mode");

                // Reset to safe values
                portENTER_CRITICAL(&mutex);
                encoderPos = 0;
                DAC = 0;
                portEXIT_CRITICAL(&mutex);
                digitalWrite(NFET_OFF, HIGH);

                // Activate battery mode
                mode = battery;
                battSetup = true;
                batteryModeActive = true;
                vTaskResume(BThandle);

                Serial.print("New mode = ");
                Serial.println(modeStrings[mode]);
                empty_avg_pool();

                // Clear any stale data from serial buffer (like old cancel commands)
                while (Serial.available()) {
                    Serial.read();
                }

                // Send acknowledgment - DL is ready to receive parameters
                Serial.println("ACK_BT");
            }
        }

        // Handle button press events
        if (button.isPressed()) {
            pressedTime = millis();
            isPressing = true;
            isLongDetected = false;
        }

        if (button.isReleased()) {
            isPressing = false;
            releasedTime = millis();
            long pressDuration = releasedTime - pressedTime;

            if (pressDuration < SHORT_PRESS_TIME) {
                // Short press: Toggle MOSFET on/off
                rawV = ADS.readADC(1);

                if (rawV < 0) {
                    // Negative voltage (reverse polarity)
                    digitalWrite(NFET_OFF, HIGH);
                    digitalWrite(DUT_PWR, LOW);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    nfetState = false;
                } else if (rawV >= minVoltage) {
                    // Valid positive voltage
                    digitalWrite(DUT_PWR, HIGH);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    digitalWrite(NFET_OFF, !digitalRead(NFET_OFF));
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    nfetState = !digitalRead(NFET_OFF);
                }
            }
        }

        // Handle long press: Mode switching
        if (isPressing && !isLongDetected) {
            long pressDuration = millis() - pressedTime;

            if (pressDuration > LONG_PRESS_TIME) {
                // Reset to safe values
                portENTER_CRITICAL(&mutex);
                encoderPos = 0;
                DAC = 0;
                portEXIT_CRITICAL(&mutex);
                digitalWrite(NFET_OFF, HIGH);

                // Mode switching logic
                switch (mode) {
                    case current:
                        mode = voltage;
                        digitalWrite(CV_MODE, HIGH);
                        portENTER_CRITICAL(&mutex);
                        DAC = 65000;
                        portEXIT_CRITICAL(&mutex);
                        dac.write(DAC);
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        if (dutV > minVoltage) {
                            portENTER_CRITICAL(&mutex);
                            encoderPos = dutV * 105;
                            portEXIT_CRITICAL(&mutex);
                        } else {
                            portENTER_CRITICAL(&mutex);
                            encoderPos = maxVoltage;
                            portEXIT_CRITICAL(&mutex);
                        }
                        break;

                    case voltage:
                        mode = power;
                        digitalWrite(CV_MODE, LOW);
                        digitalWrite(NFET_OFF, HIGH);
                        portENTER_CRITICAL(&mutex);
                        encoderPos = 0;
                        DAC = 0;
                        portEXIT_CRITICAL(&mutex);
                        break;

                    case power:
                        mode = resistance;
                        digitalWrite(NFET_OFF, HIGH);
                        if (dutV > 1) {
                            portENTER_CRITICAL(&mutex);
                            encoderPos = dutV * 100;
                            portEXIT_CRITICAL(&mutex);
                        } else {
                            portENTER_CRITICAL(&mutex);
                            encoderPos = 1000;
                            portEXIT_CRITICAL(&mutex);
                        }
                        set_current = 0;
                        currentDelta = 0;
                        portENTER_CRITICAL(&mutex);
                        DAC = 0;
                        portEXIT_CRITICAL(&mutex);
                        break;

                    case resistance:
                        mode = battery;
                        battSetup = true;
                        batteryModeActive = true;
                        vTaskResume(BThandle);
                        digitalWrite(NFET_OFF, HIGH);
                        portENTER_CRITICAL(&mutex);
                        encoderPos = 0;
                        DAC = 0;
                        portEXIT_CRITICAL(&mutex);
                        // Clear any stale data from serial buffer (like old cancel commands)
                        while (Serial.available()) {
                            Serial.read();
                        }
                        break;

                    case battery:
                        mode = current;
                        portENTER_CRITICAL(&mutex);
                        encoderPos = 0;
                        DAC = 0;
                        portEXIT_CRITICAL(&mutex);
                        battSetup = false;
                        batteryModeActive = false;
                        digitalWrite(NFET_OFF, HIGH);
                        nfetState = false;
                        digitalWrite(DUT_PWR, HIGH);
                        vTaskDelay(500 / portTICK_PERIOD_MS);
                        break;
                }

                Serial.print("New mode = ");
                Serial.println(modeStrings[mode]);
                isLongDetected = true;
                empty_avg_pool();
            }
        }

        // Mode-specific control logic
        switch (mode) {
            case current:  // Constant Current (CC) Mode
                portENTER_CRITICAL(&mutex);
                set_current = encoderPos * EncoderRes;
                DAC = set_current;
                if (DAC > maxCurrent_10A) DAC = maxCurrent_10A;
                portEXIT_CRITICAL(&mutex);
                dac.write(DAC);
                break;

            case voltage:  // Constant Voltage (CV) Mode
                portENTER_CRITICAL(&mutex);
                set_voltage = encoderPos / 10.0;
                DAC = int(set_voltage / 10 / maxVoltage * 65535 * cvCalFactor);
                if (DAC > 62726) DAC = 62726;
                portEXIT_CRITICAL(&mutex);
                dac.write(DAC);
                break;

            case power:  // Constant Power (CP) Mode
                portENTER_CRITICAL(&mutex);
                set_power = encoderPos / 10.0;
                portEXIT_CRITICAL(&mutex);
                powerDelta = abs(set_power - dutPower) * 10;

                portENTER_CRITICAL(&mutex);
                if (dutPower * 10 < set_power * 10) {
                    if (powerDelta > 50) {
                        DAC = DAC + powerDelta;
                    } else {
                        DAC = DAC + 1;
                    }
                } else if (dutPower * 10 > set_power * 10) {
                    if (powerDelta > 50) {
                        DAC = DAC - powerDelta;
                    } else if (DAC > 0) {
                        DAC = DAC - 1;
                    }
                }

                if (DAC > maxCurrent_10A) DAC = maxCurrent_10A;
                portEXIT_CRITICAL(&mutex);
                dac.write(DAC);
                break;

            case resistance: {  // Constant Resistance (CR) Mode
                portENTER_CRITICAL(&mutex);
                set_resistance = encoderPos / 10.0;
                double local_dutV = dutV;
                double local_shuntV = shuntV;
                portEXIT_CRITICAL(&mutex);

                if (local_dutV > 1.0) {
                    set_current = (local_dutV / set_resistance) * 1000;
                    currentDelta = abs(int(set_current) - int(local_shuntV * 1000));

                    portENTER_CRITICAL(&mutex);
                    if (local_shuntV * 1000 < set_current) {
                        if (currentDelta > 50) {
                            DAC = DAC + currentDelta;
                        } else {
                            DAC = DAC + 1;
                        }
                    } else if (local_shuntV * 1000 > set_current) {
                        if (currentDelta > 50) {
                            DAC = DAC - currentDelta;
                        } else if (DAC > 0) {
                            DAC = DAC - 1;
                        }
                    }

                    if (DAC > maxCurrent_10A) DAC = maxCurrent_10A;
                    portEXIT_CRITICAL(&mutex);
                    dac.write(DAC);
                }
                break;
            }

            case battery:  // Battery Test Mode (controlled by BatteryMode.cpp)
                // Battery mode control is handled in BatteryMode task
                break;
        }

        // Zero current reading if NFETs are off
        if (!nfetState) {
            portENTER_CRITICAL(&mutex);
            shuntV = 0.0;
            portEXIT_CRITICAL(&mutex);
        }

        // Optional: Send to serial monitor for debugging
        // send_to_monitor();

        // Reset watchdog timer (prevent system reset)
        esp_task_wdt_reset();

        // Small delay (loop time ~18ms)
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

//=============================================================================
// ADC READING FUNCTIONS
//=============================================================================

void readDutV() {
    // Read the DUT voltage from ADS1115
    // Dynamic gain control for optimal resolution

    // read the adc value so we can use it to find the optimum gain setting for the ADC
    rawV = ADS.readADC(1);
    double local_dutV = ADS.toVoltage(rawV) * dc_cal_factor * dutVcalib;

    // Dynamically adjust gain based on the just measured voltage
    if (local_dutV > 99 && adcGain != 0) {
        ADS.setGain(0);
        adcGain = 0;
        rawV = ADS.readADC(1);
        local_dutV = ADS.toVoltage(rawV) * dc_cal_factor * dutVcalib;
    } else if (local_dutV <= 99 && local_dutV >= 49 && adcGain != 1) {
        ADS.setGain(1);
        adcGain = 1;
        rawV = ADS.readADC(1);
        local_dutV = ADS.toVoltage(rawV) * dc_cal_factor * dutVcalib;
    } else if (local_dutV < 49 && local_dutV >= 24 && adcGain != 2) {
        ADS.setGain(2);
        adcGain = 2;
        rawV = ADS.readADC(1);
        local_dutV = ADS.toVoltage(rawV) * dc_cal_factor * dutVcalib;
    } else if (local_dutV < 24 && adcGain != 4) {
        ADS.setGain(4);
        adcGain = 4;
        rawV = ADS.readADC(1);
        local_dutV = ADS.toVoltage(rawV) * dc_cal_factor * dutVcalib;
    }

    // Turn on relay only with positive voltage
    if (rawV > 650) {
        digitalWrite(DUT_PWR, HIGH);
    }

    // Prepare averaged values for OLED display
    rawV_avg = avgVoltage.add(rawV);
    double local_dispVoltage = ADS.toVoltage(rawV_avg) * dc_cal_factor * dutVcalib;

    // Write to shared variables with mutex protection
    portENTER_CRITICAL(&mutex);
    dutV = local_dutV;
    dispVoltage = local_dispVoltage;
    portEXIT_CRITICAL(&mutex);
}

void readShunt() {
    // Read shunt voltage to obtain DUT current
    old_adcGain = adcGain;
    ADS.setGain(0);  // Set to maximum 10.5A range
    adcGain = 0;

    shuntVraw = ADS.readADC(0);
    if ((shuntVraw > 32767) || (shuntVraw < 0)) {
        shuntVraw = 0;
    }

    // Prepare averaged values for OLED display
    rawI_avg = avgCurrent.add(shuntVraw);
    double local_dispCurrent = ADS.toVoltage(rawI_avg) * shuntVcalib;

    // Write to shared variables with mutex protection
    portENTER_CRITICAL(&mutex);
    shuntV = ADS.toVoltage(shuntVraw) * shuntVcalib;
    dispCurrent = local_dispCurrent;
    portEXIT_CRITICAL(&mutex);

    // Restore gain setting
    adcGain = old_adcGain;
    ADS.setGain(adcGain);
}

void readTemp() {
    // Read heatsink temperature (only every ~1 second)
    if (tempUpdate == 30) {
        old_adcGain = adcGain;
        ADS.setGain(4);
        adcGain = 4;

        int16_t avgTemp = avgTemperature.add(ADS.readADC(2));

        // Write to shared variable with mutex protection
        portENTER_CRITICAL(&mutex);
        temperature = ADS.toVoltage(avgTemp) * 100;  // 10mV per degree C
        portEXIT_CRITICAL(&mutex);

        tempUpdate = 0;

        // Restore gain setting
        adcGain = old_adcGain;
        ADS.setGain(adcGain);
    } else {
        tempUpdate++;
    }
}

void checkValidValues() {
    // Read shared variables with mutex protection
    portENTER_CRITICAL(&mutex);
    double local_shuntV = shuntV;
    double local_temperature = temperature;
    double local_dutV = dutV;
    portEXIT_CRITICAL(&mutex);

    // Over-current protection
    if (local_shuntV > maxCurrent) {
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }

    // Over-temperature protection
    if (local_temperature < 1) {
        portENTER_CRITICAL(&mutex);
        temperature = 0;
        portEXIT_CRITICAL(&mutex);
        local_temperature = 0;
    }
    if (local_temperature > maxTemp) {
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }

    // Prevent MOSFETs from fully opening with no DUT
    if (local_dutV < minVoltage && local_shuntV < 0.01) {
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }

    // Reverse polarity protection
    if (rawV < 0) {
        digitalWrite(DUT_PWR, LOW);
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }

    // Calculate power
    double local_dutPower = local_dutV * local_shuntV;
    if (local_dutPower < 0) local_dutPower = 0;

    // Write calculated power with mutex protection
    portENTER_CRITICAL(&mutex);
    dutPower = local_dutPower;
    portEXIT_CRITICAL(&mutex);

    // Max power protection
    if (local_dutPower > maxPower) {
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }

    // Maximum voltage protection
    if (local_dutV > maxVoltage) {
        digitalWrite(NFET_OFF, HIGH);
        nfetState = false;
    }
}

void prep_new_mode() {
    // TODO: Implement mode preparation logic
}

float adc2volt() {
    // TODO: Implement if needed
    return 0.0;
}

//=============================================================================
// UTILITY FUNCTIONS
// Note: RTOS task functions (process_encoder, updateOledDisplay) are in their
//       respective module files (RotaryEncoder.cpp, OLEDDisplay.cpp)
//=============================================================================

void empty_avg_pool() {
    avgCurrent.reset();
}
