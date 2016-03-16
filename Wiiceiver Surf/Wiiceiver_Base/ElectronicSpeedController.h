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
  SoftwareServo _esc1, _esc2;
  int angle;                // the angle most recently written to _esc;
  int angleCtr;
  int microseconds;         // the time (ms) most recently written
  unsigned long lastWrite;  // time in millis() when it was last written

public:

  void init(int pin1, int pin2) {
    angle = -1;
    angleCtr = 0;
    lastWrite = 0;
    #ifdef DEBUGGING_ESC
      Serial.print("attaching to pin1 #");
      Serial.println(pin1);
      Serial.print(", pin2 #");
      Serial.print(pin2);
    #endif
    _esc1.attach(pin1);
    if (pin2) {
      dualESC = true;
      _esc2.attach(pin2);
      Serial.println("Dual ESC!");
    } else {
      dualESC = false;
    }
    
    
    delay(21);  // give the "last written" protection room to work
    #ifdef DEBUGGING
      Serial.println("initializing ESC...");
    #endif
  
    // syncESC();
    // calibrateESC();
    // sync2();
    sync3();
    setLevel(0);
    
    #ifdef DEBUGGING
      Serial.println("done");
    #endif
  } // void init(int pin)


  /*
   * input: -1 .. 1
   * output: writes +/- ESC_MAX_ANGLE to _esc
   * does *not* write the same angle twice -- possible interference with the PWM :(
   * Servo will continually pulse the last-written angle anyway
   * 
   * HELI_ESC: 
   *   -1 .. 0 => angle 0
   *    0 .. 1 => angle 0 .. 180
   */
  void setLevel(float level) {  
    int newAngle = (int)(ESC_CENTER + (ESC_MAX_ANGLE - ESC_CENTER) * level);
    #ifdef ALLOW_HELI_MODE
      if (settings.HELI_MODE) {
        if (level <= 0) {
          newAngle = 0;
        } else {
          newAngle = (int)(ESC_MAX_ANGLE * level);
        }
      }
    #endif

    if (newAngle != angle) {
      #ifdef DEBUGGING_ESC
        Serial.print(millis());
        Serial.print(F(": ESC old: "));
        // Serial.print(_esc1.readMicroseconds());
        Serial.print(F("us; new angle: "));
        Serial.print(newAngle);
        Serial.print(F(" = "));
      #endif
      angle = newAngle;
      angleCtr = 0;
      _esc1.write(angle);
      if (dualESC) {
        _esc2.write(angle);
      }
      #ifdef DEBUGGING_ESC
          //Serial.print(F(": ESC now: "));
          //Serial.println(_esc1.readMicroseconds());
      #endif
      
      lastWrite = millis();
      /*
    } else {
      
      if (angleCtr++ > 10) {
        setLevel(level + 0.01);
      }
      */
    }
  } // void setLevel(float level)


private:

/* DEAD CODE
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
*/

  // http://electronics.stackexchange.com/questions/24826/activating-electronic-speed-control-with-arduino  
  // startup sequence: some small range of inputs, then idle
  void syncESC(void) {
    setLevel(0);
    delay(100);
    setLevel(1);
    delay(50);
    setLevel(-1);
    delay(50);
    setLevel(0);
    delay(100);
    setLevel(1);
    delay(100);
    setLevel(0);
    delay(100);
  } // void syncESC(void)
  
  void sync2(void) {
    setLevel(-1);
    delay(3000);
    setLevel(0);
  }
  
  void sync3(void) {
    setLevel(1);
    delay(500);
    setLevel(-1);
    delay(500);
    setLevel(0);
  }
  
  void calibrateESC(void) {
    setLevel(1);
    delay(2000);
    setLevel(-1);
    delay(1000);
    setLevel(0);
  }
};  // class ElectronicSpeedController 

#endif
