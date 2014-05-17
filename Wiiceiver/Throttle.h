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

#ifndef THROTTLE_H
#define THROTTLE_H

// not strictly necessary, but a nice reminder
#include "Chuck.h"
#include "Smoother.h"

/*
 * Manages the throttle input; presents a smoothed output, [ -1 .. 1 ]
 */

class Throttle {
#define EEPROM_AUTOCRUISE_ADDY 1
  private:
    float autoCruise, throttle, smoothed, previousCruiseLevel;
    int xCounter;
    Smoother smoother;
    
    
    void readAutoCruise(void) {
      byte storedValue = EEPROM.read(EEPROM_AUTOCRUISE_ADDY);
#ifdef DEBUGGING_THROTTLE
      Serial.print("Read autoCruise from EEPROM: ");
      Serial.print(storedValue);
#endif
      if (storedValue > 0 && storedValue < 100) {
        autoCruise = 0.01 * storedValue;
#ifdef DEBUGGING_THROTTLE
        Serial.print("; setting autoCruise = ");
        Serial.println(autoCruise);
#endif  
      } else {
#ifdef DEBUGGING_THROTTLE
        Serial.print("; ignoring, setting autoCruise = ");
        Serial.println(autoCruise);
#endif        
      }
    } // readAutoCruise(void) 
    
    
    void writeAutoCruise(void) {
      int storedValue = autoCruise * 100;
      EEPROM.write(EEPROM_AUTOCRUISE_ADDY, storedValue);
#ifdef DEBUGGING_THROTTLE
      Serial.print("Storing autoCruise as ");
      Serial.println(storedValue);
#endif         
    }
    
    
  public:    
    
    Throttle() {
      smoother = Smoother();
      throttle = 0;
      autoCruise = THROTTLE_MIN_CC;
      xCounter = 0;
    } // Throttle()
    

    void init(void) {
      readAutoCruise();
    }
    
