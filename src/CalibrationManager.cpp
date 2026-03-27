/**
 * @file CalibrationManager.cpp
 * @brief Implementation of calibration manager using ESP32 Preferences
 * @author Paul Versteeg
 * @date 2026
 */

#include "CalibrationManager.h"

#include <Preferences.h>

#include "Config.h"
#include "DynamicLoad.h"

const char* CalibrationManager::PREFS_NAMESPACE = "dl_cal";

static Preferences preferences;

bool CalibrationManager::begin() {
    Serial.println("- Initializing calibration system...");

    // Try to load calibration values from Preferences
    if (loadCalibration()) {
        Serial.println("- Calibration values loaded from Preferences");
        printCalibration();
        return true;
    } else {
        Serial.println("- No stored calibration found, using defaults");
        Serial.println("- Use CAL commands to set and save values");
        printCalibration();
        return false;
    }
}

bool CalibrationManager::loadCalibration() {
    preferences.begin(PREFS_NAMESPACE, true);  // true = read-only

    // Check if any values exist
    if (!preferences.isKey("vCalHigh")) {
        preferences.end();
        return false;  // No saved calibration
    }

    // Load all values (with defaults as fallback)
    vCalHigh = preferences.getDouble("vCalHigh", DEFAULT_VCAL_HIGH);
    vRefLow = preferences.getDouble("vRefLow", DEFAULT_VREF_LOW);
    vRefHigh = preferences.getDouble("vRefHigh", DEFAULT_VREF_HIGH);
    DAC_ADC_TOLERANCE = preferences.getDouble("DAC_ADC_TOLERANCE", DEFAULT_DAC_ADC_TOLERANCE);
    iCalLow = preferences.getDouble("iCalLow", DEFAULT_ICAL_LOW);
    iRefLow = preferences.getDouble("iRefLow", DEFAULT_IREF_LOW);
    iCalHigh = preferences.getDouble("iCalHigh", DEFAULT_ICAL_HIGH);
    iRefHigh = preferences.getDouble("iRefHigh", DEFAULT_IREF_HIGH);
    cvCalFactor = preferences.getDouble("cvCalFactor", DEFAULT_CV_CAL_FACTOR);

    preferences.end();
    return true;
}

bool CalibrationManager::saveCalibration() {
    preferences.begin(PREFS_NAMESPACE, false);  // false = read/write

    preferences.putDouble("vCalHigh", vCalHigh);
    preferences.putDouble("vRefLow", vRefLow);
    preferences.putDouble("vRefHigh", vRefHigh);
    preferences.putDouble("DAC_ADC_TOLERANCE", DAC_ADC_TOLERANCE);
    preferences.putDouble("iCalLow", iCalLow);
    preferences.putDouble("iRefLow", iRefLow);
    preferences.putDouble("iCalHigh", iCalHigh);
    preferences.putDouble("iRefHigh", iRefHigh);
    preferences.putDouble("cvCalFactor", cvCalFactor);

    preferences.end();

    Serial.println("\r\nCalibration values saved to Preferences");
    return true;
}

bool CalibrationManager::resetToDefaults() {
    vCalHigh = DEFAULT_VCAL_HIGH;
    vRefLow = DEFAULT_VREF_LOW;
    vRefHigh = DEFAULT_VREF_HIGH;
    DAC_ADC_TOLERANCE = DEFAULT_DAC_ADC_TOLERANCE;
    iCalLow = DEFAULT_ICAL_LOW;
    iRefLow = DEFAULT_IREF_LOW;
    iCalHigh = DEFAULT_ICAL_HIGH;
    iRefHigh = DEFAULT_IREF_HIGH;
    cvCalFactor = DEFAULT_CV_CAL_FACTOR;

    Serial.println("Calibration reset to defaults");
    printCalibration();
    return true;
}

void CalibrationManager::printCalibration() {
    Serial.println("\n=== Current Calibration Values ===");
    Serial.print("vCalHigh     : ");
    Serial.println(vCalHigh, 9);
    Serial.print("vRefLow  (V) : ");
    Serial.println(vRefLow, 3);
    Serial.print("vRefHigh (V) : ");
    Serial.println(vRefHigh, 3);
    Serial.print("DAC_ADC_TOLER: ");
    Serial.println(DAC_ADC_TOLERANCE, 2);
    Serial.print("iCalLow      : ");
    Serial.println(iCalLow, 9);
    Serial.print("iRefLow  (A) : ");
    Serial.println(iRefLow, 3);
    Serial.print("iCalHigh     : ");
    Serial.println(iCalHigh, 9);
    Serial.print("iRefHigh (A) : ");
    Serial.println(iRefHigh, 3);
    Serial.print("cvCalFactor  : ");
    Serial.println(cvCalFactor, 9);
    Serial.println("==================================\n");
}

