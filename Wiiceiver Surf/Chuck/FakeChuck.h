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
 
#ifndef FAKECHUCK_H
#define FAKECHUCK_H

#include <EEPROM.h>

/*
 *  A FAKE Nunchuck class -- it should act (programatically) 
 *  like a nunchuck, but will only provide the Y axis and 
 *  C button:
 *    setup()
 *    update()
 *    float X, Y;
 *    bool C, Z;
 *    byte status[6];
 */
class Chuck {

// ports
#define Y_POT A3
#define C_BUTTON A1
#define Z_BUTTON 4 

#define EEPROM_CHUCK_YMIN (EEPROM_CHUCK + 0)
#define EEPROM_CHUCK_YMAX (EEPROM_CHUCK + sizeof(int))

private:
  int y_min = 0, y_max = 1023;
  int y_raw;


  // used in setup() -- reads the internal calibration values
  // might also prime them if needed
  void readCalibration() {
    if (EEPROM.read(EEPROM_CHUCK_YMIN) == 255) {
      // unset
      y_min = y_max = y_raw = analogRead(Y_POT);
      #ifdef DEBUGGING_CHUCK
        Serial.print("Priming ymin & ymax: ");
        Serial.println(y_min);
      #endif

      EEPROM.put(EEPROM_CHUCK_YMIN, y_min);
      EEPROM.put(EEPROM_CHUCK_YMAX, y_max); 
    } else {
      EEPROM.get(EEPROM_CHUCK_YMIN, y_min);
      EEPROM.get(EEPROM_CHUCK_YMAX, y_max);
      #ifdef DEBUGGING_CHUCK
        Serial.print("Setting ymin: ");
        Serial.print(y_min);
        Serial.print("; ymax: ");
        Serial.println(y_max);
      #endif

    }
  } // readCalibration()


  /* 
   * intended to be called frequently, this will update the 
   * internal calibration values if a new (previously) 
   * out-of-range value is observed
   * 
   * in practice, this will probably be pretty active the first
   * time the controller is used, then pretty static after.
   * 
   * Run it through min/max once and it'll probably mostly be done.
   */
  void updateCalibration() {
    if (y_raw < y_min) {
      y_min = y_raw;
      #ifdef DEBUGGING_CHUCK
        Serial.print("Updating ymin: ");
        Serial.println(y_min);
      #endif
      EEPROM.put(EEPROM_CHUCK_YMIN, y_min);
    }

    if (y_raw > y_max) {
      y_max = y_raw;
      #ifdef DEBUGGING_CHUCK
        Serial.print("Updating ymin: ");
        Serial.println(y_min);
      #endif
      EEPROM.put(EEPROM_CHUCK_YMAX, y_max);
    }
  } // updateCalibration()

  
public:
  byte status[6];
  float X, Y;
  bool C, Z;


  // setup the chuck -- send the initialization sequence & start reading data
  void setup(void) {
    pinMode(Y_POT, INPUT);
    pinMode(C_BUTTON, INPUT_PULLUP);
    pinMode(Z_BUTTON, INPUT_PULLUP);

#ifdef DEBUGGING_CHUCK
    Serial.print(millis());
    Serial.print(F(": Chuck.setup() ..."));
#endif

    readCalibration();
    
 #ifdef DEBUGGING_CHUCK
    Serial.print(F("; setup complete @ "));
    Serial.println(millis());
#endif   
  } // void setup(void)


  // update the status[] fields from the nunchuck
  void update(void) {
    y_raw = analogRead(Y_POT);
    updateCalibration();
    Y = map(y_raw, y_min, y_max, 0, 255);
    // invert Y
    Y = 255 - Y;
    C = !digitalRead(C_BUTTON);
    Z = !digitalRead(Z_BUTTON);
    
    #ifdef DEBUGGING_CHUCK
      Serial.print("Y: raw=");
      Serial.print(y_raw);
      Serial.print(", fixed=");
      Serial.print(Y);
      Serial.print("; c=");
      Serial.print(C);
      Serial.print("; z=");
      Serial.println(Z);
    #endif
    memset(status, 0, sizeof(status));
    status[1] = Y;
    if (!C) {
      status[5] = 2;
    }
    if (!Z) {
      status[5] += 1;
    }

    // random data for the accelerometers
    status[2] = random(255);
    status[3] = random(255);
    status[4] = random(255);
  } // void update(void)
};

#endif
