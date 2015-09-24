#ifndef CRUISER_H
#define CRUISER_H


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
      Serial.print("CR [");
      Serial.print(throttle);
      Serial.print("] ");
      Serial.print(crTimer + CRUISER_RETURN);
      Serial.print(" >? ");
      Serial.println(millis());
      #endif
      return (crTimer + CRUISER_RETURN) > millis();
    } // bool available()
    
    
    void update(void) {
      crTimer = millis();
      #ifdef DEBUGGING_CRUISER
      Serial.print("CR <- ");
      Serial.print(throttle);
      Serial.print(" @ ");
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
};


#define CR_NORMAL     0
#define CR_RETURNING  1
#define CR_ACCEL      2
#define CR_DECEL      3
#define CR_SET        4
#define CR_NEW        5

class Cruiser {
  private:
    float rise, fall, previous, autoCruise, desiredRate;
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
      rise = rise_;
      fall = fall_;
      autocruiseSetCounter = 0;
      state = CR_NORMAL;
      desiredRate = rise;
    } // Cruiser()
    

    void init() {
      readAutoCruise();
    } // init()    
    
    
    // "coast" at idle; decrement the cruise return
    void coast() {
      previous = 0;
      cr.coast(fall);
    } // coast
    
    
    void zero() {
      previous = 0;
      cr.zero();
    } // zero()



    float update2(float throttle, float stickX, float stickY) {
      // first, update the state
      if (abs(stickX) > 0.50) {
        state = CR_SET;
      } else if (stickY > 0.25) {
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
          Serial.print("Return, trying cr=");
          Serial.print(cr.getThrottle());
          Serial.print(" -> ");
          Serial.print("t=");
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
      } else if (state == CR_SET) {
        // hold, maybe set the autoCruise
        #ifdef DEBUGGING_CRUISER
        Serial.print("setting autocruise? asc=");
        Serial.println(autocruiseSetCounter);
        #endif
        if (++autocruiseSetCounter > 250) {
          writeAutoCruise(autoCruise = throttle);
          autocruiseSetCounter = 0;
        }        
        newThrottle = throttle;
      } else if (state == CR_NORMAL) {
        newThrottle = max(throttle, autoCruise);
        cr.update(newThrottle);
      }
      
      #ifdef DEBUGGING_CRUISER
      Serial.print(" => nt=");
      Serial.println(newThrottle);
      #endif 

      return previous = newThrottle;
    } // float update2(float throttle, float stickX, float stickY)
    

    float getAutoCruise(void) {
      return autoCruise;
    } // float getAutoCruise()


    void readAutoCruise(void) {
      byte storedValue = EEPROM.read(addy);
      #ifdef DEBUGGING_CRUISER
      Serial.print("Read autoCruise from EEPROM: ");
      Serial.print(storedValue);
      #endif

      if (storedValue > 0 && storedValue < 100) {
        autoCruise = 0.01 * storedValue;
        #ifdef DEBUGGING_CRUISER
        Serial.print("; setting autoCruise = ");
        Serial.println(autoCruise);
        #endif
      } else {
        #ifdef DEBUGGING_CRUISER
        Serial.print("; ignoring, leaving autoCruise = ");
        Serial.println(autoCruise);
        #endif
      }
    } // float readAutoCruise(void) 
    

    // sets the internal autoCruise var to the current throttle position,
    // and writes it to EEPROM    
    void writeAutoCruise(float value) {
      int storedValue = value * 100;
      EEPROM.write(addy, storedValue);
      #ifdef DEBUGGING_CRUISER
      Serial.print("Storing autoCruise as ");
      Serial.println(storedValue);
      #endif         
    } // void writeAutoCruise(void)

};



#endif
