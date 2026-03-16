/**
 * @file CalibrationManager.h
 * @brief Manages loading and saving calibration values from/to Preferences
 * @author Paul Versteeg
 * @date 2026
 */

#ifndef CALIBRATION_MANAGER_H
#define CALIBRATION_MANAGER_H

#include <Arduino.h>

class CalibrationManager {
   public:
    /**
     * @brief Initialize the calibration manager and load values from Preferences
     * @return true if successful, false if using defaults
     */
    static bool begin();

    /**
     * @brief Load calibration values from Preferences
     * @return true if values exist, false if using defaults
     */
    static bool loadCalibration();

    /**
     * @brief Save current calibration values to Preferences
     * @return true if successful, false on error
     */
    static bool saveCalibration();

    /**
     * @brief Reset calibration to default values
     * @return true if successful
     */
    static bool resetToDefaults();

    /**
     * @brief Print current calibration values to Serial
     */
    static void printCalibration();

    /**
     * @brief Process calibration command from Serial
     * @param command Full command string (e.g., "CAL CV 1.0606")
     * @return true if command valid and executed
     */
    static bool processCommand(const String& command);

   private:
    static const char* PREFS_NAMESPACE;
};

#endif  // CALIBRATION_MANAGER_H
