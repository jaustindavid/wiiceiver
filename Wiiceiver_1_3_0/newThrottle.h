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

#ifndef NEWTHROTTLE_H
#define NEWTHROTTLE_H

// not strictly necessary, but a nice reminder
#include "Chuck.h"
#include "Smoover.h"
#include "Cruiser.h"


/*
  TUNABLES: these are the most immediate ways to
  tune wiiceiver for your setup & preferences

  THROTTLE and BRAKES each have their own settings
  and cruise control.
  
  *_RISE : rate (%throttle per 20ms) of maximum increase
     higher RISE == harder acceleration
  *_FALL : rate (% per 20ms) of reset to zero on idle
     lower FALL == snappier throttle response
  *_SMOOTH : exponential smoothing factor
     higher SMOOTH == softer throttle response
  *_MIN_BUMP : minimum change for any input
     probably don't need to change this
     
  #define THROTTLE_RISE     0.003  // roughly 15% increase per second
  #define THROTTLE_FALL     0.005  // rought 25% decrease per second at idle
  #define THROTTLE_SMOOTH   0.050  // 5% exponential smoothing
  #define THROTTLE_MIN_BUMP 0.003
  
  #define BRAKES_RISE       0.004
  #define BRAKES_FALL       0.005
  #define BRAKES_SMOOTH     0.050
  #define BRAKES_MIN_BUMP   0.003

  *_CC_BUMP : max amount change for cruise control input
     same calculation as throttle; usually softer
  *_CC_AUTO : default "auto cruise" level (slowest cruise setting)
     for brakes, this is the "drag brake" when you hit Z

  #define THROTTLE_CC_BUMP  0.002
  #define THROTTLE_CC_AUTO  0.000
  
  #define BRAKES_CC_BUMP    0.003
  #define BRAKES_CC_AUTO    0.050
*/


// only accurate to 3 digits, sorry
#define THROTTLE_RISE     0.002
#define THROTTLE_FALL     0.005
#define THROTTLE_SMOOTH   1.0 // 0.050
#define THROTTLE_MIN_BUMP 0.003

#define BRAKES_RISE       0.020
#define BRAKES_FALL       0.020
#define BRAKES_SMOOTH     1.0 // 0.100
#define BRAKES_MIN_BUMP   0.003

#define THROTTLE_CC_RISE  0.001
#define THROTTLE_CC_FALL  0.003
#define THROTTLE_CC_AUTO  0.000

#define BRAKES_CC_RISE    0.003
#define BRAKES_CC_FALL    0.003
#define BRAKES_CC_AUTO    0.050

/*
 * Manages the throttle input; presents a smoothed output, [ -1 .. 1 ]
 */

class newThrottle {
  private:
    float throttle;
    Smoover *upper, *downer;
    Cruiser *cruiser, *braker;

    
/*
 * PID doesn't really work here...
 *
 * Cases: target < 0: brakes
 *                 floor, pressure down
 *                 ceiling allowed to fall
 *        target > 0: throttle
 *                 ceiling, pressure up
 *                 floor allowed to rise
 *        target + Z: more aggro
 *                 floor / ceiling moves faster
 */
    float smoove(float target) {
      float smoothed = 0;
      
      #ifdef DEBUGGING_SMOOVER
      Serial.print("Throttle::smoove(");
      Serial.print(target);
      Serial.print(") -> ");
      #endif
      if (target > -THROTTLE_MIN) {
        smoothed = upper->smoove(target);
        downer->smoove(0);
      } else if (target < THROTTLE_MIN) {
        upper->smoove(0);
        smoothed = -downer->smoove(abs(target));
      }
      
      #ifdef DEBUGGING_SMOOVER
      Serial.println(smoothed);
      #endif
      return smoothed;
    } // private float smoove(float target)

    
  public:
    
    // constructor
    newThrottle() {
      //                   RISE,  FALL,  SMOOTH, MIN_BUMP
      upper =  new Smoover(THROTTLE_RISE, THROTTLE_FALL, 
                           THROTTLE_SMOOTH, THROTTLE_MIN_BUMP);
      // 0.004, 0.005, 0.05,   0.003), 
      downer = new Smoover(BRAKES_RISE, BRAKES_FALL,
                           BRAKES_SMOOTH, BRAKES_MIN_BUMP);
      // 0.05,  0.01,  0.05,   0.003);

      cruiser = new Cruiser(THROTTLE_CC_RISE, THROTTLE_CC_FALL,   
                            THROTTLE_CC_AUTO, EEPROM_AUTOCRUISE_ADDY);
      braker =  new Cruiser(BRAKES_CC_RISE, BRAKES_CC_FALL,
                            BRAKES_CC_AUTO, EEPROM_AUTOBRAKE_ADDY);
      throttle = 0;
    } // Throttle()
    

