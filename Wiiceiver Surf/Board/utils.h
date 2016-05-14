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
 * latest software & hardware: https://github.com/jaustindavid/wiiceiver
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 12 May 2014
 * 02 Apr 2016
 *
 */

#ifndef UTILS_H
#define UTILS_H

 
/*
 * version detection -- pin layouts change over time ...
 * historical note: CSEL = "chip select"
 *
 * "v0" boards -- one shield and the first two protos
 *    pin 13 will float (INPUT_PULLUP -> HIGH)
 * "v1" boards -- hand-laid on the AdaFruit white breadboard
 *    pin 13 is grounded; A0 would float
 * "v2" board -- Peter Homann's design,  peter@homanndesigns.com
 *    A6/A7 with fixed voltages (note: DIP package doesn't have A6/A7)
 *    http://www.homanndesigns.com for more infoes!
 * future boards: a voltage divider on A0 & analog leveling
 *    pin 13 grounded; A0 at a predictable level
 */

/*
 * Pin IDs -- NOT LOCATIONS !!!
 * don't change these ever; see "pinLocation" below for
 * actual locations
 */
#define RED_LED_ID   0
#define YELLOW_LED_ID 1
#define GREEN_LED_ID 2

#define ESC1_PPM_ID  3
#define ESC2_PPM_ID  4
#define ESC3_PPM_ID  5
#define ESC4_PPM_ID  6

#define BUTTON1_ID   7
#define BUTTON2_ID   8


#define CSEL_PIN     A0

typedef struct Settings {
  byte HELI_MODE = 0;
};
Settings settings;
 
int CSEL = 1;
void chipSelect (void) {
  pinMode(CSEL_PIN, INPUT_PULLUP);
  int CSEL_VALUE = analogRead(CSEL_PIN);
  if (CSEL_VALUE == 0) {
    // v0: pin A0 is grounded
    CSEL = 0;
  } else if (ABS(CSEL_VALUE - 512) < 50) {
    // v1: pin A0 has 10k GND + 10K VCC
    CSEL = 1;
  }

  if (CSEL == -1) {
    Serial.println(F("ERROR: could not determine board type!  RTFS: CSEL in utils.h"));
    Serial.println(F("Using v0, but it's wrong."));
    CSEL = 0;
  }
  
#ifdef DEBUGGING_PINS
    Serial.print(F("Smells like v"));
    Serial.println(CSEL);
#endif
} // chipselect()


/*
 * locations are specified in the following table:
 * rows == component
 * columns = version; first column = v0, second = v1, etc
 */
byte pinLocation(byte pinID) {
  byte pinMap[9][2] = {
  // v0, v1 ...
    {2,  2},  // RED_LED     any digital pin
    {0,  3},  // YELLOW_LED  any digital pin
    {3,  4},  // GREEN_LED   any digital pin
    {9,  5},  // ESC_PPM     any digital pin
    {0,  6},  // ESC2_PPM    any digital pin
    {0,  7},  // ESC3_PPM    any digital pin
    {0,  8},  // ESC4_PPM    any digital pin
    {0, A4},  // BUTTON1     any digital pin
    {0, A5}   // BUTTON2     any digital pin
  };
  
  if (CSEL < 0) {
    chipSelect();
  }
  
  byte pin = pinMap[pinID][CSEL];
#ifdef DEBUGGING_PINS
  Serial.print(F("Pin location: ["));
  Serial.print(pinID);
  Serial.print(F("]["));
  Serial.print(CSEL);
  Serial.print(F("] == "));
  Serial.println(pin);
#endif
  return pin;
} // int pinLocation(int pinID)


// reads a setting from an EEPROM address, returns that or a default
// note that uninitialized EEPROM is all ones
byte readSetting(int eeprom_addy, byte default_value) {
  Serial.print("reading ");
  Serial.print(eeprom_addy);
  byte value = EEPROM.read(eeprom_addy);
  Serial.print(": ");
  Serial.println(value);
  return value == 255 ? default_value : value;
} // byte readSetting(int eeprom_addy, byte default_value)


// reads the acceleration profile setting (default: 2) and returns a
// multiplier [0.5 .. zillion]
float getProfileMultiplier(void) {
    byte accelProfile = readSetting(EEPROM_ACCELPROFILE_ADDY, 2);
    float multiplier = 1.0;
    
    switch (accelProfile) {
      case 0: 
        multiplier = 0.5;
        break;
      case 1:
        multiplier = 0.75;
        break;
      case 3: 
        multiplier = 1.25;
        break;
      case 4:
        multiplier = 1.5;
        break;
      case 5:
        multiplier = 2.0;
        break;
      case 6: // raw input, basically
        multiplier = 100.0;
        break;
      case 2: // FALLTHROUGH
      default:
        multiplier = 1.0;
        break;
    }
    return multiplier;
} // float getProfileMultiplier()


void readSettings(void) {
  #ifdef ALLOW_HELI_MODE
    #ifdef FORCE_HELI_MODE
      settings.HELI_MODE = 1;
    #else
      settings.HELI_MODE = readSetting(EEPROM_HELI_MODE_ADDY, 0);
    #endif
    if (settings.HELI_MODE) {
      Serial.println(F("HELI MODE"));
    }
  #else
    settings.HELI_MODE = 0;
  #endif
} // readSettings()

#endif
