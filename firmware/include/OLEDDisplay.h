/**
 * @file OLEDDisplay.h
 * @brief OLED display management for SSD1351
 * @author Paul Versteeg
 */

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>

// Display initialization
void setup_oled();
void oled_prep();

// Display update functions
void updateOledDisplay(void* pvParameters);

#endif  // OLED_DISPLAY_H