    void init() {
      cruiser->init();
      braker->init();
      zero();
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
      #define CHUCK_C 1
      #define CHUCK_Z 2
      #define CHUCK_NONE 0
      static byte lastChuckButton = CHUCK_NONE;
      
      #ifdef DEBUGGING_THROTTLE
      Serial.print("Throttle: ");
      Serial.print("y=");
      Serial.print(chuck.Y, 4);
      Serial.print(", ");
      Serial.print("c=");
      Serial.print(chuck.C);
      Serial.print(", z=");
      Serial.print(chuck.Z);
      Serial.print(", t=");
      Serial.print(throttle, 4);
      Serial.print("; ");
      #endif

      // set up ~sticky chuck buttons
      //   note that the stick is allowed to be in the range [-THROTTLE_MIN .. THROTTLE_MIN] 
      //   and still hit a button; buttons should work if it's in the dead zone, or in the 
      //   correct rage.  For C, range [-THROTTE_MIN .. 1]; Z: [-1 .. THROTTLE_MIN]
      if (lastChuckButton == CHUCK_NONE) {
        if (chuck.C && !chuck.Z && chuck.Y > -THROTTLE_MIN) {
          lastChuckButton = CHUCK_C;
        } else if (chuck.Z && !chuck.C && chuck.Y < THROTTLE_MIN) {
          lastChuckButton = CHUCK_Z;
        }
      } else if (!chuck.C && !chuck.Z) {
        lastChuckButton = CHUCK_NONE;
      }
      
      /*
      if (chuck.C && !chuck.Z && chuck.Y > -THROTTLE_MIN) {
        lastChuckButton = CHUCK_C;
      } else if (chuck.Z && !chuck.C && chuck.Y < THROTTLE_MIN) {
        lastChuckButton = CHUCK_Z;
      } else {
        lastChuckButton = CHUCK_NONE;
      }
      */
      
      
      #ifdef DEBUGGING_THROTTLE
      Serial.print(" button:");
      if (lastChuckButton) {
        Serial.print(lastChuckButton == CHUCK_C ? "C; " : "Z; ");
      } else {
        Serial.print("-; ");
      }
      #endif
      
      if (lastChuckButton == CHUCK_C) {
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" -C- ");
        #endif
        throttle = cruiser->update2(throttle, chuck.X, chuck.Y);
        upper->rough(throttle);
        downer->zero();
      } else if (lastChuckButton == CHUCK_Z) {
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" -Z- ");
        #endif
        throttle = -braker->update2(-throttle, chuck.X, -chuck.Y);
        downer->rough(abs(throttle));
        upper->zero();
      } else if (chuck.Y > THROTTLE_MIN) {  
        // gas
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" -^- ");
        #endif
        if (throttle < THROTTLE_MIN) { 
          // transition brakes -> gas
          throttle = THROTTLE_MIN;
          throttle = cruiser->getAutoCruise();
          upper->rough(throttle);
        } else {
          throttle = upper->smoove(chuck.Y);
        }
        downer->smoove(0);
        // stick input resets cruise control
        cruiser->zero();
        braker->zero();
      } else if (chuck.Y < -THROTTLE_MIN) {
        // brakes
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" -v- ");
        #endif
        if (throttle > -THROTTLE_MIN) {
          // transition gas -> brakes
          throttle = -THROTTLE_MIN;
          throttle = -braker->getAutoCruise();
          downer->rough(abs(throttle));
        } else {
          throttle = -downer->smoove(abs(chuck.Y));
        }
        upper->smoove(0);
        // stick input resets cruise control
        cruiser->zero();
        braker->zero();
      } else {
        #ifdef DEBUGGING_THROTTLE
        Serial.print(" -=- ");
        #endif

        // coasting
        cruiser->coast();
        braker->coast();
        throttle = smoove(0);
      }

      #ifdef DEBUGGING_THROTTLE
      Serial.print(F(" throttle: "));
      Serial.println(throttle);
      #endif

      return throttle;
    } // float update(void)
    
    
    float getThrottle(void) {
      return throttle;
    } // float getThrottle()
    
    
    void zero(void) {
      throttle = 0;
      upper->zero();
      downer->zero();
      cruiser->zero();
      braker->zero();
    } // void zero(void)

    
}; // class Throttle

#endif
