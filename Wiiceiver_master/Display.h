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
 
#ifndef DISPLAY_H
#define DISPLAY_H

#include "wiiceiver_i2c.h"
#include "Chuck.h"
#include "Logger.h"
#include "Throttle.h"
#include "Utils.h"

// #define DEBUGGING_DISPLAY

class Display {
  private:
    Chuck *chuck;
    Logger *logger;
    Throttle *throttle;
    Timer timer;
    byte updateCtr;


/********
 * PATTERNS!
 * http://www.codeproject.com/Articles/721796/Design-patterns-in-action-with-Arduino
 ********/
 
  // PRIVATE constructor
  Display(void) {
    chuck = Chuck::getInstance();
    logger = Logger::getInstance();
    throttle = Throttle::getInstance();

    timer.reset(5000);
  } // constructor
  
  
  Display(Display const&);
  void operator=(Display const&);
  

  public:
  
    // returns the Singleton instance of this class
    static Display* getInstance(void) {
      static Display myDisplay;
      return &myDisplay;
    } // Display* getInstance()
    
    
    // initialize the class
    void init() {
      //Wire.begin();
      splashScreen();
    } // init()


    // sends a "splash screen" to the display
    void splashScreen(void) {
      // #ifdef DEBUGGING
      Serial.println(F("sending info screen"));
      // #endif
      snprintf(statusPacket.message.text[0], 18, "Wiiceiver!");
      snprintf(statusPacket.message.text[1], 18, "v %s", WIICEIVER_VERSION);
      snprintf(statusPacket.message.text[2], 18, "WDT Resets: %d", (byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      statusPacket.message.text[3][0] = '\0'; 
      xmit(statusPacket.bytes, sizeof(statusPacket.bytes));

      #ifdef DEBUGGING
      Serial.print("sent ");
      Serial.println(sizeof(statusPacket.bytes));
      #endif
    } // splashScreen()

    
    void printMessage(const char* line1, const char* line2, const char* line3, const char* line4) {
      strncpy(statusPacket.message.text[0], line1, 18);
      strncpy(statusPacket.message.text[1], line2, 18);
      strncpy(statusPacket.message.text[2], line3, 18);
      strncpy(statusPacket.message.text[3], line4, 18);
      xmit(statusPacket.bytes, sizeof(statusPacket.bytes));
    }


    void update(void) {
      // TODO: expose structs in throttle, chuck, and logger
      
      statusPacket.message.throttle = throttle->getThrottle();
      
      statusPacket.message.chuckC = chuck->C;
      statusPacket.message.chuckY = chuck->Y;
      statusPacket.message.chuckZ = chuck->Z;
      
      statusPacket.message.uptime = millis();
      
      // our compute budget has a lot of overhead; if we needed a few 
      // cycles we could move this to init, the contents never change.
      for (byte i = 0; i < 3; i ++) {
        statusPacket.message.history[i] = logger->getNthRec(i);
      }
      statusPacket.message.peakDischarge = logger->getPeakDischarge();
      statusPacket.message.peakRegen = logger->getPeakRegen();
      statusPacket.message.totalDischarge = logger->getDischarge();
      statusPacket.message.totalRegen = logger->getRegen();
      statusPacket.message.current = logger->getAvgCurrent(10);

      unsigned long start = millis();
      // OPTIMIZATION: don't send the text block (usually)
      // the display will flash the text screen if we send it, so 
      // usually we don't. printmsg(buffer, buffer, buffer, buffer) will do it
      xmit(statusPacket.bytes, sizeof(statusPacket.bytes) - 
                               sizeof(statusPacket.message.text));

      #ifdef DEBUGGING_DISPLAY
      Serial.print("Transmitted ");
      Serial.print(sizeof(statusPacket));
      Serial.print(" bytes in ");
      Serial.print(millis() - start);
      Serial.println("ms");
      #endif
    } // update()
    
    
    
};

#endif
