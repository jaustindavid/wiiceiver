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

class Display {
  private:
    Chuck *chuck;
    Logger *logger;
    Throttle *throttle;
    Timer timer;
    byte updateCtr;
    
    #define DISP_STATUS       0
    #define DISP_CURRENT      1
    #define DISP_DISCHARGE    2
    #define DISP_REGEN        3
    #define DISP_HISTORY      4
    #define DISP_MESSAGE      5        // message doesn't get its own "screen"
    #define DISP_NUMSCREENS   5
    byte screen, prevScreen;


/********
 * PATTERNS!
 * http://www.codeproject.com/Articles/721796/Design-patterns-in-action-with-Arduino
 ********/
 
  // PRIVATE constructor
  Display(void) {
    chuck = Chuck::getInstance();
    logger = Logger::getInstance();
    throttle = Throttle::getInstance();
    updateCtr = 0;
    timer.reset(5000);
  } // constructor
  
  
  Display(Display const&);
  void operator=(Display const&);
  
  
  void clearMessages(void) {
  }
  
  void setMessage(byte line, const char* buffer, byte size) {
  }
  
  void appendMessage(const char* buffer) {
  }
  
  void appendMessage(const byte buffer) {
  }

  

  public:
  
    static Display* getInstance(void) {
      static Display myDisplay;
      return &myDisplay;
    }

    void attachChuck(Chuck* newChuck) {
      chuck = newChuck;
    }
    
    
    void init() {      
      splashScreen();
      screen = DISP_DISCHARGE;
    }


    void splashScreen(void) {
/*
      clearMessages();
      setMessage(0, "Wiiceiver!", 2);
      setMessage(1, WIICEIVER_VERSION, 1);
      setMessage(2, "Watchdog resets: ", 1);
      appendMessage((byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      */
      #ifdef DEBUGGING
      Serial.println("sending info screen");
      #endif
      snprintf(statusPacket.message.text[0], 18, "Wiiceiver!");
      snprintf(statusPacket.message.text[1], 18, "v %s", WIICEIVER_VERSION);
      snprintf(statusPacket.message.text[2], 18, "WDT Resets: %d", (byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      statusPacket.message.text[3][0] = '\0'; 
      xmit(statusPacket.bytes, sizeof(statusPacket.bytes));
      #ifdef DEBUGGING
      Serial.print("sent ");
      Serial.println(sizeof(statusPacket.bytes));
      #endif
    }    

    
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
      
      for (byte i = 0; i < 3; i ++) {
        statusPacket.message.history[i] = logger->getNthRec(i);
      }
      statusPacket.message.peakDischarge = logger->getPeakDischarge();
      statusPacket.message.peakRegen = logger->getPeakRegen();
      statusPacket.message.totalDischarge = logger->getDischarge();
      statusPacket.message.totalRegen = logger->getRegen();
      statusPacket.message.current = logger->getCurrent();

      unsigned long start = millis();
      // OPTIMIZATION: if the first text bit is 255, don't send the text block
      /*
      if (statusPacket.message.text[0][0] != (char)255) {
        xmit(statusPacket.bytes, sizeof(statusPacket.bytes));
        statusPacket.message.text[0][0] = (char)255;
      } else {
        */
        xmit(statusPacket.bytes, sizeof(statusPacket.bytes) - sizeof(statusPacket.message.text));
      // }
      Serial.print("Transmitted ");
      Serial.print(sizeof(statusPacket));
      Serial.print(" bytes in ");
      Serial.print(millis() - start);
      Serial.println("ms");
    } // update()
    
    
    
};

#endif
