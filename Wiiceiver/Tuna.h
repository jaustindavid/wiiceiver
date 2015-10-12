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

/*
 * This is the "tuner" for your wiiceiver -- the setup UI.
 */
 
#ifndef TUNA_H
#define TUNA_H


#include "elapsedMillis.h"
#include "Chuck.h"
#include "Throttle.h"
#include "watchdog.h"


// reset all EEPROM data to 255
// as if the chip were fresh & new
void factory_reset(void) {
  Serial.println("Executing factory reset lol");
  for (byte addy = 0; addy < 10; addy++) {
    EEPROM.update(addy, 255);
  }
  watchdog_setup(WDTO_250MS);
  delay(500);  // watchdog should fire & restart the whole thing
} // factory_reset()


void flash(Blinker led, byte nr_flashes) {
  for (byte i = 0; i < nr_flashes; i++) {
    led.low();
    delay(100);
    led.high();
    delay(150);
  }
  led.low();
  delay(100);  
} // flash(led, nr_flashes)


void flash(Blinker led1, Blinker led2, byte nr_flashes) {
  for (byte i = 0; i < nr_flashes; i++) {
    led1.low();
    led2.low();
    delay(100);
    led1.high();
    led2.high();
    delay(150);
  }
  led1.low();
  led2.low();
  delay(100);  
} // flash(led1, led2, nr_flashes)


void ui_checkreset(void) {
  Serial.println("UI: check reset");
  byte ctr = 0;
  do {
    if (++ctr % 2) {
      red.high();
      green.low();
    } else {
      green.high();
      red.low();
    }
    chuck.update();
    delay(200);
    if (ctr > 25) {
      factory_reset(); // never returns
    }
  } while (chuck.C && chuck.Z);
  red.high();
  green.high();
  delay(500);
} // ui_checkreset()


void calibrateChuck(void) {
  chuck.calibrateCenter();
  chuck.writeEEPROM();
} // calibrateChuck()


void ui_getThrottle(byte blinks, byte eeprom_addy, int sign) {
  float throttle = 0.01 * readSetting(eeprom_addy, 0) * sign;
  Serial.print(F("UI: get throttle #"));
  Serial.println(blinks);
  flash(green, blinks);
  delay(1000);
  green.start(constrain(abs(throttle * 20), 1, 20));
  ESC.setLevel(throttle);
  do {
    chuck.update();
    delay(20);
    green.run();
    
    if (abs(chuck.Y) > 0.5) {
      throttle += 0.005 * chuck.Y;
      throttle = constrain(abs(throttle), 0, 1) * sign;
      ESC.setLevel(throttle);
      green.update(constrain(abs(throttle * 20), 1, 20));
      Serial.println(throttle);
    } 
    
  } while (!chuck.C);
  Serial.print(F("Saving throttle #"));
  Serial.print(blinks);
  Serial.print(F(" = "));
  Serial.println(throttle);
  EEPROM.update(eeprom_addy, 100*abs(throttle));
  ESC.setLevel(0);
  green.high();
  flash(red, blinks);
  red.high();
  delay(1000);
} // ui_getThrottle(addy, blinks)



// NOTE: the value # flashed is +1, can't flash "0" times :/
int ui_getValue(byte blinks, byte valueAddy, byte defaultValue, byte maxValue) {
  int newValue = readSetting(valueAddy, defaultValue);
  Serial.print(F("getting value #"));
  Serial.println(blinks);
  flash(green, blinks);
  delay(1000);
  flash(green, newValue+1);
  do {
    chuck.update();
    if (chuck.Y > 0.5) {
      newValue ++;
      if (newValue > maxValue) {
        newValue = 0;
      }
      flash(green, newValue+1);
      delay(20);
      chuck.update();
    } else if (chuck.Y < -0.5) {
      newValue --;
      if (newValue < 0) {
        newValue = maxValue;
      }
      flash(green, newValue+1);
      delay(20);
      chuck.update();
    }
    delay(20);
  } while (!chuck.C);
  Serial.print(F("Saving value #"));
  Serial.print(blinks);
  Serial.print(F(" as "));
  Serial.println(newValue);
  EEPROM.update(valueAddy, newValue);
  green.high();
  flash(red, blinks);
  red.high();
  delay(1000);
  return newValue;
} // int ui_getValue(byte blinks, byte defaultValue)


void showTunaSettings(void) {
  Serial.print(F("Heli mode: "));
  Serial.println(readSetting(EEPROM_HELI_MODE_ADDY, 255));
  #ifndef ALLOW_HELI_MODE
    Serial.println("Heli mode disabled");
  #endif 
  Serial.print(F("Auto cruise: "));
  Serial.println(readSetting(EEPROM_AUTOCRUISE_ADDY, 255));
  Serial.print(F("Drag brake: "));
  Serial.println(readSetting(EEPROM_DRAGBRAKE_ADDY, 255));
  Serial.print(F("Acceleration profile: "));
  Serial.println(readSetting(EEPROM_ACCELPROFILE_ADDY, 255));
} // showTunaSettings()


void do_ui() {
  ESC.setLevel(0);
  red.high();
  green.high();
  ui_checkreset();  // may never actually return ...
  calibrateChuck();

  #ifdef ALLOW_HELI_MODE
    ui_getValue(1, EEPROM_HELI_MODE_ADDY, 0, 1);    // HELI_MODE ranges 0..1; default 0
  #endif
  ui_getThrottle(3, EEPROM_AUTOCRUISE_ADDY, 1);
  ui_getThrottle(4, EEPROM_DRAGBRAKE_ADDY, -1);
  ui_getValue(5, EEPROM_ACCELPROFILE_ADDY, 2, 6); // accel profile 0..6; default 2
  flash(red, green, 10);
  showTunaSettings();
  readSettings();
  throttle.init();
} // do_ui()


/*
 * enter the tuna if C+Z / no throttle, for 250 attempts (5s)
 */
void tuna() {
  static int attempts = 0;
  static elapsedMillis lastAttempt = 0;
  
  if (! (chuck.C && chuck.Z) 
      || abs(throttle.getThrottle()) > THROTTLE_MIN) {
    attempts = 0;
    return;
  }
  
  if (++attempts < 250) {
    // not there yet
    return;
  }
  
  Serial.println(F("Tuning!"));
  
  wdt_disable();
  red.stop();
  green.stop();
  ESC.setLevel(0);
  do_ui();
  red.start(1);
  green.start(1);
  watchdog_setup(WDTO_250MS);
  attempts = 0;
} // tuna()


#endif
