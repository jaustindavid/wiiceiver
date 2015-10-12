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
 
#ifndef CRUISER_H
#define CRUISER_H

/* 
 * CruiseReturn: a helper class to track the level and timing 
 *    to resume Cruise Control 
 *
 * Cruiser: the actual Cruise Control management class
 * 
 * Throttle response is positive only: the same cruiser class
 * is used for throttle + braking cruise control
 *
 */
#define CRUISER_RETURN 5000 //ms

class CruiseReturn {
  private:
    float throttle;
    unsigned long crTimer;
    
  public:
  
    CruiseReturn() {
      zero();
    } // CruiseReturn()
    
    
    void zero() {
      throttle = 0;
      crTimer = 0;
    } // zero()
    
    
    void coast(float decr) {
      throttle -= decr;
    } // coast(decr)
    
    
    bool available() {
      #ifdef DEBUGGING_CRUISER
      Serial.print(F("CR ["));
      Serial.print(throttle);
      Serial.print(F("] "));
      Serial.print(crTimer + CRUISER_RETURN);
      Serial.print(F(" >? "));
      Serial.println(millis());
      #endif
      return (crTimer + CRUISER_RETURN) > millis();
    } // bool available()
    
    
    void update(void) {
      crTimer = millis();
      #ifdef DEBUGGING_CRUISER
      Serial.print(F("CR <- "));
      Serial.print(throttle);
      Serial.print(F(" @ "));
      Serial.println(crTimer);
      #endif
    }
    
    
    void update(float throttle_) {
      throttle = throttle_;
      update();
    } // update(throttle)
    
    
    float getThrottle() {
      return throttle;
    }
}; // class CruiseReturn


#define CR_NORMAL     0
#define CR_RETURNING  1
#define CR_ACCEL      2
#define CR_DECEL      3
#define CR_SET        4
#define CR_NEW        5

class Cruiser {
  private:
    float rise, default_rise, fall, default_fall, previous, autoCruise, desiredRate;
    int addy;
    int autocruiseSetCounter;
    unsigned long cruiseReturnTimer;
    CruiseReturn cr;
    int state;
    
  public:
    // constructor
    Cruiser(float rise_, float fall_, float autoCruise_, int addy_) {
      zero();      
      autoCruise = autoCruise_;
      addy = addy_;
      default_rise = rise_;
      default_fall = fall_;
      autocruiseSetCounter = 0;
      state = CR_NORMAL;
      desiredRate = rise;
    } // Cruiser()
    

    void init() {
      // readAutoCruise();
      autoCruise = 0.01 * readSetting(addy, autoCruise*100);
      #ifdef DEBUGGING
        Serial.print(F("Cruiser::init(): autoCruise="));
        Serial.println(autoCruise);
      #endif

      float multiplier = getProfileMultiplier();
      multiplier = constrain(multiplier, 0.5, 3);
      rise = default_rise * multiplier;
      fall = default_fall * multiplier;
      #ifdef DEBUGGING
        Serial.print(F("Cruiser::init(): profileMultiplier="));
        Serial.print(multiplier);
        Serial.print(F(", rise="));
        Serial.print(rise, 4);
        Serial.print(F(", fall="));
        Serial.println(fall, 4);
      #endif

    } // init()    
    
    
    // "coast" at idle; decrement the cruise return
    void coast() {
      previous = 0;
      cr.coast(fall/2);
    } // coast
    
    
    void zero() {
      previous = 0;
      cr.zero();
    } // zero()



    float update(float throttle, float stickX, float stickY) {
      // first, update the state
      if (stickY > 0.25) {
        state = CR_ACCEL;
      } else if (stickY < -0.25) {
        state = CR_DECEL;
      } else if (previous == 0) {
        if (cr.available() && cr.getThrottle() > autoCruise) {
          state = CR_RETURNING;
        } else {
          state = CR_NORMAL;
        }
      } else if (state != CR_RETURNING) {
        state = CR_NORMAL;
      }
      
      
      #ifdef DEBUGGING_CRUISER
      Serial.print("Cruiser state: ");
      Serial.print(state);
      Serial.print(", t=");
      Serial.print(throttle);
      Serial.print(", p=");
      Serial.print(previous);
      #endif 
      
      
      // second, act on the state
      float newThrottle;
      if (state == CR_RETURNING) {
        if (throttle >= cr.getThrottle()) {
          // cruise return is caught up; be normal
          state = CR_NORMAL;
          newThrottle = throttle;
          #ifdef DEBUGGING_CRUISER
          Serial.print("done w/ return");
          #endif
        } else {
          // advance throttle aggressively
          if (previous < THROTTLE_MIN) {
            previous = throttle;
          }
          newThrottle = previous + rise * 4;
          newThrottle = min(newThrottle, 1.0);
          cr.update();  // update the time, but not the level
          #ifdef DEBUGGING_CRUISER
            Serial.print(F("Return, trying cr="));
            Serial.print(cr.getThrottle());
            Serial.print(F(" -> "));
            Serial.print(F("t="));
            Serial.print(throttle);
          #endif
        }
      } else if (state == CR_ACCEL) {
        // advance throttle a little
        newThrottle = max(previous, throttle) + stickY * rise;
        newThrottle = min(newThrottle, 1.0);
        cr.update(newThrottle);
      } else if (state == CR_DECEL) {
        // drop throttle a little (sticky < 0)
        newThrottle = min(previous, throttle) + stickY * fall;
        newThrottle = max(newThrottle, 0.0);
        cr.update(newThrottle);
      } else if (state == CR_NORMAL) {
        newThrottle = max(throttle, autoCruise);
        cr.update(newThrottle);
      }
      
      #ifdef DEBUGGING_CRUISER
        Serial.print(F(" => nt="));
        Serial.println(newThrottle);
      #endif 
      
      return previous = newThrottle;
    } // float update(float throttle, float stickX, float stickY)
    

    float getAutoCruise(void) {
      return autoCruise;
    } // float getAutoCruise()

}; // class Cruiser



#endif