    /*
     * returns a smoothed float [-1 .. 1]
     *
     * Theory of Operation: identify the throttle position (joystick angle), 
     *   then return a smoothed representation
     *
     *   if C is pressed, "cruise control":
     *      set "cruise" to last joystick position
     *      if joystick == up, increment throttle position (Z button: 3x increment)
     *      if joystick == down, decrement throttle position  (Z button: 3x decrement)
     *   else throttle position == chuck.Y joystick position
     *   return a smoothed value from the throttle position (Z button: 4x less smoothed)
     */
    float update(Chuck chuck) {
#ifdef DEBUGGING_THROTTLE
      Serial.print("Throttle: ");
      Serial.print("y=");
      Serial.print(chuck.Y, 4);
      Serial.print(", ");
      Serial.print("c=");
      Serial.print(chuck.C);
      Serial.print("; ");
#endif

      // we're looking for autoCruise, so do that;
      // don't change the throttle position
      if (checkAutoCruise(chuck)) {
        return throttle; 
      }
      

      // CC return: in CC mode, drop C, then resume shortly after 
      // (with no other input) -- resume the previous CC 
      if (checkCruiseReturn(chuck)) {
#ifdef DEBUGGING_THROTTLE
        Serial.print(" returning previous cruise level");
#endif         
        throttle = previousCruiseLevel;
      }  
      
      if (chuck.C) { // cruise control!
        // holding Z + C == faster CC changes
        if (chuck.Y > 0.5 && throttle < 1.0) {
          throttle += THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
        } else {
          if (chuck.Y < -0.5 && throttle > 0) {
            throttle -= THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
          } else {
            if (throttle < autoCruise) {
              throttle += THROTTLE_CC_BUMP * (chuck.Z ? 4 : 2);
            }
          }
        } // if (chuck.Y > 0.5 && throttle < 1.0) - else
      } else { // if (checkAutoCruise() && chuck.C)
        throttle = chuck.Y;
        // "center" the joystick by enforcing some very small minimum
        if (abs(throttle) < THROTTLE_MIN) {
          throttle = 0;
        }
      } // if (chuck.C) -- else

      // if formerly braking, just go to idle (don't smooth the brake release)
      if (abs(chuck.Y) < THROTTLE_MIN &&
          smoothed < 0) {
        zero();
      } else {
        // holding Z == more aggressive smoothing (e.g., more responsive)
        smoothed = smoother.compute(throttle, THROTTLE_SMOOTHNESS * (chuck.Z ? 4 : 1));
      }
      
#ifdef DEBUGGING_THROTTLE
      Serial.print(F("position: "));
      Serial.print(throttle, 4);
      Serial.print(F(" smoothed: "));
      Serial.println(smoothed, 4);
#endif

      return smoothed;
    } // float update(void)
    
    
    /*
     * returns 'true' if we're in a "set autocruise level" state:
     *   while in cruise (chuck.C), with no throttle input (chuck.Y =~ 0), 
     *   nonzero throttle, full X deflection (chuck.X > 0.75) ... for N cycles
     * set "autoCruise" to the current throttle level.
     *   
     */
    bool checkAutoCruise(Chuck chuck) {
      if (! chuck.C) {
#ifdef DEBUGGING_THROTTLE
        Serial.println("checkAutoCruise: no C");
#endif
        xCounter = 0;
        return false;
      }
      if (abs(chuck.Y) < 0.5 && 
          abs(chuck.X) > 0.75) {
        ++xCounter;
#ifdef DEBUGGING_THROTTLE
        Serial.print("checkAutoCruise: xCounter = ");
        Serial.println(xCounter);
#endif
        if (xCounter == 150) { // ~3s holding X
          setAutoCruise();  
        }
        return true;
      } else {
        xCounter = 0;
#ifdef DEBUGGING_THROTTLE
        Serial.println("checkAutoCruise: no X or Y");
        Serial.print("x = ");
        Serial.print(abs(chuck.X));
        Serial.print(", y = ");
        Serial.println(abs(chuck.Y));
#endif        
        return false;
      }
    } // bool checkAutoCruise(void)
    
    
    void setAutoCruise(void) {
#ifdef DEBUGGING_THROTTLE
      Serial.print("Setting autoCruise = ");
      Serial.println(throttle);
#endif
      autoCruise = throttle;
      writeAutoCruise();
    } // setAutoCruise(void)
    
    
    float getThrottle(void) {
      return smoothed;
    } // float getThrottle()
    
    
    void zero(void) {
      throttle = smoothed = 0;
      smoother.zero();
    } // void zero(void)
    
    
    /*
     * true if we should return to the previous CC value
     * ... because we were holding C for a bit, then stopped (coasted),
     *     then resumed after only a short time
     *
     * side effects: stores a few states as well as the previousCruise level
     */
    bool checkCruiseReturn(Chuck chuck) {
      static bool previousC;
      static unsigned long previousCruiseMS;
      bool retval = false;

#ifdef DEBUGGING_THROTTLE
        Serial.print("checkCruiseReturn: ");
#endif 

      if (chuck.C && !previousC && 
          previousCruiseMS + THROTTLE_CRUISE_RETURN_MS > millis()) {
#ifdef DEBUGGING_THROTTLE
        Serial.print("!C -> C && still time");
#endif
        retval = true;
      }
      
      if (! chuck.C && previousC) {
#ifdef DEBUGGING_THROTTLE
        Serial.print("C -> !C");
#endif
        previousCruiseMS = millis();
        previousCruiseLevel = getThrottle();
        retval = false;
      }
      
      // any joystick input kills cruise return
      if (abs(chuck.X) > THROTTLE_MIN ||
          abs(chuck.Y) > THROTTLE_MIN) {
#ifdef DEBUGGING_THROTTLE
        Serial.print("stick");
#endif
        previousCruiseMS = 0;
        retval = false;
      }

#ifdef DEBUGGING_THROTTLE
      Serial.println();
#endif
      
      previousC = chuck.C;
      return retval;
    } // checkCruiseReturn(Chuck chuck)
    
}; // class Throttle

#endif
