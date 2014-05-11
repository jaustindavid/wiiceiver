/*
 * version detection -- pin layouts change over time ...
 * historical note: CSEL = "chip select"
 *
 * "v0" boards -- one shield and the first two protos
 *    pin 13 will float (INPUT_PULLUP -> HIGH)
 * "v1" boards -- hand-laid on the AdaFruit white breadboard
 *    pin 13 is grounded; A0 would float
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
#define ESC_GROUND 00     // hard-wired

#define WII_POWER_ID 3
#define WII_GROUND 00     // hard-wired

#define WII_SCL_ID   4
#define WII_SDA_ID   5


 
int CSEL = -1;
void chipSelect (void) {
  pinMode(13, INPUT_PULLUP);
  if (digitalRead(13) == HIGH) {
    CSEL = 0;
  } else {
    // future: perform analogRead on pin 23
    CSEL = 1;
  }
  
#ifdef DEBUGGING
    Serial.print("Smells like v");
    Serial.println(CSEL);
#endif
}


/*
 * locations are specified in the following table:
 * rows == component
 * columns = version; first column = v1, second = v2, etc
 */
int pinLocation(int pinID) {
  int pinMap[6][2] = {
  // v1, v2
    {8,   8}, // RED_LED     any digital pin
    {7,   6}, // GREEN_LED   any digital pin
    {10,  9}, // ESC_PPM     PWM required
    {9,  11}, // WII_POWER   any digital pin
    {19, 19}, // WII_SCL     A5, don't change
    {18, 18}, // WII_SDA     A4, don't cange
  };
  
  if (CSEL < 0) {
    chipSelect();
  }
  
  int pin = pinMap[pinID][CSEL];
#ifdef DEBUGGING
  Serial.print("pin location: [");
  Serial.print(pinID);
  Serial.print("][");
  Serial.print(CSEL);
  Serial.print("] == ");
  Serial.println(pin);
#endif
  return pin;
} // int pinLocation(int pinID)
