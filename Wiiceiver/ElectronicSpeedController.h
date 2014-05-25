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

#ifndef ELECTRONICSPEEDCONTROLLER_H
#define ELECTRONICSPEEDCONTROLLER_H

/*
 *    ESC wrapper class
 */

class ElectronicSpeedController {
#define ESC_CENTER 90       // angle of the "center"; probably always 90
#define ESC_MAX_ANGLE 180   // angle of "max" deflection

private:
  Servo _esc;
  int angle;                // the angle most recently written to _esc;
  unsigned long lastWrite;  // time in millis() when it was last written

public:

void init(int pin) {
  angle = ESC_CENTER;
  lastWrite = 0;
#ifdef DEBUGGING_ESC
  Serial.print("attaching to pin #");
  Serial.println(pin);
#endif

  _esc.attach(pin, 1000, 2000);
  
#ifdef DEBUGGING_ESC
  Serial.println("initializing ESC...");
#endif
  _esc.write(30);
  delay(1000);
  _esc.write(ESC_CENTER);
#ifdef DEBUGGING_ESC
  Serial.println("done");
#endif
}


/*
 * input: -1 .. 1
 * output: writes +/- ESC_MAX_ANGLE to _esc
 * does *not* write the same angle twice -- possible interference with the PWM :(
 */
void setLevel(float level) {
  int newAngle = (int)(ESC_CENTER + (ESC_MAX_ANGLE - ESC_CENTER) * level);
  if (lastWrite + 20 > millis()) {
#ifdef DEBUGGING_ESC
  Serial.println("Too recently written; skipping");
#endif

    return;
  } 
  if (newAngle == ESC_CENTER || 
      newAngle != angle) {
#ifdef DEBUGGING_ESC
    Serial.print(millis());
    Serial.print(F(": ESC old: "));
    Serial.print(_esc.readMicroseconds());
    Serial.print(F("ms; new angle: "));
    Serial.println(newAngle);
#endif
    angle = newAngle;
    _esc.write(angle);
    lastWrite = millis();
  }
} // void setLevel(float level)


private:
// unused code
#define STEP_DELAY 20
  void _sweep(void) {
    setLevel(0);
    delay(STEP_DELAY);
    for (float level = 0; level <= 1.0; level += 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    for (float level = 1.0; level >= -1.0; level -= 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    for (float level = -1.0; level <= 0; level += 0.1) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    setLevel(0);
    delay(STEP_DELAY);
  } // void _sweep(void)
};  // class ElectronicSpeedController 

#endif
