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
    float throttle, smoothed;
    Smoother smoother;
    
  public:    
    
    Throttle() {
      smoother = Smoother();
      throttle = 0;
    } // Throttle()
    
    
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

      if (chuck.C) { // cruise control!
#ifdef DEBUGGING_THROTTLE
        Serial.print("CC: last = ");
        Serial.print(throttle, 4);
        Serial.print(", ");
#endif
        if (throttle < THROTTLE_MIN_CC) {
          throttle += THROTTLE_CC_BUMP * 1.5;
        } else {
          if (chuck.Y > 0.5 && throttle < 1.0) {
            throttle += THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
          } else if (chuck.Y < -0.5 && throttle > -1.0) {
            throttle -= THROTTLE_CC_BUMP * (chuck.Z ? 2 : 1);
          } // if (chuck.Y > 0.5 && throttle < 1.0) - else
        } // if (throttle < THROTTLE_MIN_CC) - else
      } else {
        throttle = chuck.Y;

        // "center" the joystick by enforcing some very small minimum
        if (abs(throttle) < THROTTLE_MIN) {
          throttle = 0;
        }
      } // if (chuck.C) -- else

      smoothed = smoother.compute(throttle, THROTTLE_SMOOTHNESS * (chuck.Z ? 4 : 1));

#ifdef DEBUGGING_THROTTLE
      Serial.print(F("position: "));
      Serial.print(throttle, 4);
      Serial.print(F(" smoothed: "));
      Serial.println(smoothed, 4);
#endif

      return smoothed;
    } // float update(void)
    
    
    float getThrottle(void) {
      return smoothed;
    } // float getThrottle()
    
    
    void zero(void) {
      throttle = 0;
      smoother.zero();
    } // void zero(void)
    
}; // class Throttle

#endif
