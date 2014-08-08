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

#ifndef LOGGER_H
#define LOGGER_H

#define TINYQUEUE_SIZE 50
#include "StaticQueue.h"
#include "Throttle.h"

/*
 * Attach one of these: 
 *   http://www.panucatt.com/Current_Sensor_for_Arduino_p/cs-100a.htm
 * to GND, 5V, A1.
 *
 * Log: top 1s A, bottom 1s A, avg A, total charge -Ah, 
 * total discharge Ah, net consumed Ah, duration (s)
 */

// #define DEBUGGING_LOGGER

#define FAKE_AMMETER
#ifdef FAKE_AMMETER
#define analogRead(PIN) (random(20)+512)
#endif

#define HISTORY 10         // save 10 previous rides
#define WRITE_PERIOD 30000 // 30s

class Logger {
  private:
    byte pin;
    int zeroOffset, logEntryAddy;
    StaticQueue values;
    float current;
    unsigned long lastWritten;
    Throttle* throttle;
    
    struct logEntryStruct {
      float peakDischarge;
      float peakRegen;
      float totalDischarge;
      float totalRegen;
      // net == discharge - regen
      unsigned long duration;
    }; 
    
    typedef struct logEntryStruct LogEntry;
    
    LogEntry logEntry;
    
    
/********
 * PATTERNS!
 * http://www.codeproject.com/Articles/721796/Design-patterns-in-action-with-Arduino
 ********/

    // PRIVATE constructor
    Logger(void) {
      pin = lastWritten = 0;
      throttle = Throttle::getInstance();
    } // constructor
    
    
    Logger(Logger const&);
    void operator=(Logger const&);

  
    void findUnusedAddy(void) {
      int addy = EEPROM_LOGGER_ADDY;
      while (addy < 1024 && EEPROM.read(addy) != 255) {
        addy += sizeof(LogEntry);
      }
      
      if (addy + sizeof(LogEntry) > 1024) {
        addy = sizeof(LogEntry);
      }
      
      Serial.print("First unused log address: ");
      Serial.print(addy);
      EEPROM.write(addy, 255);
    }    
    
    
    // write recent history to EEPROM
    void saveValues(void) {
      Serial.print("saving? millis - last = ");
      Serial.print(millis() - lastWritten);
      Serial.print("; current = ");
      Serial.print(abs(current));
      Serial.print("; throttle = ");
      Serial.println(throttle->getThrottle());
      if (millis() - lastWritten > WRITE_PERIOD &&
          abs(current) < 1.5 && 
          abs(throttle->getThrottle()) < THROTTLE_MIN) {
        Serial.println("Logger: saving!");
        lastWritten = millis();
      }
    } // saveValues(void)
    
    
    void showLogEntry() {
      Serial.print("peak discharge: ");
      Serial.print(logEntry.peakDischarge);
      Serial.print("A; peak regen: ");
      Serial.print(logEntry.peakRegen);
      Serial.print("A; total Discharge: ");
      Serial.print(logEntry.totalDischarge);
      Serial.print("mAh; total Regen: ");
      Serial.print(logEntry.totalRegen);
      Serial.print("mAh; net discharge: ");
      Serial.print(logEntry.totalDischarge - logEntry.totalRegen);
      Serial.print("mAh in ");
      Serial.print(logEntry.duration / 1000);
      Serial.println(" seconds");
    } // showLogEntry(LogEntry logEntry)
    
    
  public:
  
    
    static Logger* getInstance(void) {
      static Logger logger;
      return &logger;
    } // static Logger* getInstance()


    void init(int newPin) {
      pin = newPin;
      if (pin) {
        pinMode(pin, INPUT_PULLUP);
        int value = analogRead(pin);
        if (value >= 1000) {
          Serial.println("No ammeter detected; disabling logging");
          pin = 0;
        } else {
          pinMode(pin, INPUT);
          int sum = 0;
          for (int i = 0; i < 50; i++) {
            sum += analogRead(pin);
          }
          int avg = sum / 50;
          zeroOffset = 512 - avg;
          Serial.print("Using 0A offset ");
          Serial.println(zeroOffset);
        }
      }
    }


    void update(void) {
      static int updateCounter = 0;
      if (! pin) return;
      
      int value = analogRead(pin);
      #ifdef DEBUGGING_LOGGER
      Serial.print("Ammeter value: ");
      Serial.print(value);
      #endif
      
      if ((value + zeroOffset) < 0 && (value + zeroOffset) > -3) {
        current = 0;
      } else {
        #define VCC 5.0 // volts
        //  Current = ((analogRead(1)*(5.00/1024))- 2.5)/ .02;
        current = (((value + zeroOffset) * (VCC)/1024) - (VCC)/2) / 0.02;
      }
      values.enqueue(current);
      #ifdef DEBUGGING_LOGGER
      Serial.print("; estimated current: ");
      Serial.print(current);
      #endif
      
      float avgCurrent = values.sum() / TINYQUEUE_SIZE;
      
      if (random(100) < 0) {
        values.dump(Serial);
      }
      
      #ifdef DEBUGGING_LOGGER
      Serial.print("; 50mavg current = ");
      Serial.println(avgCurrent);
      #endif
      
      logEntry.peakDischarge = max(logEntry.peakDischarge, avgCurrent);
      logEntry.peakRegen = min(logEntry.peakRegen, avgCurrent);
      
      float mAh = current * 20 / 3600;  // amps * 20ms / 3600s/H
      
      if (current > 0) {
        logEntry.totalDischarge += mAh;
      } else {
        logEntry.totalRegen -= mAh;
      }
      
      logEntry.duration = millis();
      
      if (++updateCounter % 20 == 0) {
        showLogEntry();
        updateCounter = 0;
      }
      
      saveValues();
    }
    
}; // class Logger

#endif
