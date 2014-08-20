/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using SPI to communicate
4 or 5 pins are required to interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// If using software SPI (the default case):
#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#define DEBUGGING

#define WIICEIVER_HEAD_VERSION "0.1"
// addys for vars stored in EEPROM
#define EEPROM_Y_ADDY            0
#define EEPROM_AUTOCRUISE_ADDY   1
#define EEPROM_WDC_ADDY          2
#define EEPROM_LOGGER_ADDY       16

// #include "MemoryFree.h"
#include "wiiceiver_i2c.h"
#include "utils.h"
#include "display.h"
#include "wiiceiver_i2c.h"

Timer memTimer(10000);


#define WIICEIVER_HEAD_VERSION "0.1"

// addys for vars stored in EEPROM
#define EEPROM_Y_ADDY            0
#define EEPROM_AUTOCRUISE_ADDY   1
#define EEPROM_WDC_ADDY          2
#define EEPROM_LOGGER_ADDY       16


void setup()   {
  Serial.begin(115200);
  
  Serial.print(F("Starting Wiiceiver Head v"));
  Serial.println(F(WIICEIVER_HEAD_VERSION));
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  
  // init done

  Wire.begin(WIICEIVER_I2C);
  Wire.onReceive(receiveEvent); // register event
  splashScreen();
  Serial.println(F("splashed"));
  delay(1000);
  screen = DISP_DISCHARGE;
  #ifdef MEMORY_FREE_H
  Serial.print(F("free memory: "));
  Serial.println(freeMemory());
  #endif  

}


void loop() {
  // Serial.println(F("loop"));
  update();
  
  #ifdef MEMORY_FREE_H
  if (memTimer.isExpired()) {
    memTimer.reset();
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
  }
  #endif  
}


