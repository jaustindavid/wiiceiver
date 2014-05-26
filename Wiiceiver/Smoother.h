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

#ifndef SMOOTHER_H
#define SMOOTHER_H

/*
 * A helper class -- smooths the throttle input
 * 
 */

class Smoother {
  private:
    float value;
    
  public:
    Smoother(void) {
      value = 0;
    }


    // compute a new smoothed value
    float compute(float target, float factor) {
      float step = (target - value) * factor;
      
#ifdef DEBUGGING_SMOOTHER
      Serial.print("Target: ");
      Serial.print(target, 4);
      Serial.print(", Factor: ");
      Serial.print(factor, 4);
      Serial.print(", Value: ");
      Serial.print(value, 4);      
      Serial.print(", Step: ");
      Serial.print(step, 4);
#endif

      if (abs(step) < SMOOTHER_MIN_STEP) {
#ifdef DEBUGGING_SMOOTHER
      Serial.print(" BUMP");
#endif
        value = target;
      } else {
        value += step;
      }
      // value = (float)round(value * 10000) / 10000.0;
#ifdef DEBUGGING_SMOOTHER
      Serial.print(", result ");
      Serial.println(value, 4);
#endif
      return value;
    }  // float compute(float target, float factor)


    // reset the internal smoothing value, to quickly seek zero
    void zero() {
      value = 0;
    } // void zero()
};

#endif
