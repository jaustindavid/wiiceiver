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
#define GREEN_LED_ID 1

#define ESC_PPM_ID   2
#define ESC2_PPM_ID  3

#define CSEL_PIN     A0

typedef struct Settings {
  byte HELI_MODE = 0;
};
Settings settings;
 
int CSEL = -1;
void chipSelect (void) {
  pinMode(CSEL_PIN, INPUT);
  int CSEL_VALUE = analogRead(CSEL_PIN);
  if (CSEL_VALUE == 0) {
    // v0: ping A0 is grounded
    CSEL = 0;
  } 

  if (CSEL == -1) {
    Serial.println(F("ERROR: could not determine board type!  RTFS: CSEL in __FILE__"));
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
int pinLocation(byte pinID) {
  byte pinMap[4][2] = {
  // v0, v1 ...
    {2,  2},  // RED_LED     any digital pin
    {3,  3},  // GREEN_LED   any digital pin
    {9,  5},  // ESC_PPM     PWM required
    {0,  6},  // ESC2_PPM    PWM required
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
    settings.HELI_MODE = readSetting(EEPROM_HELI_MODE_ADDY, 0);
    if (settings.HELI_MODE) {
      Serial.println(F("HELI MODE"));
    }
  #else
    settings.HELI_MODE = 0;
  #endif
} // readSettings()

