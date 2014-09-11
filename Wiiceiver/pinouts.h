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
#define ESC2_PPM_ID   6
#define ESC_GROUND 00     // hard-wired

#define WII_POWER_ID 3
#define WII_GROUND 00     // hard-wired

#define WII_SCL_ID   4
#define WII_SDA_ID   5


 
int CSEL = -1;
void chipSelect (void) {
  int A6Sel = analogRead(6);
  int A7Sel = analogRead(7);
  
  if ((A6Sel > 900 ) && (A7Sel < 100 )) {
    CSEL = 2;
  } else {
    pinMode(13, INPUT_PULLUP);
    if (digitalRead(13) == HIGH) {
      CSEL = 0;
    } else {
    // future: perform analogRead on pin 23
    CSEL = 1;
    }
  }
  
#ifdef DEBUGGING_PINS
    Serial.print("Smells like v");
    Serial.println(CSEL);
#endif
}


/*
 * locations are specified in the following table:
 * rows == component
 * columns = version; first column = v0, second = v1, etc
 */
int pinLocation(int pinID) {
  int pinMap[7][3] = {
  // v1, v2, v3
    {8,   8,  8},  // RED_LED     any digital pin
    {7,   6,  6},  // GREEN_LED   any digital pin
    {10,  9,  9},  // ESC_PPM     PWM required
    {9,  11,  5},  // WII_POWER   any digital pin
    {19, 19, 19}, // WII_SCL     A5, don't change
    {18, 18, 18}, // WII_SDA     A4, don't change
    {0,   0, 10}, // ESC2_PPM    PWM required
  };
  
  if (CSEL < 0) {
    chipSelect();
  }
  
  int pin = pinMap[pinID][CSEL];
#ifdef DEBUGGING_PINS
  Serial.print("pin location: [");
  Serial.print(pinID);
  Serial.print("][");
  Serial.print(CSEL);
  Serial.print("] == ");
  Serial.println(pin);
#endif
  return pin;
} // int pinLocation(int pinID)
