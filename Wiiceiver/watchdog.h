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
 * latest software: https://github.com/jaustindavid/wiiceiver
 * schematic & parts: http://www.digikey.com/schemeit#t9g
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 12 May 2014
 *
 */
 
#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>

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


// interupt service routine for the watchdog timer
ISR(WDT_vect) {
  byte wdt_counter = EEPROM.read(EEPROM_WDC_ADDY);
  EEPROM.write(EEPROM_WDC_ADDY, wdt_counter + 1);
} // ISR for the watchdog timer


// display the current watchdog counter
void display_WDC(void) {
  byte wdt_counter = EEPROM.read(EEPROM_WDC_ADDY);
  Serial.print(F("Watchdog Resets: "));
  Serial.println((byte)(wdt_counter + 1));
} // display_WDC()


#endif
