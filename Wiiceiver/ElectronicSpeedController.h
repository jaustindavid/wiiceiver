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
  bool dualESC;
  Servo _esc1, _esc2;
  int angle;                // the angle most recently written to _esc;
  int microseconds;         // the time (ms) most recently written
  unsigned long lastWrite;  // time in millis() when it was last written

public:

void init(int pin1, int pin2) {
  angle = -1;
  lastWrite = 0;
#ifdef DEBUGGING_ESC
  Serial.print("attaching to pin1 #");
  Serial.println(pin1);
  Serial.print(", pin2 #");
  Serial.print(pin2);
#endif
  _esc1.attach(pin1, 1000, 2000);
  if (pin2) {
    dualESC = true;
    _esc2.attach(pin2, 1000, 2000);
    Serial.println("Dual ESC!");
  } else {
    dualESC = false;
  }
  
  
  delay(21);  // give the "last written" protection room to work
#ifdef DEBUGGING_ESC
  Serial.println("initializing ESC...");
#endif

  syncESC();
  setLevel(0);
  
#ifdef DEBUGGING_ESC
  Serial.println("done");
#endif
} // void init(int pin)


/*
 * input: -1 .. 1
 * output: writes +/- ESC_MAX_ANGLE to _esc
 * does *not* write the same angle twice -- possible interference with the PWM :(
 * Servo will continually pulse the last-written angle
 */
void setLevel(float level) {
  int newAngle = (int)(ESC_CENTER + (ESC_MAX_ANGLE - ESC_CENTER) * level);
  /* proably dead: attempt to drive usecs to game throttle positions */
  int newUs;
  if (level > 0) {
    newUs = (int)(1600 + (2000 - 1600) * level);
  } else if (level < 0) {
    newUs = (int)(1400 + (2000 - 1400) * level);
  } else {
    newUs = 1500;
  }
  
  if (lastWrite + 19 > millis()) {
#ifdef DEBUGGING_ESC
    Serial.println("Too recently written; skipping");
#endif
    return;
  } 
  if (newAngle != angle) {
#ifdef DEBUGGING_ESC
    Serial.print(millis());
    Serial.print(F(": ESC old: "));
    Serial.print(_esc.readMicroseconds());
    Serial.print(F("us; new angle: "));
    Serial.print(newAngle);
    Serial.print(F(" = "));
    Serial.print(newUs);
    Serial.print(F("us"));
#endif
    angle = newAngle;
    microseconds = newUs;
    _esc1.write(angle);
    if (dualESC) {
      _esc2.write(angle);
    }
    // _esc.writeMicroseconds(microseconds);
#ifdef DEBUGGING_ESC
    Serial.print(F(": ESC now: "));
    Serial.println(_esc.readMicroseconds());
#endif
    
    lastWrite = millis();
  }
} // void setLevel(float level)


private:

#define STEP_DELAY 20
#define SYNC_LIMIT 0.6

  void sweep(float startLevel, float endLevel, float step) {
#ifdef DEBUGGING_ESC
    Serial.print("sweep: ");
    Serial.print(startLevel);
    Serial.print(" to ");
    Serial.print(endLevel);
    Serial.print(" by ");
    Serial.println(step);
#endif
    for (float level = startLevel; level != endLevel; level += step) {
      setLevel(level);
      delay(STEP_DELAY);
    }
    setLevel(endLevel);
  } // sweep(float startLevel, float endLevel, float step)

  
  // startup sequence: some small range of inputs, then idle
  void syncESC(void) {
/*    
    sweep(0, SYNC_LIMIT, SYNC_LIMIT/2);
    delay(500);
    sweep(SYNC_LIMIT, -SYNC_LIMIT, -SYNC_LIMIT/2);
    delay(500);
*/
    sweep(-SYNC_LIMIT, 0, SYNC_LIMIT/2);
    delay(1000);
  } // void syncESC(void)
  
};  // class ElectronicSpeedController 

#endif
