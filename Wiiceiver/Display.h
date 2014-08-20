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

#define UPDATE_RATE 10      // higher == slower; # 20ms cycles between updates.  10 == 2FPS
class Display {
  private:
    Adafruit_SSD1306 *adafruitDisplay;
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
  
  void justify(byte width, float value, byte decimals) {
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
      adafruitDisplay->print(F(" "));        
    }
    adafruitDisplay->print(value, decimals);
  } // justify(width, value, decimals)


  public:
  
    static Display* getInstance(void) {
      static Display myDisplay;
      return &myDisplay;
    }

    void attachChuck(Chuck* newChuck) {
      chuck = newChuck;
    }
    
    
    void init(Adafruit_SSD1306* newAdafruitDisplay) {      
      adafruitDisplay = newAdafruitDisplay;
      adafruitDisplay->begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
      splashScreen();
      screen = DISP_DISCHARGE;
    }


    void splashScreen(void) {
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      adafruitDisplay->setTextSize(2);
      adafruitDisplay->print(F("Wiiceiver!"));
      adafruitDisplay->setTextSize(1);
      adafruitDisplay->println(F(WIICEIVER_VERSION));
      adafruitDisplay->print(F("Watchdog resets: "));
      adafruitDisplay->println((byte)(EEPROM.read(EEPROM_WDC_ADDY) + 1));
      adafruitDisplay->display();
    }    

    
    void printCurrent(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: CURRENT"));
        timer.reset(5000);
      }
      
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      if (! timer.isExpired()) {  
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("Current"));
        justify(5, logger->getCurrent(), 2);
        adafruitDisplay->println(F("A"));
      } else {
        adafruitDisplay->setTextSize(1);
        adafruitDisplay->print(F("Current: "));
        adafruitDisplay->print(logger->getPeakRegen(), 1);
        adafruitDisplay->print(F(", "));
        adafruitDisplay->println(logger->getPeakDischarge(), 1);
        adafruitDisplay->setTextSize(3);
        justify(5, logger->getCurrent(), 2);
        adafruitDisplay->print(F("A"));
      }
      adafruitDisplay->display();
    }
    
    
    void printDischarge(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: DISCHARGE"));
        timer.reset();
      }
   
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      if (! timer.isExpired()) {
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("DISCHARGE"));
        justify(5, logger->getNetDischarge(), 1);
        adafruitDisplay->println(F("mAh"));
      } else {
        adafruitDisplay->setTextSize(1);
        adafruitDisplay->print(F("Peak Discharge:"));
        adafruitDisplay->print(logger->getPeakDischarge(), 1);
        adafruitDisplay->println(F("A"));
        adafruitDisplay->setTextSize(3);
        justify(5, logger->getNetDischarge(), 1);
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("mAh"));
      }
      adafruitDisplay->display();
    }


    

    void printRegen(void) {
      char buf[5];
      if (screen != prevScreen) {
        Serial.println(F("screen: REGEN"));
        timer.reset();
      }
   
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      if (! timer.isExpired()) {
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("REGEN"));
        justify(5, logger->getRegen(), 1);
        adafruitDisplay->println(F("mAh"));
      } else {
        adafruitDisplay->setTextSize(1);
        adafruitDisplay->print(F("Peak Regen: "));
        adafruitDisplay->print(logger->getPeakRegen(), 1);
        adafruitDisplay->println(F("A"));
        adafruitDisplay->setTextSize(3);
        justify(5, logger->getRegen(), 1);
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("mAh"));
      }
      adafruitDisplay->display();
    } // printRegen()


    void printHistory(void) {
      if (screen != prevScreen) {
        Serial.println(F("screen: HISTORY"));
        timer.reset();
      }
   
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      if (! timer.isExpired()) {
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("HISTORY"));
        adafruitDisplay->setTextSize(1);
        for (int i = 2; i > 0; i--) {
          adafruitDisplay->print(i);
          adafruitDisplay->print(F(": "));
          adafruitDisplay->print(logger->getNthRec(i));
          adafruitDisplay->println(F("mAh"));
        }
      } else {
        adafruitDisplay->setTextSize(1);
        adafruitDisplay->println(F("HISTORY, mAh"));
        for (int i = 2; i >= 0; i--) {
          adafruitDisplay->print(i);
          adafruitDisplay->print(F(": "));
          adafruitDisplay->print(logger->getNthRec(i));
          adafruitDisplay->println(F("mAh"));
        }
      }
      adafruitDisplay->display();
    } // printHistory()
    
    
    void printStatus() {
      if (screen != prevScreen) {
        Serial.println(F("screen: STATUS"));
        timer.reset();
      }
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      if (! timer.isExpired()) {
        adafruitDisplay->setTextSize(2);
        adafruitDisplay->println(F("STATUS"));
        adafruitDisplay->setTextSize(1);
      } else {
        adafruitDisplay->setTextSize(1);
        adafruitDisplay->println(F("Status"));
        adafruitDisplay->setTextSize(2);
      }
      if (throttle->getThrottle() > 0) {
        adafruitDisplay->print(F("Gas: "));
        adafruitDisplay->print(throttle->getThrottle() * 100, 0);
        adafruitDisplay->println(F("%"));    
      } else if (throttle->getThrottle() < 0) {
        adafruitDisplay->print(F("Brake:"));
        adafruitDisplay->print(throttle->getThrottle() * -100, 0);    
        adafruitDisplay->println(F("%"));    
      } else {
        adafruitDisplay->println(F("Neutral"));
      }
      adafruitDisplay->setTextSize(1);
      adafruitDisplay->print(F("Uptime: "));
      adafruitDisplay->print(millis() / 1000);
      adafruitDisplay->println(F("s"));
      adafruitDisplay->display();
    }

    
    void printMessage(char buffer[]) {
      Serial.println(F("screen: MESSAGE"));
      if ((screen != DISP_MESSAGE) && 
          (prevScreen != DISP_MESSAGE)) {
        prevScreen = screen;
      }
      screen = DISP_MESSAGE;
      adafruitDisplay->clearDisplay();
      adafruitDisplay->setTextColor(WHITE);
      adafruitDisplay->setCursor(0,0);
      adafruitDisplay->setTextSize(2);
      adafruitDisplay->println(buffer);
      adafruitDisplay->display();
    } // printMessage(buffer)
    
    
    void update(void) {
      if (chuck->Z && ! chuck->zPrev &&
          abs(chuck->Y) < 0.25) {
        if (screen == DISP_MESSAGE) {
          screen = prevScreen;
          timer.reset();
        } else {
          screen = (screen + 1) % DISP_NUMSCREENS;
        }
      }

      if (++updateCtr < UPDATE_RATE) {
        return;
      }
      
      updateCtr = 0;
      unsigned long startMS = millis();
      
      Serial.print(F("screen = "));
      Serial.print(screen);
      Serial.print(F("; prev = "));
      Serial.println(prevScreen);
      switch (screen) {
        case DISP_MESSAGE:
            // NOP
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
      Serial.print(F("Display latency: "));
      Serial.print(millis() - startMS);
      Serial.println(F("ms"));
    }
    
    
    
};

#endif
