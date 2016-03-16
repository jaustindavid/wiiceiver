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
     higher SMOOTH == more direct throttle response
     lower SMOOTH == softer throttle response
  *_MIN_BUMP : minimum change for any input
     probably don't need to change this
     
  #define THROTTLE_RISE     0.003  // roughly 15% increase per second
  #define THROTTLE_FALL     0.005  // rought 25% decrease per second at idle
  #define THROTTLE_SMOOTH   0.050  // 5% exponential smoothing (1.0 disables it)
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
#define THROTTLE_CC_AUTO  0.050

#define BRAKES_CC_RISE    0.003
#define BRAKES_CC_FALL    0.003
#define BRAKES_CC_AUTO    0.050

/*
 * Manages the throttle input; presents a smoothed output, [ -1 .. 1 ]
 */

class Throttle {
  private:
    float throttle, maxThrottle;
    Smoover *upper, *downer;
    Cruiser *cruiser, *braker;


    // http://forum.arduino.cc/index.php?topic=3922.0
    float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
      return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    } // float mapfloat(x, in_min, in_max, out_min, out_max)

    
  public:
    
    // constructor
    Throttle() {
      upper =  new Smoover(THROTTLE_RISE, THROTTLE_FALL, 
                           THROTTLE_SMOOTH, THROTTLE_MIN_BUMP);
      downer = new Smoover(BRAKES_RISE, BRAKES_FALL,
                           BRAKES_SMOOTH, BRAKES_MIN_BUMP);

      cruiser = new Cruiser(THROTTLE_CC_RISE, THROTTLE_CC_FALL,   
                            THROTTLE_CC_AUTO, EEPROM_AUTOCRUISE_ADDY);
      braker =  new Cruiser(BRAKES_CC_RISE, BRAKES_CC_FALL,
                            BRAKES_CC_AUTO, EEPROM_DRAGBRAKE_ADDY);
      throttle = 0;
    } // Throttle()
    

    void init() {
      upper->init();
      downer->init();
      cruiser->init();
      braker->init();
      maxThrottle = 0.01 * readSetting(EEPROM_MAXTHROTTLE_ADDY, 100);
      zero();
    } // init()

    
    /*
     * returns a smoothed (rate-limited) float [-1 .. 1]
     *
     * Theory of Operation: identify the throttle position (joystick angle), 
     *   then return a smoothed representation
     *
     *   if C is pressed, "cruise control":
     *      set "cruise" to last joystick position
     *      if joystick == up, increment throttle position
     *      if joystick == down, decrement throttle position
     *   else throttle position == chuck.Y joystick position
     *   return a smoothed value from the throttle position
     *   
     *   "Z" is the same as C but upside-down & brakes
     *   
     *   if HELI_MODE: treat Z like dead-man switch; Z on == auto-cruise (slowest throttle)
     *     to fake neutral.  Z off == lowest stick, which is either neutral or brake
     *     on the ESC
     */
    float update(Chuck chuck) {
      #define CHUCK_C 1
      #define CHUCK_Z 2
      #define CHUCK_BOTH 3
      #define CHUCK_NONE 0
      static byte lastChuckButton = CHUCK_NONE;
      
      #ifdef DEBUGGING_THROTTLE
        Serial.print(F("Throttle: "));
        Serial.print(F("y="));
        Serial.print(chuck.Y, 4);
        Serial.print(F(", "));
        Serial.print(F("c="));
        Serial.print(chuck.C);
        Serial.print(F(", z="));
        Serial.print(chuck.Z);
        Serial.print(F(", t="));
        Serial.print(throttle, 4);
        Serial.print(F("; "));
      #endif

      // a sort of button debouncing
      if (chuck.C && !chuck.Z) {
        lastChuckButton = CHUCK_C;
      } else if (chuck.Z && !chuck.C) {
        lastChuckButton = CHUCK_Z;
      } else if (!chuck.C && !chuck.Z) {
        lastChuckButton = CHUCK_NONE;
      } else if (chuck.Y <= cruiser->getAutoCruise() && chuck.Y >= braker->getAutoCruise()
                 && throttle <= cruiser->getAutoCruise() && throttle >= braker->getAutoCruise() 
                 && chuck.C && chuck.Z) {
        lastChuckButton = CHUCK_BOTH;
      }
      
      #ifdef DEBUGGING_THROTTLE_BUTTONS
        Serial.print(F(" button:"));
        if (lastChuckButton) {
          Serial.print(lastChuckButton == CHUCK_C ? F("C; ") : F("Z; "));
        } else {
          Serial.print(F("-; "));
        }
      #endif
      
      if (lastChuckButton == CHUCK_C) {
        #ifdef DEBUGGING_THROTTLE
          Serial.print(F(" -C- "));
        #endif
        throttle = cruiser->update(throttle, chuck.X, chuck.Y);
        upper->rough(throttle);
        downer->zero();
      } else if (lastChuckButton == CHUCK_Z) {
        #ifdef DEBUGGING_THROTTLE
          Serial.print(F(" -Z- "));
        #endif
        #ifdef ALLOW_HELI_MODE
          if (settings.HELI_MODE) {
            #ifdef DEBUGGING_THROTTLE
              Serial.print(F(" (heli) "));
            #endif
            // Z == dead-man switch: run at min throttle (or Y, whichever)
            // similar behavior to throttle stick at auto-cruise or higher
            // stick is normalized tho
  
            float newY = mapfloat(chuck.Y, THROTTLE_MIN, 1.0, cruiser->getAutoCruise(), 1.0);
            // if throttle > y, coasting; smooth
            // if throttle < MIN be rough
            if (chuck.Y < -THROTTLE_MIN) {
               #ifdef DEBUGGING_THROTTLE
                Serial.print(F(" [t<0; brakes] "));
              #endif
              throttle = 0;
              upper->rough(throttle);
            } else if (throttle < cruiser->getAutoCruise()) {
              #ifdef DEBUGGING_THROTTLE
                Serial.print(F(" [t<cruise; idle, rough] "));
              #endif
              throttle = cruiser->getAutoCruise();
              upper->rough(throttle);
            } else if (throttle > newY && newY <= cruiser->getAutoCruise()) {
              Serial.print(F(" [coasting; smoove down] "));
              throttle = upper->smoove(throttle - (throttle - cruiser->getAutoCruise())*0.1);
            } else {
              Serial.print(F(" [active throttle; smoove] "));
              throttle = upper->smoove(newY);
            }
            downer->smoove(0);
            // Z == stick input, which resets cruise control
            cruiser->zero();
            braker->zero();
          } else {
         #else
            // not heli; Z == drag brake
            throttle = -braker->update(-throttle, chuck.X, -chuck.Y);
            downer->rough(ABS(throttle));
            upper->zero();
        #endif
        #ifdef ALLOW_HELI_MODE
          }
        #endif
      } else if (lastChuckButton == CHUCK_BOTH) {
        #ifdef DEBUGGING_THROTTLE
          Serial.print(F(" -!!- "));
        #endif
        throttle = chuck.Y;
        upper->rough(throttle);
        downer->rough(throttle);
        cruiser->zero();
        braker->zero();
      } else if (chuck.Y > THROTTLE_MIN) {  
        // gas
        #ifdef DEBUGGING_THROTTLE
          Serial.print(F(" -^- "));
        #endif
        if (throttle < THROTTLE_MIN) { 
          // transition brakes -> gas
          throttle = max(THROTTLE_MIN, cruiser->getAutoCruise());
          upper->rough(throttle);
        } else {
          float newY = mapfloat(chuck.Y, THROTTLE_MIN, 1.0, cruiser->getAutoCruise(), 1.0);
          throttle = upper->smoove(newY);
        }
        downer->smoove(0);
        // stick input resets cruise control
        cruiser->zero();
        braker->zero();
      } else if (chuck.Y < -THROTTLE_MIN) {
        // brakes
        #ifdef DEBUGGING_THROTTLE
        Serial.print(F(" -v- "));
        #endif
        if (throttle > -THROTTLE_MIN) {
          // transition gas -> brakes
          throttle = min(-THROTTLE_MIN, -braker->getAutoCruise());
          downer->rough(ABS(throttle));
        } else {
          float newY = mapfloat(chuck.Y, -THROTTLE_MIN, -1.0, -braker->getAutoCruise(), -1.0);
          #ifdef DEBUGGING_THROTTLE
            Serial.print(F(" newY:"));
            Serial.print(newY);
            Serial.print(F(" "));
          #endif
          // throttle = -downer->smoove(ABS(chuck.Y));
          throttle = -downer->smoove(ABS(newY));
        }
        upper->smoove(0);
        // stick input resets cruise control
        cruiser->zero();
        braker->zero();
      } else {
        // coasting
        #ifdef DEBUGGING_THROTTLE
          Serial.print(F(" -=- "));
        #endif

        cruiser->coast();
        braker->coast();
        // throttle = upper->smoove(0);
        // drop throttle gently
        throttle = upper->smoove(throttle*0.75);
        // dump brakes immediately
        downer->smoove(0);
      }

      #ifdef DEBUGGING_THROTTLE
        Serial.print(F(" throttle: "));
        Serial.println(throttle);
      #endif

      throttle = min(throttle, maxThrottle);

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
