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
  private:
    float autoCruise, throttle, previousCruiseLevel;
    int xCounter;
    Smoother smoother;
    
    
    void readAutoCruise(void) {
      byte storedValue = EEPROM.read(EEPROM_AUTOCRUISE_ADDY);
      Serial.print("Read autoCruise from EEPROM: ");
      Serial.print(storedValue);

      if (storedValue > 0 && storedValue < 100) {
        autoCruise = 0.01 * storedValue;
        Serial.print("; setting autoCruise = ");
        Serial.println(autoCruise);
      } else {
        Serial.print("; ignoring, leaving autoCruise = ");
        Serial.println(autoCruise);
      }
    } // float readAutoCruise(void) 
    

    // sets the internal autoCruise var to the current throttle position,
    // and writes it to EEPROM    
    void writeAutoCruise(void) {
      autoCruise = throttle;
      int storedValue = autoCruise * 100;
      EEPROM.write(EEPROM_AUTOCRUISE_ADDY, storedValue);
      #ifdef DEBUGGING_THROTTLE
      Serial.print("Storing autoCruise as ");
      Serial.println(storedValue);
      #endif         
    } // void writeAutoCruise(void)


    /*
     * returns 'true' if we're in a "set autocruise level" state:
     *   while in cruise (chuck.C), with no throttle input (chuck.Y =~ 0), 
     *   holding Z, full X deflection (chuck.X > 0.75) ... for N cycles,
     *   within 2 minutes of startup ...
     * set "autoCruise" to the current throttle level.
     *   
     */
    bool checkAutoCruise(Chuck chuck) {
      if (! chuck.C) {
        #ifdef DEBUGGING_THROTTLE_CAC
        Serial.println("checkAutoCruise: no C");
        #endif
        xCounter = 0;
        return false;
      }
      if (millis() < 120 * 1000 &&
          chuck.Z &&
          abs(chuck.Y) < 0.25 && 
          abs(chuck.X) > 0.75) {
        ++xCounter;
        #ifdef DEBUGGING_THROTTLE_CAC
        Serial.print("checkAutoCruise: xCounter = ");
        Serial.println(xCounter);
        #endif
        if (xCounter == 150) { // ~3s holding X
          writeAutoCruise();  
        }
        return true;
      } else {
        xCounter = 0;
        #ifdef DEBUGGING_THROTTLE_CAC
        Serial.println("checkAutoCruise: no X or Y");
        Serial.print("x = ");
        Serial.print(abs(chuck.X));
        Serial.print(", y = ");
        Serial.println(abs(chuck.Y));
        #endif        
        return false;
      }
    } // bool checkAutoCruise(void)
    


    // #define DEBUGGING_THROTTLE_CCR
    /*
     * > 0 (a throttle level) if we should return to the previous CC value
     * ... because we were holding C for a bit, then stopped (coasted),
     *     then resumed after only a short time
     *
     * returning 0.0 is equivalent to "ignore me and use throttle", which *can*
     * mean holding an existing cruise level.  This can happen while cruise is 
     * resuming (acceleration to a previous cruise level, while holding C).
     *
     * side effects: stores a few states as well as the previousCruise level
     */
    float checkCruiseReturn(Chuck chuck) {
      static bool previousC;
      static unsigned long previousCruiseMS;
      float newThrottle = 0.0;
      static int ccrState = 0;
      
      #define CCR_NULL 0             // default
      #define CCR_RESUMING 1         // !C -> C, resuming
      #define CCR_WAITING 2          // C -> !C, no input -- resume is possible

      #ifdef DEBUGGING_THROTTLE_CCR
      Serial.print("checkCruiseReturn (");
      Serial.print(ccrState);
      Serial.print("): ");
      #endif 

      if (ccrState == CCR_WAITING &&
          (previousCruiseMS + THROTTLE_CRUISE_RETURN_MS < millis())) {
        // time's up -- cancel cruise
        ccrState = CCR_NULL; 
        previousCruiseLevel = 0;
      }

      if (chuck.C && !previousC && ccrState == CCR_WAITING) {
        #ifdef DEBUGGING_THROTTLE_CCR
        Serial.print("!C -> C && still time");
        #endif
        ccrState = CCR_RESUMING;
      }
      
      if (! chuck.C && previousC) {
        previousCruiseLevel = getThrottle();
        previousCruiseMS = millis();
        newThrottle = 0.0;
        #ifdef DEBUGGING_THROTTLE_CCR
        Serial.print("C -> !C");
        Serial.print("saving prev: ");
        Serial.print(previousCruiseLevel);
        #endif
        ccrState = CCR_WAITING;
      }
      
      // any joystick input kills cruise resume
      if (abs(chuck.X) > THROTTLE_MIN ||
          abs(chuck.Y) > THROTTLE_MIN) {
        #ifdef DEBUGGING_THROTTLE_CCR
        Serial.print("stick");
        #endif
        previousCruiseMS = previousCruiseLevel = 0;
        newThrottle = 0.0;
        ccrState = CCR_NULL;
      }
      
      if (ccrState == CCR_RESUMING) {
        newThrottle = smoother.smooth(previousCruiseLevel, SMOOTHER_CRUISE_RESUME_PROGRAM);
      } // CCR_RESUMING

      #ifdef DEBUGGING_THROTTLE_CCR
      Serial.print(" (->");
      Serial.print(ccrState);
      Serial.println(")");
      #endif
      
      previousC = chuck.C;
      return newThrottle;
    } // checkCruiseReturn(Chuck chuck)

    
    // returns the throttle position appropriate for cruise
    // This is called if C is down
    // Theory of Operation:
    //   checkAutoCruise: if looking to setting, don't change throttle
    //   !C -> C: 
    float cruiseControl(Chuck chuck) {
      if (checkAutoCruise(chuck)) {                                  // setting auto cruise?
        // we're looking for autoCruise, so do that;
        // don't change the throttle position, just
        // return throttle; fallthrough is OK
      } else if (chuck.Y > 0.25) {                                   // accel?
        // speed up, but not past 1.0 (full blast)
        throttle += chuck.Y * THROTTLE_CC_BUMP;
        throttle = min(throttle, 1.0);
      } else if (chuck.Y < -0.25) {                                  // decel?
        throttle += chuck.Y * THROTTLE_CC_BUMP;
        throttle = max(throttle, 0.0);
      } else if (throttle < autoCruise) {                            // auto cruise?
        throttle += 4 * THROTTLE_CC_BUMP;
      } 
      return throttle;
    } // float cruiseControl(void)
    
    
  public:
    
    // constructor
    Throttle() {
      smoother = Smoother();
      throttle = 0;
      autoCruise = THROTTLE_MIN_CC;
      xCounter = 0;
    } // Throttle()
    

    void init(void) {
      readAutoCruise();
    } // init()

    
    
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

      if (float newThrottle = checkCruiseReturn(chuck)) {
        // CC return: in CC mode, drop C, then resume shortly after 
        // (with no other input) -- resume the previous CC 
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" resuming cruise: ");
        Serial.print(newThrottle);
        Serial.print(") ");
        #endif         
        throttle = newThrottle;
      } else if (chuck.C) { 
        // cruise control!
        throttle = cruiseControl(chuck);
        // don't actually use a smoothed throttle, but keep the smoother algorithm warm
        // so when we drop the cruise, throttle (with stick @ 0) will "smooth" back to neutral
        smoother.smooth(throttle, SMOOTHER_THROTTLE_PROGRAM);
      } else if (chuck.Y < -THROTTLE_MIN) { 
        // brakes!
        throttle = smoother.smooth(chuck.Y, SMOOTHER_BRAKES_PROGRAM);
      } else {
        throttle = smoother.smooth(chuck.Y, (chuck.Z ? SMOOTHER_THROTTLE_Z_PROGRAM : SMOOTHER_THROTTLE_PROGRAM));
      }
      
      #ifdef DEBUGGING_THROTTLE
      Serial.print(F("throttle: "));
      Serial.println(throttle);
      #endif

      return throttle;
    } // float update(void)
    
    
    float getThrottle(void) {
      return throttle;
    } // float getThrottle()
    
    
    void zero(void) {
      throttle = 0;
      smoother.zero();
    } // void zero(void)

    
}; // class Throttle

#endif
