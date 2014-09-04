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

#ifndef LOGGER_H
#define LOGGER_H

#define TINYQUEUE_SIZE 10
#include "StaticQueue.h"
#include "Throttle.h"
#include "EEPROMAnything.h"

/*
 * Attach one of these: 
 *   http://www.panucatt.com/Current_Sensor_for_Arduino_p/cs-100a.htm
 * to GND, 5V, A1.
 *
 * If an ammeter is not detected, this code does nothing
 *   that is, it works on a "normal" wiiceiver.
 *
 * Log: top 1s A, bottom 1s A, avg A, total charge -Ah, 
 * total discharge Ah, net consumed Ah, duration (s)
 *
 * Theory of Operation:
 * A rolling (circular) list of LogEntry records is stored in EEPROM
 * at a given starting location.  A new record is written after at least
 * 30s; at that time, the next record (possibly wrapping to 0) is 
 * cleared by writing 255 to the first byte.  This new record is rewritten
 * every ~30s until power off.  Subsequent startups writes to the next
 * record (first with 255).
 * 
 * To replay the log, find the first 255, then read forward HISTORY
 * records.
 */

// #define DEBUGGING_LOGGER
// #define DEBUGGING_LOGGER_CURRENT
// #define DEBUGGING_LOGGER_VOLTAGE
// #define DEBUGGING_LOGGER_STORAGE

// for bench testing, the FAKE_*METER will inject ~random data influenced by 
// chuck Y position.
// #define FAKE_AMMETER
// #define FAKE_VOLTMETER

#define HISTORY 30         // save N previous rides (~upper limit at 1024 bytes EEPROM)
#define WRITE_PERIOD 30000 // 30s

class Logger {
private:
  byte ammeterPin, voltmeterPin;
  int zeroOffset, logEntryBlock, discardCounter;
  StaticQueue <int> values;
  float current, voltage, cHistory[3], vHistory[3];
  unsigned long lastWritten;
  Throttle* throttle;

  // increment this when logEntryStruct changes
  #define LOGGER_VERSION 0
  struct logEntryStruct {
    float peakDischarge;
    float peakRegen;
    float totalDischarge;
    float totalRegen;
    // net == discharge - regen
    float startVoltage;
    float minVoltage;
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
    ammeterPin = voltmeterPin = lastWritten = 0;
    throttle = Throttle::getInstance();
  } // constructor


  Logger(Logger const&);
  void operator=(Logger const&);


/***************
 *
 * Sensor handling
 *
 ***************/

  #ifdef FAKE_AMMETER
  #define AMMETER_READ(PIN) ((100 * Chuck::getInstance()->Y)+512)
  #else
  #define AMMETER_READ(PIN) analogRead(PIN)
  #endif


  // polls the ammeter and attempts to identify a zero point
  // after a few weeks of observation this appears to be super accurate
  // returns false if not detected
  bool calibrateAmmeter(byte pin) {
    if (pin) {
      pinMode(pin, INPUT_PULLUP);
      int value = AMMETER_READ(pin);
      if (value >= 1000) {
        Serial.println(F("No ammeter detected; disabling logging"));
        pin = 0;
      } else {
        pinMode(pin, INPUT);
        int sum = 0;
        for (int i = 0; i < 50; i++) {
          sum += AMMETER_READ(pin);
        }
        int avg = sum / 50;
        zeroOffset = 512 - avg;
        // DEBUGGING
        Serial.print(F("Using 0A offset: "));
        Serial.println(zeroOffset);
      }
    }    
    return pin != 0;
  } // bool calibrateAmmeter()
  
  
  // reads the ammeter, and updates internal data structures 
  void readAmmeter(void) {
    int value = AMMETER_READ(ammeterPin);
    #ifdef DEBUGGING_LOGGER_CURRENT
    Serial.print(F("Ammeter value: "));
    Serial.print(value);
    #endif
    
    // sanity check 250 < value < 1023
    if (250 > value || value > 1023) {
      Serial.print("ZOMG: nonsensical ammeter reading: ");
      Serial.println(value);
      current = 0;
    } else if ((value + zeroOffset) < 0 && (value + zeroOffset) > -3) {
      current = 0;
    } else {
      #define VCC 5.0 // volts
      //  Current = ((analogRead(1)*(5.00/1024))- 2.5)/ .02;
      current = (((value + zeroOffset) * (VCC)/1024) - (VCC)/2) / 0.02;
      // sanity check -10 < current < 100
      
      if (-30 > current || current > 100) {
        Serial.print("ZOMG current is out of bounds; discarding: ");
        Serial.println(current);
        ++discardCounter;
        // current = 0;
        current = constrain(current, -10, 100);
      }
    }
    values.enqueue(current);
    #ifdef DEBUGGING_LOGGER_CURRENT
    Serial.print(F("; estimated current: "));
    Serial.print(current);
    #endif

    float avgCurrent = values.sum() / TINYQUEUE_SIZE;

    #ifdef NEVER_DO_THIS
    if (random(100) > 99) {
      values.dump(Serial);
    }
    #endif

    #ifdef DEBUGGING_LOGGER_CURRENT
    Serial.print(F("; 50mavg current = "));
    Serial.println(avgCurrent);
    #endif

    logEntry.peakDischarge = max(logEntry.peakDischarge, avgCurrent);
    logEntry.peakRegen = min(logEntry.peakRegen, avgCurrent);

    float mAh = current * 20 / 3600;  // amps * 20ms / 3600s/H
    // sanity-check
    if (current > 0) {
      logEntry.totalDischarge += mAh;
    } 
    else {
      logEntry.totalRegen -= mAh;
    }    
  } // readAmmeter()


