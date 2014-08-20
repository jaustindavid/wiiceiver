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


#ifndef UTILS_H
#define UTILS_H

// #define DEBUGGING_TIMER

class Timer {
  private:
    unsigned long startMS;
    int stopMS;

  public:
    Timer(int newStopMS) {
      reset(newStopMS);
    } // Constructor
    
    
   void reset(void) {
     startMS = millis();
   } // reset()
   
   
   void reset(int newStopMS) {
      stopMS = newStopMS;
      reset();
   }
   
   bool isExpired(void) {
     #ifdef DEBUGGING_TIMER
     Serial.print(F("Expired? "));
     Serial.print(startMS);
     Serial.print(F("+"));
     Serial.print(stopMS);
     Serial.print(F(" <> now@"));
     Serial.println(millis());
     #endif
     return millis() >= startMS + stopMS;
   } // bool isExpired()
   
};

#endif
