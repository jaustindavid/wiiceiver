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

#include "utils.h"

    Timer displayTimer(5000);
    
    #define DISP_STATUS       0
    #define DISP_CURRENT      1
    #define DISP_DISCHARGE    2
    #define DISP_REGEN        3
    #define DISP_HISTORY      4
    #define DISP_MESSAGE      5       
    #define DISP_NUMSCREENS   6
    #define NO_SCREEN         99   // not a screen
    byte screen, prevScreen, nextScreen = NO_SCREEN;
    
    StatusPacket_t statusPacket;
    
    unsigned long lastUpdate;

  
  void justify(byte width, float value, byte decimals) {
    byte i;
    for (i = 0; i < decimals; i++) {
      value *= 10;
    }
    value = (int)value;
    for (i = 0; i < decimals; i++) {
      value /= 10;
    }

    // total = log10(value) + 1 + decimals
    byte spaces = 1;
    float working = value;
    if (working < 0) {
      working = working * -1;
      spaces += 1;
    }
    while (working >= 10) {
      ++spaces;
      working = working / 10;
    }

    if (decimals) {
      spaces += 1 + decimals;
    }

    
    if (spaces > width) {
      decimals = max(0, decimals - (spaces - width));

      if (decimals == 0) {
        spaces -= 3;
      }
    }
    
    for (; spaces < width; spaces ++) {
      display.print(F(" "));        
    }
    display.print(value, decimals);
  } // justify(width, value, decimals)


    void splashScreen(void) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print(F("Wiiceiver!"));
      display.setTextSize(1);
      display.print(F("head v"));
      display.println(F(WIICEIVER_HEAD_VERSION));
      display.print(F("Watchdog resets: "));
      display.println((byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      display.display();
    }    



    
    void printCurrent(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: CURRENT"));
        displayTimer.reset();
      }
      
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.println(F("Current"));
      
      display.setTextSize(1);
      display.print(F("   "));
      display.print(statusPacket.message.peakRegen, 1);
      display.print(F("A, "));
      display.print(statusPacket.message.peakDischarge, 1);
      display.print(F("A"));
      
      display.setTextSize(4);
      display.setCursor(0, 32);
      justify(5, statusPacket.message.current, 2);

      display.display();
    }
    
    
    void printDischarge(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: DISCHARGE"));
        displayTimer.reset();
      }
   
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.println(F("Discharge"));
      display.setTextSize(1);
      display.print(F("Peak draw: "));
      display.print(statusPacket.message.peakDischarge, 1);
      display.println(F("A"));
      
      display.setTextSize(4);
      display.setCursor(0, 32);
      justify(5, statusPacket.message.totalDischarge - statusPacket.message.totalRegen, 1);

      display.display();
    } // printDischarge()


    

    void printRegen(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: REGEN"));
        displayTimer.reset();
      }
   
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.println(F("Regen"));

      display.setTextSize(1);
      display.print(F("Peak regen: "));
      display.print(statusPacket.message.peakRegen, 1);
      display.println(F("A"));

      display.setTextSize(4);
      display.setCursor(0, 32);
      justify(5, statusPacket.message.totalRegen, 1);

      display.display();
    } // printRegen()


    void printHistory(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: HISTORY"));
        displayTimer.reset();
      }
   
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.println(F("History"));
      display.setTextSize(2);
      for (int i = 2; i >= 0; i--) {
        //display.print(i);
        display.print(F(":"));
        justify(5, statusPacket.message.history[i], 0);
        display.println(F("mAh"));
      }
      display.display();
    } // printHistory()
    
    
    void printStatus() {
      if (screen != prevScreen) {
        Serial.println(F("screen: STATUS"));
        displayTimer.reset();
      }
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.println(F("Status"));
      /*
      if (statusPacket.message.throttle > 0) {
        display.print(F("Gas: "));
        justify(4, statusPacket.message.throttle * 100, 0);
        display.setTextSize(1); 
        display.println(F("%"));
      } else if (statusPacket.message.throttle < 0) {
        display.print(F("Brake:"));
        justify(4, statusPacket.message.throttle * 100, 0);   
        display.setTextSize(1); 
        display.println(F("%")); 
      } else {
        display.println(F("Neutral"));
      }
      */
      display.setTextSize(4);
      justify(4, statusPacket.message.throttle * 100, 0);   
      display.print(F("%"));

      display.setTextSize(1);
      display.setCursor(0,48);
      display.print(F("Uptime: "));
      display.print(statusPacket.message.uptime / 1000);
      display.println(F("s"));
      #ifdef DEBUGGING
      #ifdef MEMORY_FREE_H
      display.print(F("Free Memory: "));
      display.println(freeMemory());
      #endif
      #endif
      display.display();
    }


    unsigned long lastMessage = 0;    
    void printMessage() {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      display.print(statusPacket.message.text[0]);
      display.setCursor(0, 16);
      display.setTextSize(1);
      for (byte i = 1; i < 4; i++) {
        display.println(statusPacket.message.text[i]);
      }
      display.print(F("Last update: "));
      display.print((int)((millis() - lastMessage)/1000));
      display.display();
    } // printMessage(buffer)
    
    
    void update(void) {
      static boolean zPrev = false;
      
      if (! lock) {
        return;
      }
      
      lock = 0;
      statusPacket = statusPacketBuffer;
      #ifdef DEBUGGING_I2C
      Serial.print(F("from buffer: "));
      Serial.println(statusPacket.message.uptime);
      #endif
      
      if (lastByte > (sizeof(statusPacket.bytes) - sizeof(statusPacket.message.text))) {
        nextScreen = screen;
        screen = DISP_MESSAGE;
        lastMessage = millis();
      } else {
        if (statusPacket.message.chuckZ && ! zPrev &&
            abs(statusPacket.message.chuckY) < 0.25) {
          Serial.print(F("screen = "));
          Serial.print(screen);
          Serial.print(F("; prev = "));
          Serial.println(prevScreen);
          if (nextScreen != NO_SCREEN) {
            Serial.println(F("returning to next screen"));
            screen = nextScreen;
            nextScreen = NO_SCREEN;
          } else {
            Serial.println(F("incrementing screens"));
            screen = (screen + 1) % DISP_NUMSCREENS;
          }
        }
      }
      zPrev = statusPacket.message.chuckZ;

      
      unsigned long startMS = millis();
      
      switch (screen) {
        case DISP_MESSAGE:
            printMessage();
            break;
        case DISP_CURRENT: 
            printCurrent();
            break;
        case DISP_DISCHARGE:
            printDischarge();
            break;
        case DISP_REGEN:
            printRegen();
            break;
        case DISP_HISTORY:
            printHistory();
            break;
        case DISP_STATUS: // fallthrough
        default: 
            printStatus();
      }

      if (screen != DISP_MESSAGE) {
        prevScreen = screen;
      }
      
      /*
      Serial.print(F("Display latency: "));
      Serial.print(millis() - startMS);
      Serial.println(F("ms"));
      */
      lastUpdate = millis();
    } // update()
    

#endif