  #ifdef FAKE_VOLTMETER
  #define VOLTMETER_READ(PIN) (900+100 * Chuck::getInstance()->Y)
  #else
  #define VOLTMETER_READ(PIN) analogRead(PIN)
  #endif
  
  
  /*
   * there isn't a magic way to "calibrate" a voltage divider :/
   * we just dump the value read, and the user can #define the 
   * actual calibrated voltage
   * returns false if not detected
   *
   * ideal math + perfect resistors:
   *    8S: 33.6, 999
   *    6S: 25.2, 678
   */
  bool calibrateVoltmeter(byte pin) {
    /*
     * 1. hook up a voltmeter (don't just read it cold)
     * 2. start Wiiceiver w/ serial console
     * 3. watch the console, and add the voltage measured (decimal)
     *    and value read (integer) below
     * 4. recompile
     */
    #define VOLTAGE_REF       33.6
    #define VALUE_REF          931
    if (pin) {
      pinMode(pin, INPUT_PULLUP);
      int value = VOLTMETER_READ(pin);
      if (value >= 1000) {
        Serial.println(F("No voltmeter detected; disabling logging"));
        pin = 0;
      } else {
        pinMode(pin, INPUT);
        unsigned int sum = 0;
        for (int i = 0; i < 50; i++) {
          sum += VOLTMETER_READ(pin);
        }
        int avg = sum / 50;
        // DEBUGGING?
        Serial.println(F("Calibration instructions: "));
        Serial.println(F("Measure the *actual* voltage and set as VOLTAGE_REF"));
        Serial.println(F("And update:"));
        Serial.print(F("      #define VALUE_REF   "));
        Serial.println(avg);
        Serial.println(F("then re-upload this sketch"));
        
        logEntry.startVoltage = logEntry.minVoltage = readVoltage(pin);
      }
    }    
    return pin != 0;
  } // bool calibrateVoltmeter()
  
  
  // reads the ADC on pin and calculates a voltage
  // based on a known divider reference
  float readVoltage(byte pin) {
    int value = VOLTMETER_READ(pin);
    float V = (float)(VOLTAGE_REF * value / VALUE_REF);
    return V;
  } // float readVoltage(pin)
  
  
  void readVoltmeter(void) {
    float V = readVoltage(voltmeterPin);
    if (V > 16 && V < 40) {
      voltage = V;
      logEntry.minVoltage = min(logEntry.minVoltage, voltage);
      #ifdef DEBUGGING_LOGGER_VOLTAGE
      Serial.print(F("Voltage: start "));
      Serial.print(logEntry.startVoltage);
      Serial.print(F("v; now "));
      Serial.print(voltage);
      Serial.println(F("v"));
      #endif
    } else {
      #ifdef DEBUGGING_LOGGER_VOLTAGE
      Serial.print(F("Bogus voltage: "));
      Serial.println(V);
      #endif
    }
  } // readVoltmeter()


  // convert a block location to an EEPROM address
  int blockToAddy(int block) {
    return EEPROM_LOGGER_ADDY + block * sizeof(LogEntry);
  } // int blockToAddy(block)


  // checks the logger struct version, and 
  // formats flash to match (if needed)
  void confirmLoggerFormat(void) {
    byte formatVersion = EEPROM.read(EEPROM_LOGGER_VERSION);
    if (formatVersion == LOGGER_VERSION) {
      return;
    }
    
    Serial.print("Logger format version check: old version = ");
    Serial.print(formatVersion);
    Serial.print("; updating to ");
    Serial.println(LOGGER_VERSION);
    for (byte block = 0; block < HISTORY; block ++) {
      clearBlock(block);
    }
    
    EEPROM.write(EEPROM_LOGGER_VERSION, LOGGER_VERSION);
  } // confirmLoggerVersion()


