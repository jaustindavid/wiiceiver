/*
 * (CC BY-NC-SA 4.0) 
 * http://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * WARNING WARNING WARNING: attaching motors to a skateboard is 
 * a terribly dangerous thing to do.  This software is totally
 * for amusement and/or educational purposes.  Don't obtain or
 * make a wiiceiver (see below for instructions and parts), 
 * don't attach it to a skateboard, and CERTAINLY don't use it
 * to zip around with just a tiny, ergonomic nunchuck instead
 * of a bulky R/C controller.
 *
 * This software is made freely available.  If you wish to 
 * sell it, don't.  If you wish to modify it, DO! (and please
 * let me know).  Much of the code is derived from others out
 * there, I've made attributuions where appropriate.
 *
 * http://austindavid.com/wiiceiver
 *  
 * latest software & schematic: 
 *    https://github.com/jaustindavid/wiiceiver
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 12 May 2014
 *
 */
 
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

#include <avr/wdt.h> 
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
// #define DEBUGGING_I2C

#define WIICEIVER_HEAD_VERSION "0.5 alpha"
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


/********
 *  WATCHDOG STUFF
 * I referenced the following:
 * http://forum.arduino.cc/index.php?topic=63651.0
 * http://tushev.org/articles/arduino/item/46-arduino-and-watchdog-timer
 * http://www.ivoidwarranties.com/2012/02/arduino-mega-optiboot.html
 *
 ********/
 
/*
 * setup a watchdog timer for the given reset interval
 *
 * constants: WDTO_250MS, WDTO_8S, WDTO_60MS, etc
 */
void watchdog_setup(byte wd_interval) {
  wdt_reset();
  wdt_enable(wd_interval);
  cli();
  WDTCSR |= (1<<WDCE) | (1<<WDE) | (1<<WDIE); 
  sei();
} // watchdog_setup(unsigned int wd_interval)


// increment a _W_atch _D_og _C_ounter stored in EEPROM, for future debugging
ISR(WDT_vect) {
  byte wdt_counter = EEPROM.read(EEPROM_WDC_ADDY);
  EEPROM.write(EEPROM_WDC_ADDY, wdt_counter + 1);
} // ISR for the watchdog timer



// display the current value of the watchdog counter
void display_WDC(void) {
  byte wdt_counter = EEPROM.read(EEPROM_WDC_ADDY);
  Serial.print(F("Watchdog Resets: "));
  Serial.println((byte)(wdt_counter + 1));
} // display_WDC()



void setup()   {
  wdt_disable();
  Serial.begin(115200);
  
  Serial.print(F("Starting Wiiceiver Head v"));
  Serial.println(F(WIICEIVER_HEAD_VERSION));
  
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  
  // init done

  Wire.begin(WIICEIVER_I2C);
  Wire.onReceive(receiveEvent); // register event
  splashScreen();

  #ifdef DEBUGGING
  Serial.println(F("splashed"));
  #endif
  
  delay(1000);
  screen = DISP_DISCHARGE;
  #ifdef MEMORY_FREE_H
  Serial.print(F("free memory: "));
  Serial.println(freeMemory());
  #endif  
  
  watchdog_setup(WDTO_250MS);
}


void loop() {
  wdt_reset();
  update();
  
  #ifdef MEMORY_FREE_H
  if (memTimer.isExpired()) {
    memTimer.reset();
    Serial.print(F("free memory: "));
    Serial.println(freeMemory());
  }
  #endif  
}


