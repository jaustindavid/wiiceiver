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
 * latest software & schematic: 
 *    https://github.com/jaustindavid/wiiceiver
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

// #define DEBUGGING_SMOOTHER
#define SMOOTHER_MIN_STEP 0.003           // minimum change for smoothing; 0.003 for ~1s, 0.001 for ~2s
#define SMOOTHER_BRAKES_PROGRAM 0
#define SMOOTHER_THROTTLE_PROGRAM 1
#define SMOOTHER_THROTTLE_Z_PROGRAM 2
#define SMOOTHER_CRUISE_RESUME_PROGRAM 3

class Smoother {
  private:
    float value;
    float ceiling; 
    
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


  public:
    Smoother(void) {
      value = 0;
      ceiling = 0;
    }


    // the main "smooth" function -- actually computes a moving ceiling, and 
    // reacts more aggressively under that ceiling.  Net result is (meant to 
    // be) a responsive throttle *eith* a reasonable acceleration limit.
    float smooth(float target, int program) {
      #define INCR 0
      #define DECR 1
      #define SMOOTHNESS 2
      float programMap[5][3] = {
        // INCR,  DECR,  SMOOTHNESS
          {0.000, 0.010, 1.00}, // BRAKES
          {0.004, 0.005, 0.10}, // THROTTLE
          {1.000, 1.000, 1.00}, // THROTTLE + Z
          {0.004, 0.005, 0.10}, // "resume" cruise
      };
      float returnValue = target;

#ifdef DEBUGGING_SMOOTHER
      Serial.print("Adap2ive: program #");
      Serial.print(program);
      Serial.print("; target = ");
      Serial.print(target, 4);
      Serial.print(", ceiling: ");
      Serial.print(ceiling, 4);
#endif

      if (target > ceiling) {
        ceiling += programMap[program][INCR];
        if (ceiling > target) { // avoid hunting
          ceiling = target;
          ceiling = min(ceiling, 1.0);
        }
        returnValue = ceiling;
      } else if (target < ceiling) {
        ceiling -= programMap[program][DECR];
        if (ceiling < target) { // avoid hunting
          ceiling = target;
        }
        ceiling = max(ceiling, 0.0);
        returnValue = target; 
      }
       
      ceiling = max(ceiling, 0.2);

#ifdef DEBUGGING_SMOOTHER
      Serial.print(", next: ");
      Serial.print(returnValue);
      Serial.print(", new ceiling: ");
      Serial.print(ceiling, 4);
#endif

      returnValue = compute(returnValue, programMap[program][SMOOTHNESS]);

#ifdef DEBUGGING_SMOOTHER
      Serial.print(", returning: ");
      Serial.println(returnValue, 4);
#endif

      return returnValue;
    } // float smooth(float target, int program)


    // reset the internal smoothing value, to quickly seek zero
    void zero() {
      value = 0;
      ceiling = 0;
    } // void zero()
};

#endif
