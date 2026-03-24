/**
 * @file main.cpp
 * @brief Main entry point for Dynamic Load firmware
 * @author Paul Versteeg
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
#include "CalibrationManager.h"
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
bool firstCVEntry = true;  // Flag to handle first CV mode entry current surge

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
const uint16_t DAC_MAX_VALUE = 65535;  // 16-bit DAC maximum value

// Regulation
double powerDelta = 0.0;
double voltDelta = 0.0;
int currentDelta = 0;

// Safety limits
long maxCurrent = 10.2;  // 10.2A
long maxVoltage = 105;   // 105V
long maxPower = 185;     // 185W
long maxTemp = 95;       // 95°C
long minVoltage = 0.7;   // 0.7V for 1.2V NiCad/NiMH support

// Constants
const double voltage_ref = 4.096;
const double dc_cal_factor = 24.15;
double currOffset = 0.0;

// Runtime calibration values (loaded from DL_Cal_Values.ini at boot)
double dutVcalib = DEFAULT_DUT_V_CALIB;
double DAC_ADC_TOLERANCE = DEFAULT_DAC_ADC_TOLERANCE;
double iCalLow = DEFAULT_ICAL_LOW;    // Current correction factor at low reference point
double iRefLow = DEFAULT_IREF_LOW;    // True current at low calibration point (A)
double iCalHigh = DEFAULT_ICAL_HIGH;  // Current correction factor at high reference point
double iRefHigh = DEFAULT_IREF_HIGH;  // True current at high calibration point (A)
double cvCalFactor = DEFAULT_CV_CAL_FACTOR;

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

// Communication watchdog - safety feature to abort Battery discharge test if PC disconnects
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

    Serial.print("\n\r\n\rDynamic DC Load - Version ");
    Serial.println(FW_VERSION);

    // Initialize calibration manager and load values from Preferences
    if (!CalibrationManager::begin()) {
        Serial.println("WARNING: Calibration system initialization failed");
    }

    // Setup fan controller EARLY (before calibration mode check)
    // This allows us to control the fan during calibration mode
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RESOLUTION);
    ledcAttachPin(FAN_PWM, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, 0);  // Turn off initially

    // Initialize OLED display
    Serial.println("Starting TFT");
    tft.begin();
    tft.setRotation(0);

    // Check if button is held during boot for calibration mode (check EARLY)
    pinMode(ENC_BUT, INPUT_PULLUP);
    delay(100);                                            // Delay for button state to settle
    bool calibrationMode = (digitalRead(ENC_BUT) == LOW);  // Read before splash screen

    // Show splash screen (but we already know if cal mode is needed)
    oled_prep();

    if (calibrationMode) {  // Button was pressed during boot

        // Turn off the fan during calibration mode
        ledcWrite(FAN_PWM_CHANNEL, 0);
        Serial.println("- Fan stopped for calibration mode");

        // Display initial calibration mode screen (simple)
        tft.fillScreen(BLACK);
        tft.setFont();  // Use default small font
        tft.setTextColor(YELLOW);
        tft.setCursor(10, 20);
        tft.print("CALIBRATION");
        tft.setCursor(10, 30);
        tft.print("MODE");
        tft.setTextColor(WHITE);
        tft.setCursor(10, 50);
        tft.print("Connect serial");
        tft.setCursor(10, 65);
        tft.print("Type CAL on");
        tft.setCursor(10, 75);
        tft.print("serial monitor");
        tft.setTextColor(GREEN);
        tft.setCursor(10, 90);
        tft.print("or click to");
        tft.setCursor(10, 100);
        tft.print("cancel");

        CalibrationManager::printCalibration();
        Serial.println("\nEnter calibration commands:");
        Serial.println("Type 'CAL SHOW' for help");

        // Wait for button release
        while (digitalRead(ENC_BUT) == LOW) {
            delay(10);
        }

        // Calibration command loop
        String inputBuffer = "";
        bool showedMenu = false;
        bool exitViaCommand = false;  // Track if exited via CAL EXIT vs button
        while (true) {
            if (Serial.available()) {
                char c = Serial.read();
                if (c == '\n' || c == '\r') {
                    if (inputBuffer.length() > 0) {
                        // Show command menu after first input
                        if (!showedMenu) {
                            tft.fillScreen(BLACK);
                            tft.setFont();
                            tft.setTextColor(YELLOW);
                            tft.setCursor(5, 2);
                            tft.print("CAL Commands:");
                            tft.setTextColor(CYAN);
                            tft.setCursor(5, 14);
                            tft.print("CAL SHOW");
                            tft.setCursor(5, 23);
                            tft.print("CAL CV");
                            tft.setCursor(5, 32);
                            tft.print("CAL DUTV");
                            tft.setCursor(5, 41);
                            tft.print("CAL DUTC");
                            tft.setCursor(5, 50);
                            tft.print("CAL CURRL");
                            tft.setCursor(5, 59);
                            tft.print("CAL CURRH");
                            tft.setCursor(5, 68);
                            tft.print("CAL SAVE");
                            tft.setCursor(5, 77);
                            tft.print("CAL RESET");
                            tft.setCursor(5, 86);
                            tft.print("CAL EXIT");
                            tft.setTextColor(GREEN);
                            tft.setCursor(5, 100);
                            tft.print("Active - use");
                            tft.setCursor(5, 110);
                            tft.print("serial monitor");
                            showedMenu = true;
                        }

                        if (CalibrationManager::processCommand(inputBuffer)) {
                            // Check for exit command
                            String cmd = inputBuffer;
                            cmd.trim();
                            cmd.toUpperCase();
                            if (cmd == "CAL EXIT") {
                                exitViaCommand = true;
                                break;  // Exit calibration mode
                            }
                        }
                        inputBuffer = "";
                        Serial.print("\n> ");
                    }
                } else if (c == 8 || c == 127) {  // Backspace or Delete
                    if (inputBuffer.length() > 0) {
                        inputBuffer.remove(inputBuffer.length() - 1);
                        Serial.print("\b \b");  // Backspace, space, backspace to erase character
                    }
                } else if (c == 3) {  // Ctrl-C
                    if (inputBuffer.length() > 0) {
                        Serial.println("\n^C (cancelled)");
                        inputBuffer = "";
                        Serial.print("> ");
                    }
                } else if (c >= 32 && c <= 126) {  // Printable characters
                    inputBuffer += c;
                    Serial.print(c);  // Echo
                }
            }

            // Check for button press to cancel calibration mode
            if (digitalRead(ENC_BUT) == LOW) {
                delay(50);  // Debounce
                if (digitalRead(ENC_BUT) == LOW) {
                    Serial.println("\n\nCalibration cancelled - continuing normal boot");
                    exitViaCommand = false;
                    break;  // Exit calibration mode and continue boot
                }
            }

            esp_task_wdt_reset();  // Feed watchdog (safe no-op if not subscribed)
            delay(10);
        }

        // Only show exit screen and halt if exited via CAL EXIT command
        if (exitViaCommand) {
            Serial.println("\nCalibration mode exited. Power cycle to restart.");
            tft.fillScreen(BLACK);
            tft.setFont();  // Use default small font
            tft.setTextColor(GREEN);
            tft.setCursor(20, 50);
            tft.print("Calibration");
            tft.setCursor(20, 60);
            tft.print("Complete!");
            tft.setTextColor(WHITE);
            tft.setCursor(10, 80);
            tft.print("Power cycle to");
            tft.setCursor(10, 90);
            tft.print("restart Dynamic");
            tft.setCursor(10, 100);
            tft.print("Load");

            // Infinite loop - user must power cycle
            while (true) {
                delay(1000);
            }
        }
        // If exited via button, just continue with normal boot
    }

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
    // Note: ISR attachment is in process_encoder task for correct core affinity

    // Setup fan tacho interrupt (PWM setup already done early in setup)
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
    esp_task_wdt_init(WDT_TIMEOUT, true);  // 5 second timeout, enable panic to reboot on timeout
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
    // Boot message with core info
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
    bool longPressJustCompleted = false;  // Prevents button action immediately after mode switch

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
        // Python app sends "AUTO_BT\n" to trigger automatic mode switch or reset state
        // This doesn't interfere with firmware upload (esptool doesn't send this string)
        // Process AUTO_BT even when in battery mode (to allow test restart)
        // but only check at beginning of serial data (peek for 'A')
        if (Serial.available() && Serial.peek() == 'A' && !battSetup) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            if (cmd == "AUTO_BT") {
                // Update communication watchdog
                lastSerialActivity = millis();

                bool justSwitchedMode = false;
                if (mode != battery) {
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

                    justSwitchedMode = true;
                } else {
                    // Already in battery mode - reset for new test
                    Serial.println("Resetting Battery Test mode for new test");

                    // Reset to safe values
                    portENTER_CRITICAL(&mutex);
                    encoderPos = 0;
                    DAC = 0;
                    portEXIT_CRITICAL(&mutex);
                    digitalWrite(NFET_OFF, HIGH);

                    // Trigger setup phase for new test
                    battSetup = true;
                    batteryModeActive = true;
                    // Task is already running, no need to resume
                }

                // Clear any stale data from serial buffer (like old cancel commands)
                while (Serial.available()) {
                    Serial.read();
                }

                // Send acknowledgment - DL is ready to receive parameters
                Serial.println("ACK_BT");

                // Delay after mode switch to allow "Waiting for parameters" message to be visible
                // Use vTaskDelay with watchdog reset instead of blocking delay()
                if (justSwitchedMode) {
                    for (int i = 0; i < 10; i++) {
                        esp_task_wdt_reset();
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                    }
                }
            }
        }

        // Handle button press events
        if (button.isPressed()) {
            pressedTime = millis();
            isPressing = true;
            isLongDetected = false;
            longPressJustCompleted = false;  // Clear guard on new press
        }

        if (button.isReleased()) {
            isPressing = false;
            releasedTime = millis();
            long pressDuration = releasedTime - pressedTime;

            // Ignore this release if it's right after a long press mode switch
            if (!longPressJustCompleted && pressDuration < SHORT_PRESS_TIME) {
                // Short press: Toggle MOSFET on/off
                rawV = ADS.readADC(1);

                if (rawV < 0) {
                    // Negative voltage (reverse polarity?) - turn everything off for safety
                    digitalWrite(NFET_OFF, HIGH);
                    digitalWrite(DUT_PWR, LOW);
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                    nfetState = false;
                } else if (rawV >= minVoltage) {
                    // Valid positive voltage
                    digitalWrite(DUT_PWR, HIGH);
                    vTaskDelay(100 / portTICK_PERIOD_MS);

                    // First time turning on in CV mode: soft-start to prevent up to 4A current surge
                    if (mode == voltage && firstCVEntry && digitalRead(NFET_OFF) == HIGH) {
                        // Calculate target DAC value
                        portENTER_CRITICAL(&mutex);
                        unsigned long local_encoderPos = encoderPos;
                        portEXIT_CRITICAL(&mutex);

                        double target_voltage = local_encoderPos / 10.0;
                        uint16_t target_DAC = int(target_voltage / 10 / maxVoltage * DAC_MAX_VALUE * cvCalFactor);
                        if (target_DAC > DAC_MAX_CV_MODE) target_DAC = DAC_MAX_CV_MODE;

                        // Start with maximum DAC value (maximum voltage target above DUT voltage)
                        // This helps to remove the turn-on current spike by starting with a high voltage target that is above the DUT voltage, so there is no initial current flow when NFETs turn on - then we ramp down to the actual target voltage
                        dac.write(DAC_MAX_VALUE);
                        vTaskDelay(100 / portTICK_PERIOD_MS);  // make sure it has settled

                        // Turn on NFETs
                        digitalWrite(NFET_OFF, LOW);
                        vTaskDelay(50 / portTICK_PERIOD_MS);  // Get rid of a current glitch on turn-on

                        // Set final target Set value
                        portENTER_CRITICAL(&mutex);
                        DAC = target_DAC;
                        portEXIT_CRITICAL(&mutex);
                        dac.write(target_DAC);

                        vTaskDelay(100 / portTICK_PERIOD_MS);  // let the DAC settle at the target voltage before allowing any adjustments
                        nfetState = true;

                        firstCVEntry = false;  // Only do the the first time we enter CV mode to prevent current spike on every toggle
                    } else {
                        // Normal operation
                        digitalWrite(NFET_OFF, !digitalRead(NFET_OFF));
                        vTaskDelay(100 / portTICK_PERIOD_MS);
                        nfetState = !digitalRead(NFET_OFF);

                        // In CV mode, give extra time for circuit to settle
                        if (mode == voltage && nfetState) {
                            vTaskDelay(200 / portTICK_PERIOD_MS);
                        }
                    }
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
                // This could look a little strange because the switch(mode) actually acivates the
                // next mode, so the case labels are the "currently selected" mode, but the actions
                // inside are for switching to the next mode
                switch (mode) {
                    case current:
                        mode = voltage;
                        digitalWrite(CV_MODE, HIGH);
                        vTaskDelay(100 / portTICK_PERIOD_MS);  // Let CV_MODE circuit stabilize first

                        // Calculate and set the correct DAC value immediately to avoid large step change
                        if (dutV > minVoltage) {
                            // Set encoder to 102% of DUT voltage for safety margin,
                            // so there is no current flowing when we switch to CV mode - prevents large current spike if DUT voltage is close to target voltage
                            double target_voltage = dutV * 1.02;
                            portENTER_CRITICAL(&mutex);
                            encoderPos = dutV * 102;
                            DAC = int(target_voltage / maxVoltage * DAC_MAX_VALUE * cvCalFactor);
                            if (DAC > DAC_MAX_CV_MODE) DAC = DAC_MAX_CV_MODE;
                            portEXIT_CRITICAL(&mutex);
                        } else {
                            // No valid voltage, set to maximum
                            portENTER_CRITICAL(&mutex);
                            encoderPos = maxVoltage;
                            DAC = DAC_MAX_CV_MODE;
                            portEXIT_CRITICAL(&mutex);
                        }

                        dac.write(DAC);
                        avgCurrent.reset();                    // Clear current average for clean CV mode start
                        vTaskDelay(500 / portTICK_PERIOD_MS);  // Let CV circuit fully settle at target voltage
                        break;

                    case voltage:
                        mode = power;
                        digitalWrite(CV_MODE, LOW);  // switch the DAC output back to current control mode
                        digitalWrite(NFET_OFF, HIGH);
                        portENTER_CRITICAL(&mutex);
                        encoderPos = 0;
                        DAC = 0;
                        portEXIT_CRITICAL(&mutex);
                        firstCVEntry = true;  // Reset flag when leaving CV mode
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
                longPressJustCompleted = true;  // Prevent button action on release
                Serial.println(modeStrings[mode]);
                isLongDetected = true;
                // Don't reset averaging on mode switch - voltage/temp stay same, current is real
            }
        }

        // Mode-specific control logic. Here is where the actual work in the modes is done for each mode,
        // using the measurements and set points to calculate the appropriate DAC value to
        // write, with safety checks and multi-stage control for faster settling in CP and CR modes.
        switch (mode) {
            case current:  // Constant Current (CC) Mode
            {
                portENTER_CRITICAL(&mutex);
                unsigned long local_encoderPos = encoderPos;
                portEXIT_CRITICAL(&mutex);

                set_current = local_encoderPos * EncoderRes;
                uint16_t local_DAC = set_current;
                if (local_DAC > maxCurrent_10A) local_DAC = maxCurrent_10A;

                portENTER_CRITICAL(&mutex);
                DAC = local_DAC;
                portEXIT_CRITICAL(&mutex);

                dac.write(local_DAC);
            } break;

            case voltage:  // Constant Voltage (CV) Mode
            {
                portENTER_CRITICAL(&mutex);
                unsigned long local_encoderPos = encoderPos;
                portEXIT_CRITICAL(&mutex);

                set_voltage = local_encoderPos / 10.0;
                uint16_t local_DAC = int(set_voltage / 10 / maxVoltage * DAC_MAX_VALUE * cvCalFactor);
                if (local_DAC > DAC_MAX_CV_MODE) local_DAC = DAC_MAX_CV_MODE;

                portENTER_CRITICAL(&mutex);
                DAC = local_DAC;
                portEXIT_CRITICAL(&mutex);

                dac.write(local_DAC);
            } break;

            case power: {  // Constant Power (CP) Mode, DUTcurrent = set_power/DUTV, @loop time speed
                // Read all inputs in one critical section
                portENTER_CRITICAL(&mutex);
                unsigned long local_encoderPos = encoderPos;
                double local_dutPower = dutPower;
                uint16_t local_DAC = DAC;
                portEXIT_CRITICAL(&mutex);

                // Calculate set point outside critical section
                set_power = local_encoderPos / 10.0;  // encoderPos is Watt in 100mW or 1W clicks

                // Multi-stage control algorithm using fixed steps for faster settling:
                // Proportional control is too slow - use aggressive fixed steps instead
                double powerError = abs(set_power - local_dutPower);

                if (powerError > 2.0) {
                    // Very far from target: large fixed step for fast initial response
                    powerDelta = 100;
                } else if (powerError > 0.5) {
                    // Far from target: medium-large fixed step
                    powerDelta = 50;
                } else if (powerError > 0.1) {
                    // Medium distance: moderate fixed step
                    powerDelta = 20;
                } else if (powerError > 0.02) {
                    // Close to target: small fixed step for stable approach
                    powerDelta = 5;
                } else {
                    // Very close (within 20mW): stop adjusting to prevent hunting
                    powerDelta = 0;
                }

                // Update DAC based on power error
                if (local_dutPower < set_power) {
                    local_DAC = local_DAC + powerDelta;
                } else if (local_dutPower > set_power) {
                    local_DAC = local_DAC - powerDelta;
                }

                if (set_power == 0) {
                    local_DAC = 0;
                }  // prevent an initial run-away when the power is applied

                // special condition!
                // if the DUT (a power supply?) goes to the CC mode, the DUT voltage goes to zero and the NFET's will be switched off.
                // This also results in a dutPower value of zero.
                // When you switch the NFET's back on, the DAC value remains high and the powerDelta is also high, which will
                // switch the DUT in over-current again. The only remedy is to completely dial the DL down to zero power
                // and start the measurement again. The following code allows you to resume the measurement.
                if ((nfetState == false) && (local_dutPower == 0)) {
                    local_DAC = 0;
                    set_power = 0;
                }

                if (local_DAC > DAC_MAX_CP_MODE) {
                    local_DAC = DAC_MAX_CP_MODE;
                }

                // Write back DAC in single critical section
                portENTER_CRITICAL(&mutex);
                DAC = local_DAC;
                portEXIT_CRITICAL(&mutex);

                dac.write(local_DAC);  // output the DAC value
                break;
            }

            case resistance: {  // Constant Resistance (CR) Mode
                // In this mode, a small encoder value means a high current
                // This mode starts with the encoder/DAC set at a safe current of 100mA based
                // on the DUT voltage

                // Read all inputs in one critical section
                portENTER_CRITICAL(&mutex);
                unsigned long local_encoderPos = encoderPos;
                double local_dutV = dutV;
                double local_shuntV = shuntV;
                uint16_t local_DAC = DAC;
                portEXIT_CRITICAL(&mutex);

                if (nfetState == true) {                       // only adjust the DAC when the DL is on.
                    set_resistance = local_encoderPos / 10.0;  // encoderPos in Ohms (100mOhm or 1 Ohm clicks)
                    set_current = abs(local_dutV / set_resistance);

                    // Multi-stage control algorithm using fixed steps for faster settling
                    double currentError = abs(set_current - local_shuntV);

                    if (currentError > 0.2) {
                        // Very far from target: large fixed step for fast initial response
                        currentDelta = 100;
                    } else if (currentError > 0.05) {
                        // Far from target: medium-large fixed step
                        currentDelta = 50;
                    } else if (currentError > 0.01) {
                        // Medium distance: moderate fixed step
                        currentDelta = 20;
                    } else if (currentError > 0.002) {
                        // Close to target: small fixed step for stable approach
                        currentDelta = 5;
                    } else {
                        // Very close (within 2mA): stop adjusting to prevent hunting
                        currentDelta = 0;
                    }

                    if (local_shuntV < set_current) {
                        local_DAC = local_DAC + currentDelta;  // increase the current
                    } else if (local_shuntV > set_current) {
                        local_DAC = local_DAC - currentDelta;
                    }
                }

                // Safety limit check - always enforced regardless of NFET state
                if (local_DAC > maxCurrent_4A) local_DAC = maxCurrent_4A;

                // Write back DAC in single critical section
                portENTER_CRITICAL(&mutex);
                DAC = local_DAC;
                portEXIT_CRITICAL(&mutex);

                dac.write(local_DAC);  // output the DAC value
                break;
            }

            case battery:  // Battery Test Mode (controlled by BatteryMode.cpp)
                // Battery mode control is handled in the BatteryMode task
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

        // No delay - run as fast as possible for CP/CR mode response (loop time ~18ms)
        // vTaskDelay(10 / portTICK_PERIOD_MS);
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

    // Dynamically adjust gain based on the just measured voltage, and then read
    // the voltage again with the new gain setting for optimal resolution - this
    // is important for accurate measurements and stable control in CP and CR modes
    // across the wide voltage range (up to 100V)
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

    // Turn on relay only with a positive voltage
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

    // Two-point linear interpolation of current correction factor:
    // - Below iRefLow  : use iCalLow flat
    // - Above iRefHigh : use iCalHigh flat
    // - In between     : interpolate proportionally
    double rawCurrent = ADS.toVoltage(shuntVraw) * I_GAIN;
    double calFactor;
    if (rawCurrent <= iRefLow) {
        calFactor = iCalLow;
    } else if (rawCurrent >= iRefHigh) {
        calFactor = iCalHigh;
    } else {
        calFactor = iCalLow + (rawCurrent - iRefLow) / (iRefHigh - iRefLow) * (iCalHigh - iCalLow);
    }

    double rawCurrentAvg = ADS.toVoltage(rawI_avg) * I_GAIN;
    double calFactorAvg;
    if (rawCurrentAvg <= iRefLow) {
        calFactorAvg = iCalLow;
    } else if (rawCurrentAvg >= iRefHigh) {
        calFactorAvg = iCalHigh;
    } else {
        calFactorAvg = iCalLow + (rawCurrentAvg - iRefLow) / (iRefHigh - iRefLow) * (iCalHigh - iCalLow);
    }
    double local_dispCurrent = rawCurrentAvg * calFactorAvg;

    // Write to shared variables with mutex protection
    portENTER_CRITICAL(&mutex);
    shuntV = rawCurrent * calFactor;
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

//=============================================================================
// UTILITY FUNCTIONS
// Note: RTOS task functions (process_encoder, updateOledDisplay) are in their
//       respective module files (RotaryEncoder.cpp, OLEDDisplay.cpp)
//=============================================================================

void empty_avg_pool() {
    // Reset all averaging pools when entering Battery Test mode
    // Battery mode needs clean slate for accurate profiling
    avgCurrent.reset();
    avgVoltage.reset();
    avgTemperature.reset();
}