bool CalibrationManager::processCommand(const String& command) {
    String cmd = command;
    cmd.trim();
    cmd.toUpperCase();

    // Just "CAL" - show help
    if (cmd == "CAL") {
        printCalibration();
        Serial.println("Commands:");
        Serial.println("  CAL SHOW                      - Display current values");
        Serial.println("  CAL CV <value>                - Set cvCalFactor");
        Serial.println("  CAL VH <actual_V> <oled_V>    - Set high-point voltage cal");
        Serial.println("  CAL VREF <voltage>            - Set low anchor voltage (default 2.5V)");
        Serial.println("  CAL DUTC <value>              - Set DAC_ADC_TOLERANCE");
        Serial.println("  CAL CURRL <set_mA> <oled_mA>  - Calibrate low current point in mA");
        Serial.println("  CAL CURRH <set_mA> <oled_mA>  - Calibrate high current point in mA");
        Serial.println("  CAL SAVE                      - Save to persistent storage");
        Serial.println("  CAL RESET                     - Reset to defaults");
        Serial.println("  CAL EXIT                      - Exit calibration mode");
        return true;
    }

    // CAL SHOW
    if (cmd == "CAL SHOW") {
        printCalibration();
        return true;
    }

    // CAL SAVE
    if (cmd == "CAL SAVE") {
        return saveCalibration();
    }

    // CAL RESET
    if (cmd == "CAL RESET") {
        resetToDefaults();
        Serial.println("\r\nUse 'CAL SAVE' to persist these defaults");
        return true;
    }

    // CAL EXIT
    if (cmd == "CAL EXIT") {
        Serial.println("\r\nExiting calibration mode - power cycle to restart");
        return true;
    }

    // Parse CAL <param> <value> commands
    if (cmd.startsWith("CAL ")) {
        String params = command.substring(4);  // Use original case for value
        params.trim();

        int spacePos = params.indexOf(' ');
        if (spacePos == -1) {
            Serial.println("ERROR: Missing value. Format: CAL <param> <value>");
            return false;
        }

        String param = params.substring(0, spacePos);
        param.toUpperCase();
        String valueStr = params.substring(spacePos + 1);
        valueStr.trim();
        double value = valueStr.toDouble();

        // Set the appropriate parameter
        if (param == "CV") {
            cvCalFactor = value;
            Serial.print("\r\ncvCalFactor set to: ");
            Serial.println(cvCalFactor, 9);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "VH") {
            // CAL VH <actual_V> <oled_V>
            int sp2 = valueStr.indexOf(' ');
            if (sp2 == -1) {
                Serial.println("\r\nERROR: Format: CAL VH <actual_V> <oled_V>");
                return false;
            }
            double actualV = valueStr.substring(0, sp2).toDouble();
            double oledV = valueStr.substring(sp2 + 1).toDouble();
            if (oledV < 0.001) {
                Serial.println("\r\nERROR: OLED reading is zero or missing");
                return false;
            }
            vCalHigh = actualV / oledV;
            vRefHigh = actualV;
            Serial.print("\r\nvCalHigh set to: ");
            Serial.println(vCalHigh, 9);
            Serial.print("vRefHigh set to: ");
            Serial.println(vRefHigh, 3);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "VREF") {
            vRefLow = value;
            Serial.print("\r\nvRefLow set to: ");
            Serial.println(vRefLow, 3);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "DUTC") {
            DAC_ADC_TOLERANCE = value;
            Serial.print("\r\nDAC_ADC_TOLERANCE set to: ");
            Serial.println(DAC_ADC_TOLERANCE, 2);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "CURRL") {
            // Expects two values: <set> <oled>
            // Values may be in A or mA - if > 10, assumed mA and auto-converted
            int sp2 = valueStr.indexOf(' ');
            if (sp2 == -1) {
                Serial.println("\r\nERROR: Format: CAL CURRL <set_mA> <oled_mA>");
                return false;
            }
            double setA = valueStr.substring(0, sp2).toDouble();
            double dispA = valueStr.substring(sp2 + 1).toDouble();
            if (setA > 10.0) setA /= 1000.0;    // mA -> A
            if (dispA > 10.0) dispA /= 1000.0;  // mA -> A
            if (dispA < 0.001) {
                Serial.println("\r\nERROR: OLED reading is zero or missing");
                return false;
            }
            iCalLow = setA / dispA;
            iRefLow = setA;
            Serial.print("\r\niCalLow set to: ");
            Serial.println(iCalLow, 9);
            Serial.print("iRefLow set to: ");
            Serial.println(iRefLow, 3);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "CURRH") {
            int sp2 = valueStr.indexOf(' ');
            if (sp2 == -1) {
                Serial.println("\r\nERROR: Format: CAL CURRH <set_mA> <oled_mA>");
                return false;
            }
            double setA = valueStr.substring(0, sp2).toDouble();
            double dispA = valueStr.substring(sp2 + 1).toDouble();
            if (setA > 10.0) setA /= 1000.0;    // mA -> A
            if (dispA > 10.0) dispA /= 1000.0;  // mA -> A
            if (dispA < 0.001) {
                Serial.println("\r\nERROR: OLED reading is zero or missing");
                return false;
            }
            iCalHigh = setA / dispA;
            iRefHigh = setA;
            Serial.print("\r\niCalHigh set to: ");
            Serial.println(iCalHigh, 9);
            Serial.print("iRefHigh set to: ");
            Serial.println(iRefHigh, 3);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else {
            Serial.print("\r\nERROR: Unknown parameter '");
            Serial.print(param);
            Serial.println("'. Valid: CV, VH, VREF, DUTC, CURRL, CURRH");
            return false;
        }
    }

    Serial.println("ERROR: Unknown command. Valid commands:");
    Serial.println("  CAL SHOW                      - Display current values");
    Serial.println("  CAL CV <value>                - Set cvCalFactor");
    Serial.println("  CAL VH <actual_V> <oled_V>    - Set high-point voltage cal");
    Serial.println("  CAL VREF <voltage>            - Set low anchor voltage (default 2.5V)");
    Serial.println("  CAL DUTC <value>              - Set DAC_ADC_TOLERANCE");
    Serial.println("  CAL CURRL <set_mA> <oled_mA>  - Calibrate low current point in mA");
    Serial.println("  CAL CURRH <set_mA> <oled_mA>  - Calibrate high current point in mA");
    Serial.println("  CAL SAVE                      - Save to persistent storage");
    Serial.println("  CAL RESET                     - Reset to defaults");
    Serial.println("  CAL EXIT                      - Exit calibration mode");
    return false;
}
