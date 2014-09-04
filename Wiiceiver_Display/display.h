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

    #define MASTER_TIMEOUT 2000 // 2 secs

    Timer displayTimer(5000);
    
    #define DISP_STATUS       0
    #define DISP_CURRENT      1
    #define DISP_VOLTAGE      2
    #define DISP_DISCHARGE    3
    #define DISP_REGEN        4
    #define DISP_HISTORY      5
    #define DISP_MESSAGE      6
    #define DISP_SPLASH       7    
    #define DISP_NUMSCREENS   8
    #define NO_SCREEN         99   // not a screen
    byte screen, prevScreen, nextScreen = NO_SCREEN;
    byte lastMessageID = MSG_NOMESSAGE;
    
    StatusPacket_t statusPacket;

  
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
      display.setCursor(0,0);    
      if ((millis() - lastContact) > MASTER_TIMEOUT) {
        display.setTextSize(2);
        display.setTextColor(BLACK, WHITE); // 'inverted' text
        display.println(F("No signal!"));
        display.setCursor(0, 16);
        display.setTextSize(1);
        display.println(F("Lost connection :("));
        display.setTextColor(WHITE);
        display.print("last msg: ");
        display.print((int)((millis() - lastContact)/1000));
        display.println(F(" secs ago"));
      } else {
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.println(F("Wiiceiver"));
        display.println(F(" Display!"));      
      }
      display.setTextSize(1);
      display.print(F("head v"));
      display.println(F(WIICEIVER_HEAD_VERSION));
      display.print(F("Watchdog resets: "));
      display.println((byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      display.print(F("Uptime: "));
      display.print((int)(millis() / 1000));
      display.print(F("s"));
      display.display();
    }    


    // E [###########______] F 
    void showFuelGauge(void) {
      #define PACK_CAPACITY  9000 // mAh
      float fuelLevel = (PACK_CAPACITY
                         - (statusPacket.message.totalDischarge 
                          - statusPacket.message.totalRegen))
                       / PACK_CAPACITY;
      fuelLevel = constrain(fuelLevel, 0.0, 1.0);

      #define SFG_LETTER_H   14   // E / F size, pixels
      #define SFG_LETTER_W   5    // E / F height, pixels
      #define SFG_LETTER_SPC 2    // space between letters, pixels
      #define SFG_LEFT       0
      #define SFG_RIGHT      127
      #define SFG_TOP        0
      #define SFG_BOTTOM     SFG_TOP + SFG_LETTER_H

      // fullness
      #define SFG_LEFT_BORDER (SFG_LEFT + SFG_LETTER_W + SFG_LETTER_SPC)
      #define SFG_RIGHT_BORDER (SFG_RIGHT - (SFG_LETTER_W + SFG_LETTER_SPC))
      byte rightEdge = (byte) (fuelLevel * (SFG_RIGHT - SFG_LEFT) 
                                 + SFG_LEFT);
      display.fillRect(SFG_LEFT, SFG_TOP, 
                       rightEdge, SFG_BOTTOM, 
                       WHITE);
      // ticks @ 25, 50, 75%
      for (byte i = 1; i < 8; i++) {
        byte x = (byte)(1.0 * i/8 * (SFG_RIGHT - SFG_LEFT) 
                                   + SFG_LEFT);
        byte top = SFG_TOP;
        if (i % 2 != 0) {
          top = SFG_TOP + (SFG_BOTTOM - SFG_TOP)/2;
        }
        display.drawLine( x, top, x, SFG_BOTTOM - 1, 
                         (x > rightEdge ? WHITE : BLACK));
      }
      
      // frame          
      display.drawRect(SFG_LEFT, SFG_TOP, 
                       SFG_RIGHT, SFG_BOTTOM, 
                       WHITE);
     } // showFueldGauge()
    
    
    void printCurrent(void) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (! displayTimer.isExpired()) {
        display.print(F("Current"));
      } else {
        showFuelGauge();
      }
      display.println();
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
    } // printCurrent()
    
    
    void printVoltage(void) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (! displayTimer.isExpired()) {
        display.print(F("Voltage"));
      } else {
        showFuelGauge();
      }
      display.println();
      display.setTextSize(1);
      display.print(F("   "));
      display.print(statusPacket.message.startVoltage, 1);
      display.print(F("v - "));
      display.print(statusPacket.message.minVoltage, 1);
      display.print(F("v"));
      
      display.setTextSize(4);
      display.setCursor(0, 32);
      justify(5, statusPacket.message.voltage, 2);

      display.display();
    } // printVoltage()
    
    
    void printDischarge(void) {   
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (! displayTimer.isExpired()) {
        display.print(F("Discharge"));
      } else {
        showFuelGauge();
      }
      display.println();
      display.setTextSize(1);
      display.print(F("Peak draw: "));
      display.print(statusPacket.message.peakDischarge, 1);
      display.println(F("A"));
      
      display.setTextSize(4);
      display.setCursor(0, 32);
      justify(5, statusPacket.message.totalDischarge - statusPacket.message.totalRegen, 0);

      display.display();
    } // printDischarge()


    void printRegen(void) {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (! displayTimer.isExpired()) {
        display.print(F("Regen"));
      } else {
        showFuelGauge();
      }
      display.println();
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
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (! displayTimer.isExpired()) {
        display.println(F("History"));
      } else {
        if (statusPacket.message.lastWritten) {
          display.print(F("Last: "));
          display.println(statusPacket.message.uptime
                        - statusPacket.message.lastWritten);
        } else {
          display.println(F("Not saved"));
        }
      }
      display.setTextSize(2);
      for (int i = 2; i >= 0; i--) {
        // display.print(i);
        // display.print(F(": "));
        justify(4, statusPacket.message.cHistory[i], 0);
        display.print("|");
        justify(4, statusPacket.message.vHistory[i], 1);
        display.println();
      }
      display.display();
    } // printHistory()
    
    
    void printStatus() {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      if (abs(statusPacket.message.throttle) < 0.05) {
        display.println(F("Neutral"));
      } else if (statusPacket.message.throttle > 0) {
        display.println(F("Gas"));
      } else {
        display.println(F("Brake"));
      }
      display.setTextSize(4);
      justify(4, statusPacket.message.throttle * 100, 0);   
      display.print(F("%"));

      display.setTextSize(1);
      display.setCursor(0,48);
      display.print(F("Uptime: "));
      display.print(statusPacket.message.uptime);
      display.println(F("s"));
      #ifdef DEBUGGING
      #ifdef MEMORY_FREE_H
      display.print(F("Free Memory: "));
      display.println(freeMemory());
      #endif
      #endif
      display.display();
    } // printStatus()


    void showMessages(const __FlashStringHelper* buffer1, 
                      const __FlashStringHelper* buffer2, 
                      const __FlashStringHelper* buffer3) {
      display.print(buffer1);
      display.setCursor(0, 18);
      display.setTextSize(1);
      display.println(buffer2);
      display.println(buffer3);
    }
    

    unsigned long lastMessage = 0;    
    void printMessage() {
      display.clearDisplay();
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.setTextSize(2);
      switch (statusPacket.message.messageID) {
        case MSG_STARTUP:
          display.println(F("Wiiceiver"));
          display.setTextSize(1);
          display.print(F("Master v "));
          display.println(statusPacket.message.text);
          display.print(F("Host resets: "));
          display.println(statusPacket.message.watchdogResets);
          break;
        case MSG_CALIBRATION_1:
          showMessages(F("Calibrate?"), F("Hold C..."), F(""));
          break;
        case MSG_CALIBRATION_2:
          showMessages(F("Calibrated!"), F("Chuck center"), F("  stored."));
          break;
        case MSG_CALIBRATION_3:
          showMessages(F("Skipped"), F("Calibration cancelled"), F("Reset to try again"));
          break;
        case MSG_CHUCK_1:
          showMessages(F("NO CHUCK!"), F("Lost signal"), F("  from nunchuck"));
          break;
        case MSG_CHUCK_2:
          showMessages(F("WAITING..."), F("Return stick"), F("  to center"));
          break;
        case MSG_CHUCK_3:
          showMessages(F("Active!"), F("Resuming operation"), F(""));
          break;
          display.print("ID ");
          display.println(statusPacket.message.messageID);
          break;
        default:
          display.println("UNKNOWN");
          display.print("wot: ");
          display.println(statusPacket.message.messageID);
      }

      display.setTextSize(1);
      display.setCursor(0, 48);
      display.print(F("Host uptime: "));
      display.print(statusPacket.message.uptime);
      display.println(F("s"));
      display.print(F("Last update: "));
      display.print((int)((millis() - lastMessage)/1000));
      display.display();
    } // printMessage(buffer)
    
    
    void update(void) {      
      static boolean zPrev = false;
      if ((millis() - lastContact) > MASTER_TIMEOUT) {
        Serial.println(F("Timeout :/"));
        splashScreen();
        return;
      }
      if (! lock) {
        return;
      }
      
      lock = 0;
      statusPacket = statusPacketBuffer;
      // memcopy(&statusPacketBuffer, &statusPacket, sizeof(statusPacketBuffer));
      #ifdef DEBUGGING_I2C
      Serial.print(F("from buffer: "));
      Serial.println(statusPacket.message.uptime);
      #endif
      
      if (statusPacket.message.messageID != lastMessageID) {
        // sorta a hack; if text was sent, switch to the text screen.
        nextScreen = screen;
        screen = DISP_MESSAGE;
        lastMessage = millis();
        lastMessageID = statusPacket.message.messageID;
        displayTimer.reset();
      } else {
        if (! statusPacket.message.chuckZ && zPrev &&
            abs(statusPacket.message.chuckY) < 0.25) {
          // release Z (Z from ON -> OFF) & no significant Y axis...
          #ifdef DEBUGGING
          Serial.print(F("screen = "));
          Serial.print(screen);
          Serial.print(F("; prev = "));
          Serial.println(prevScreen);
          #endif
          displayTimer.reset();
          if (nextScreen != NO_SCREEN) {
            #ifdef DEBUGGING
            Serial.println(F("returning to next screen"));
            #endif
            screen = nextScreen;
            nextScreen = NO_SCREEN;
          } else {
            #ifdef DEBUGGING
            Serial.println(F("incrementing screens"));
            #endif
            screen = (screen + 1) % DISP_NUMSCREENS;
          }
        }
      }
      zPrev = statusPacket.message.chuckZ;

      
      unsigned long startMS = millis();
      
      // not using a lookup table, for readability purposes
      switch (screen) {
        case DISP_MESSAGE:
            printMessage();
            break;
        case DISP_CURRENT: 
            printCurrent();
            break;
        case DISP_VOLTAGE:
            printVoltage();
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
        case DISP_SPLASH:
            splashScreen();
            break;    
        case DISP_STATUS: // fallthrough
        default: 
            printStatus();
      }

      if (screen != DISP_MESSAGE) {
        prevScreen = screen;
      }
      
      #ifdef DEBUGGING
      Serial.print(F("Display latency: "));
      Serial.print(millis() - startMS);
      Serial.println(F("ms"));
      #endif
    } // update()
    

#endif
