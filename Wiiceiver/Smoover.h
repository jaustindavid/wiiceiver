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

#ifndef SMOOVER_H
#define SMOOVER_H

/*
 * A class to "smoove" the throttle response.  This is positive only; 
 * the same class (different instances) is used to smooth throttle or
 * brake response.
 */
 
  class Smoover {
    private:
      float rise, default_rise, fall, default_fall, exp_factor, min_step;
      float last, ceiling;
      
      
    public:
    
      // constructor
      Smoover(float rise_, float fall_, float exp_factor_, float min_step_) {
        default_rise = rise_; 
        default_fall = fall_; 
        exp_factor = exp_factor_;
        min_step = min_step_;
        zero();
      } // Smoover(rise, fall, exp_factor, min_step)
      
      
      // zeroes internal vars (e.g. ceiling)
      void zero() {
        last = ceiling = 0;
      } // zero()
      
      
      // initialization: read vars from EEPROM
      void init(void) {
        float multiplier = getProfileMultiplier();
        rise = default_rise * multiplier;
        fall = default_fall * multiplier;
        #ifdef DEBUGGING
          Serial.print(F("Smoother::init(): profileMultiplier="));
          Serial.print(multiplier);
          Serial.print(F(", rise="));
          Serial.print(rise, 4);
          Serial.print(F(", fall="));
          Serial.println(fall, 4);
        #endif
      } // init()
      
       
      /*
      
      // return the smooved value for target
      *
      * target is (generally) a stick position, [0..1]
      * the returned value is a "smoothed" throttle, [0..1]
      * 
      * GENERALLY:
      *   if the new value is under something we've seen recently
      *   (the ceiling) move it quickly -- responsive throttle
      *   because we're not accelerating hard.
      *
      *   if the value is above the ceiling (hard acceleration)
      *   limit the "rate" of change (acceleration limit "rise")
      *
      *   The ceiling "falls" at a fixed rate; ideally this would
      *   be the rate at which the board slows down naturally
      */
      float smoove(float target) {
        float goal;
        
        #ifdef DEBUGGING_SMOOVER
          if (ceiling > 0) {
            Serial.print(F("Smoover: c="));
            Serial.print(ceiling, 4);
            Serial.print(F(", t="));
            Serial.print(target, 4);
          }
        #endif
        if (target > (ceiling + rise)) {
          // increase ceiling as fast as "rise"; goal is pinned here
          // scale by target: smaller stick motion = smaller rate
          ceiling += rise;  
          ceiling = min(ceiling, 1.0);
          goal = ceiling;
          #ifdef DEBUGGING_SMOOVER
            if (ceiling > 0) {
              Serial.print(F(" ^^ "));
            }
          #endif
        } else if (target < (ceiling - fall)) {
          // decrease ceiling as fast as "fall"
          // target is passed through
          ceiling -= fall;
          ceiling = max(ceiling, 0.0);
          goal = target;
          #ifdef DEBUGGING_SMOOVER
            if (ceiling > 0) {
              Serial.print(F(" vv "));
            }
          #endif
        } else {
          goal = target;
          #ifdef DEBUGGING_SMOOVER
            if (ceiling > 0) {
              Serial.print(F(" == "));
            }
          #endif
        }

        #ifdef DEBUGGING_SMOOVER
          if (ceiling > 0) {
            Serial.print(F(" => c="));
            Serial.print(ceiling, 4);
            Serial.print(F(", g="));
            Serial.println(goal, 4);
          }
        #endif
        
        return goal;
      } // float smoove(target)


      // opposite of smoove
      void rough(float goal) {        
        #ifdef DEBUGGING_SMOOVER
          if (ceiling > 0) {
            Serial.print(F("Smoover::rough("));
            Serial.print(goal, 4);
            Serial.print(F(") => c="));
            Serial.print(ceiling, 4);
          }
        #endif
        
        last = ceiling = goal;
          
        #ifdef DEBUGGING_SMOOVER
          if (ceiling > 0) {
            Serial.print(F("; now c="));
            Serial.println(ceiling, 4);
          }
        #endif
      } // float rough(goal)

      
      // "primes" this smoother, if needed;
      // similar to a conditional rough(goal)
      float prime(float goal) {
        if (ceiling < goal) {
          rough(goal);
        }
      } // float prime(goal)

  }; // class Smoove
  
#endif
