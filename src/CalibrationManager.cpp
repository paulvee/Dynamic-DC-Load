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
    if (!preferences.isKey("dutVcalib")) {
        preferences.end();
        return false;  // No saved calibration
    }

    // Load all values (with defaults as fallback)
    dutVcalib = preferences.getDouble("dutVcalib", DEFAULT_DUT_V_CALIB);
    DAC_ADC_TOLERANCE = preferences.getDouble("DAC_ADC_TOLERANCE", DEFAULT_DAC_ADC_TOLERANCE);
    shuntVcalib = preferences.getDouble("shuntVcalib", DEFAULT_SHUNT_V_CALIB);
    cvCalFactor = preferences.getDouble("cvCalFactor", DEFAULT_CV_CAL_FACTOR);

    preferences.end();
    return true;
}

bool CalibrationManager::saveCalibration() {
    preferences.begin(PREFS_NAMESPACE, false);  // false = read/write

    preferences.putDouble("dutVcalib", dutVcalib);
    preferences.putDouble("DAC_ADC_TOLERANCE", DAC_ADC_TOLERANCE);
    preferences.putDouble("shuntVcalib", shuntVcalib);
    preferences.putDouble("cvCalFactor", cvCalFactor);

    preferences.end();

    Serial.println("\r\nCalibration values saved to Preferences");
    return true;
}

bool CalibrationManager::resetToDefaults() {
    dutVcalib = DEFAULT_DUT_V_CALIB;
    DAC_ADC_TOLERANCE = DEFAULT_DAC_ADC_TOLERANCE;
    shuntVcalib = DEFAULT_SHUNT_V_CALIB;
    cvCalFactor = DEFAULT_CV_CAL_FACTOR;

    Serial.println("Calibration reset to defaults");
    printCalibration();
    return true;
}

void CalibrationManager::printCalibration() {
    Serial.println("\n=== Current Calibration Values ===");
    Serial.print("dutVcalib    : ");
    Serial.println(dutVcalib, 6);
    Serial.print("DAC_ADC_TOLER: ");
    Serial.println(DAC_ADC_TOLERANCE, 2);
    Serial.print("shuntVcalib  : ");
    Serial.println(shuntVcalib, 6);
    Serial.print("cvCalFactor  : ");
    Serial.println(cvCalFactor, 6);
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
        Serial.println("  CAL SHOW              - Display current values");
        Serial.println("  CAL CV <value>        - Set cvCalFactor");
        Serial.println("  CAL DUTV <value>      - Set dutVcalib");
        Serial.println("  CAL DUTC <value>      - Set DAC_ADC_TOLERANCE");
        Serial.println("  CAL SHUNT <value>     - Set shuntVcalib");
        Serial.println("  CAL SAVE              - Save to persistent storage");
        Serial.println("  CAL RESET             - Reset to defaults");
        Serial.println("  CAL EXIT              - Exit calibration mode");
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
            Serial.println(cvCalFactor, 6);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "DUTV") {
            dutVcalib = value;
            Serial.print("\r\ndutVcalib set to: ");
            Serial.println(dutVcalib, 6);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "DUTC") {
            DAC_ADC_TOLERANCE = value;
            Serial.print("\r\nDAC_ADC_TOLERANCE set to: ");
            Serial.println(DAC_ADC_TOLERANCE, 2);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else if (param == "SHUNT") {
            shuntVcalib = value;
            Serial.print("\r\nshuntVcalib set to: ");
            Serial.println(shuntVcalib, 4);
            Serial.println("Use 'CAL SAVE' to persist");
            return true;
        } else {
            Serial.print("\r\nERROR: Unknown parameter '");
            Serial.print(param);
            Serial.println("'. Valid: CV, DUTV, DUTC, SHUNT");
            return false;
        }
    }

    Serial.println("ERROR: Unknown command. Valid commands:");
    Serial.println("  CAL SHOW              - Display current values");
    Serial.println("  CAL CV <value>        - Set cvCalFactor");
    Serial.println("  CAL DUTV <value>      - Set dutVcalib");
    Serial.println("  CAL DUTC <value>      - Set DAC_ADC_TOLERANCE");
    Serial.println("  CAL SHUNT <value>     - Set shuntVcalib");
    Serial.println("  CAL SAVE              - Save to persistent storage");
    Serial.println("  CAL RESET             - Reset to defaults");
    Serial.println("  CAL EXIT              - Exit calibration mode");
    return false;
}