  // returns the block 0..HISTORY-1 for the to-be-written logEntry
  int findUnusedBlock(void) {
    int block = 0;    // logical "block", 0..HISTORY-1
    while (block < HISTORY && EEPROM.read(blockToAddy(block)) != 255) {
#ifdef DEBUGGING_LOGGER
      Serial.print("checking block #");
      Serial.print(block);
      Serial.print(", address ");
      Serial.println(blockToAddy(block));
#endif
      block += 1;
    }

    if (block >= HISTORY) {
      block = 0;
    }
    return block;
  } // int findUnusedBlock()


  // clears the specified block, if needed
  void clearBlock(byte block) {
    if (block >= HISTORY) {
      block = 0;
    }

    if (EEPROM.read(blockToAddy(block)) != 255) {
      Serial.print(F(" clearing block #"));
      Serial.print(block);
      Serial.print(F(" at address ..."));
      Serial.println(blockToAddy(block));
      EEPROM.write(blockToAddy(block), 255);
    }
  } // clearNextAddy(block)


  // write recent history to EEPROM
  void saveValues(void) {
#ifdef DEBUGGING_LOGGER_STORAGE
    Serial.print("saving? millis - last = ");
    Serial.print(millis() - lastWritten);
    Serial.print("; current = ");
    Serial.print(abs(current));
    Serial.print("; throttle = ");
    Serial.println(throttle->getThrottle());
#endif
    if (millis() - lastWritten > WRITE_PERIOD &&
      abs(current) < 1.5 && 
      logEntry.totalDischarge > 50 &&
      abs(Chuck::getInstance()->Y) < THROTTLE_MIN) {
      Serial.print(F("Logger: saving ..."));
      unsigned long start = millis();
      clearBlock(logEntryBlock+1);
      EEPROM_writeAnything(blockToAddy(logEntryBlock), logEntry);
      Serial.print(F("; done in "));
      Serial.print(millis() - start);
      Serial.println(F("ms"));
      lastWritten = millis();
    }
  } // saveValues(void)


  // prints a single LogEntry record
  void showLogEntry(LogEntry entry) {
    Serial.print(entry.startVoltage);
    Serial.print(F("v - "));
    Serial.print(entry.minVoltage);
    Serial.print(F("v; "));
    Serial.print(F("peak discharge: "));
    Serial.print(entry.peakDischarge);
    Serial.print(F("A; peak regen: "));
    Serial.print(entry.peakRegen);
    Serial.print(F("A; total Discharge: "));
    Serial.print(entry.totalDischarge);
    Serial.print(F("mAh; total Regen: "));
    Serial.print(entry.totalRegen);
    Serial.print(F("mAh; net discharge: "));
    Serial.print(entry.totalDischarge - entry.totalRegen);
    Serial.print(F("mAh in "));
    Serial.print(entry.duration / 1000);
    Serial.println(F(" seconds"));
  } // showLogEntry(LogEntry logEntry)


  // displays the history in the log (EEPROM)
  void showLogHistory(void) {
    LogEntry entry;
    byte b; 
    byte block = logEntryBlock;

    Serial.println(F("History (oldest to newest):"));
    for (b = 0; b < HISTORY; b ++) {
      if (EEPROM.read(blockToAddy(block)) != 255 &&
        EEPROM_readAnything(blockToAddy(block), entry)) {
        Serial.print(F("#"));
        Serial.print(block);
        Serial.print(F(": "));
        showLogEntry(entry);
      }
      if (++block >= HISTORY) {
        block = 0;
      }
    }
  } // showLogHistory()


  void buildHistory(void) {
    LogEntry entry;
    byte block = logEntryBlock;
    for (byte b = 0; b < 3; b++) {
      if (--block < 0) {
        block = HISTORY;
      }
      #ifdef DEBUGGING_LOGGER_STORAGE
      Serial.print(F("Looking @ block "));
      Serial.println(block);
      #endif
      if (EEPROM.read(blockToAddy(block)) == 255) {
        cHistory[b] = vHistory[b] = 0.0;
      } else {
        EEPROM_readAnything(blockToAddy(block), entry);
        cHistory[b] = entry.totalDischarge - entry.totalRegen;
        vHistory[b] = entry.minVoltage;
      }
    }
  } // buildHistory()
  

public:

