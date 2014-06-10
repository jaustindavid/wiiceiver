/*
 * This will "factory reset" a wiiceiver chip by returning saved values
 * to default, as present in a new / unflashed ATMega328P
 * 
 * This is NOT MAGIC -- it doesn't really do anything you can't easily
 * do in a working wiiceiver, but it might help troubleshooting in 
 * specific cases.
 *
 * To reset a wiiceiver the easy way:
 * 1) start it and hold C; count to 10
 * 2) hold down until the wheels stop, then all the way right for 5 secs
 * 
 * This will re-calibrate the nunchuck stick, and set auto-cruise to 0
 *
 * If for whatever reason you want to do the hard way:
 * 1) upload this to your wiiceiver
 * 2) observe both red & green LEDs lot solid
 * 3) upload the wiiceiver code
 * 4) observe wiiceiver acting like a "new" device, with
 *    default settings for the joystick calibration and auto-cruise
 *
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 10 June 2014
 */

#include <EEPROM.h>

#define RED_LED 8
#define GREEN_LED 6

// returns true if the value at the specified location is set correctly
boolean fixValue(int address, byte desiredValue) {
  byte storedValue = EEPROM.read(address);
  if (storedValue != desiredValue) {
    EEPROM.write(address, desiredValue);
  }
  
  return EEPROM.read(address) == desiredValue;
}

void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);

  if (fixValue(0, 255)) {
    digitalWrite(RED_LED, HIGH);
  }
  
  if (fixValue(1, 255)) {
    digitalWrite(GREEN_LED, HIGH);
  }

}

void loop() {
}
