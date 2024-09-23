/*
 * This sketch runs only on an ESP32 DevKit1
 * Other devkits or types may have different pin-outs and several only have one core.
 *
 * This sketch is used to test the OLED screen and also to help position it within
 * the hole on the face plate. 
 * It also helps to set the orientation just in case it needs to be rotated.
 * The visible area of the OLED pixels is 27x27mm.
 * 
 * To secure the OLED board, use the studs with an extra 2mm ring such that the glass
 * of the OLED itself does not touch the face plate. It may otherwise crack the glass.
 * Run this program so you can see the dimensions of the visible pixels on the OLED.
 * Position the OLED screen so that the whole display area is visible through the hole
 * in the face plate. You can use tape to temporarily secure it in position.
 * 
 * The next step is to use either glue or solder to fix the extension studs to the 
 * face plate solder pads.
 * 
 * Paulv July 2024
 */

#include <arduino.h> // Just in case I use some Arduino specific functionality
#include <Adafruit_SSD1351.h> // 128x128 RGB OLED display
#include <Adafruit_GFX.h> // needed for OLED display
#include <Fonts/FreeSans18pt7b.h> // display fonts used for V and A digits
#include <Fonts/FreeSans9pt7b.h> // display fonts used for the rest
#include <SPI.h>  // communication method for the OLED display


const String FW_VERSION = "1.0";    // initial version - stripped from main firmware


#define DC 16           // OLED DC
#define RES 17          // OLED RES
#define CS 5            // OLED CS
#define SCL 18          // OLED SCL
#define SDA 23          // OLED SDA/MOSI


// SSD1531 OLED Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

// Use the hardware SPI pins to communicate with the OLED display
Adafruit_SSD1351 tft = 
            Adafruit_SSD1351(
              SCREEN_WIDTH, 
              SCREEN_HEIGHT,
              &SPI, // For an ESP32 SCL(VSPI_CLK) = GPIO18 and sid/mosi (VSPI_MOSI) = GPIO23
              CS,   // ESP32 (VSPI_CS0) = GPIO5
              DC,   // any pin
              RES   // any pin
            ); 

// OLED display vertical line positions, down from the top
#define v_line 25    // Volt line
#define a_line 56    // Amp line
#define p_line 58    // Mode + Power line
#define s_line 87    // Set line + Set suffix
#define m_line 106   // DAC/Menu/Status/Temp line

#define line_1   0 // line position in pixels
#define line_2  20
#define line_3  40
#define line_4  60
#define line_5  80
#define line_6  100

// Horizontal starting positions for the FreeSans18pt large digits
#define ldigit_1 6  // 10.000 or 100.00
#define ldigit_2 25 // 1.000 
#define sdigit_6 114 // V, A en W suffix position

// Horizontal starting positions for the FreeSans9pt digits
#define digit_0 0
#define digit_1 10
#define digit_2 20
#define digit_3 30
#define digit_4 40
#define digit_5 50
#define digit_6 60
#define digit_7 70
#define digit_8 80
#define digit_9 90
#define digit_10 100

// OLED Color definitions
#define BLACK     0x0000
#define BLUE      0x001F
#define L_BLUE    0x0010
#define RED       0xF800
#define BR_RED	  0xF810
#define GREEN     0x07E0
#define DK_GREEN  0x03E0
#define CYAN      0x07FF
#define ORANGE    0xFD20
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0  
#define WHITE     0xFFFF
#define L_GREY    0xC618 // 0x8410
#define D_GREY    0x7BEF // 0x4208
#define GR_YELLOW 0xAFE5



/*
 * Forward Function Declarations
 */

void oled_test(); // setup for measurements


/*
 * The initial setup code
 */
void setup(void) {
  Serial.begin(9600); // battery test must have 9600 Bd, debugging can be done at 15200 Bd (download speed is unaffected)
  while (!Serial);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  Serial.print("\n\r\n\rOLED Test - Version ");
  Serial.println(FW_VERSION);

  // setup the OLED display
  Serial.println("starting tft");
  tft.begin();
  tft.setRotation(0); // rotate the display if needed (0=0 dgs 1=90, 2=180, 3=270)
  oled_test(); // splash screen

}


/*
 * Initial setup for the OLED, show welcome message
 */
void oled_test() {
  Serial.print("oled prep ");
  // clear the screen
  tft.fillScreen(BLACK);
  // display welcome msg with credits
  tft.setTextColor(WHITE);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.setCursor(digit_2, v_line);
  tft.print(" OLED Test");
  tft.setCursor(digit_0+5, a_line);
  tft.print("Version");
  tft.setCursor(digit_7, a_line); // from left side, down
  tft.print(FW_VERSION); // send to OLED display
  tft.setCursor(digit_1, s_line); // from left side, down
  tft.print("Paulv & BudB"); // send to OLED display  
  vTaskDelay(5000 / portTICK_PERIOD_MS); // show it for 5 seconds

  tft.fillScreen(WHITE);

  // configuration help; de-activate when done
  // set the outside corner indicators by using one pixel 
  // tft.drawPixel(0, 0, WHITE);
  // tft.drawPixel(0, 127, WHITE);
  // tft.drawPixel(127, 0, WHITE);
  // tft.drawPixel(127, 127, WHITE);
  //vTaskDelay(2000 / portTICK_PERIOD_MS); // show it for 2 seconds
  //tft.fillScreen(BLACK);
  Serial.println("done...");
}


void loop() {
  // not used
} 

// -- END OF FILE --