  // returns the Singleton instance
  static Logger* getInstance(void) {
    static Logger logger;    // NB: I don't like this idiom
    return &logger;
  } // static Logger* getInstance()


  void init() {
    memset(&logEntry, 0, sizeof(logEntry));
    
    ammeterPin = pinLocation(AMMETER_ID);
    if (! calibrateAmmeter(ammeterPin)) {
      ammeterPin = 0;
    }
    
    voltmeterPin = pinLocation(VOLTMETER_ID);
    if (! calibrateVoltmeter(voltmeterPin)) {
      voltmeterPin = 0;
    }
    
    if (ammeterPin || voltmeterPin) {
      confirmLoggerFormat();
      logEntryBlock = findUnusedBlock();
      Serial.print(F("This entry: block #"));
      Serial.println(logEntryBlock);

      showLogHistory();
      buildHistory();
        
      discardCounter = 0;
    }
  } // init()


  void update(void) {
    // shortcut 
    if (! ammeterPin && ! voltmeterPin) return;
    
    #ifdef DEBUGGING_LOGGER
    static int updateCounter = 0;
    #endif
    
    
    if (ammeterPin) {
      readAmmeter();
    }
    
    if (voltmeterPin) {
      readVoltmeter();
    }
/*
    int value = analogRead(pin);
    #ifdef DEBUGGING_LOGGER
    Serial.print(F("Ammeter value: "));
    Serial.print(value);
    #endif
    
    // sanity check 250 < value < 1023
    if (250 > value || value > 1023) {
      Serial.print("ZOMG: nonsensical ammeter reading: ");
      Serial.println(value);
      current = 0;
    } else if ((value + zeroOffset) < 0 && (value + zeroOffset) > -3) {
      current = 0;
    } else {
      #define VCC 5.0 // volts
      //  Current = ((analogRead(1)*(5.00/1024))- 2.5)/ .02;
      current = (((value + zeroOffset) * (VCC)/1024) - (VCC)/2) / 0.02;
      // sanity check -10 < current < 100
      
      if (-10 > current || current > 100) {
        Serial.print("ZOMG current is out of bounds; discarding: ");
        Serial.println(current);
        ++discardCounter;
        // current = 0;
        current = constrain(current, -10, 100);
      }
    }
    values.enqueue(current);
    #ifdef DEBUGGING_LOGGER
    Serial.print(F("; estimated current: "));
    Serial.print(current);
    #endif

    float avgCurrent = values.sum() / TINYQUEUE_SIZE;

    #ifdef NEVER_DO_THIS
    if (random(100) > 99) {
      values.dump(Serial);
    }
    #endif

    #ifdef DEBUGGING_LOGGER
    Serial.print(F("; 50mavg current = "));
    Serial.println(avgCurrent);
    #endif

    logEntry.peakDischarge = max(logEntry.peakDischarge, avgCurrent);
    logEntry.peakRegen = min(logEntry.peakRegen, avgCurrent);

    float mAh = current * 20 / 3600;  // amps * 20ms / 3600s/H
    // sanity-check
    if (current > 0) {
      logEntry.totalDischarge += mAh;
    } 
    else {
      logEntry.totalRegen -= mAh;
    }
    */
    logEntry.duration = millis();
    saveValues();

    #ifdef DEBUGGING_LOGGER
    if (++updateCounter % 20 == 0) {
      showLogEntry(logEntry);
      updateCounter = 0;
    }
    #endif
  }


  float getDischarge(void) {
    return logEntry.totalDischarge;
  }


  float getRegen(void) {
    return logEntry.totalRegen;
  }


  float getNetDischarge(void) {
    return logEntry.totalDischarge - logEntry.totalRegen;
  }


  float getPeakDischarge(void) {
    return logEntry.peakDischarge;
  }
  
  
  float getPeakRegen(void) {
    return logEntry.peakRegen;
  }


  float getCurrent(void) {
    return current;
  }
  
  
  float getAvgCurrent(const byte n) {
    return values.sum(n) / n;
  }
  
  
  float getStartVoltage(void) {
    return logEntry.startVoltage;
  }
  
  
  float getVoltage(void) {
    return voltage;
  }
  
  
  float getMinVoltage(void) {
    return logEntry.minVoltage;
  }
  
  
  unsigned long getLastWritten() {
    return lastWritten;
  }
  

  // n = 0..2 returns one of the previous 3 rides
  float getNthCRec(byte n) {
    return cHistory[n];
  }
  
   // n = 0..2 returns one of the previous 3 rides
  float getNthVRec(byte n) {
    return vHistory[n];
  }
 

}; // class Logger

#endif

