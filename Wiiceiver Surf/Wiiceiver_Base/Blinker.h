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

#ifndef BLINKER_H
#define BLINKER_H

#include <Arduino.h>

#ifndef BLINKY_LIT_DURATION
#define BLINKY_LIT_DURATION 25           // 50ms "on"; max BPS =~ 20  
#endif

/*
 * Blinker: asynchronously blinks a specified LED at some rate
 *
 * ex: 
 *   #include <Blinker.h>
 *   Blinker led = Blinker(13);  // blink LED on pin 13
 *   void setup(void) {
 *     led.start(10);          // blink 10x / sec
 *   }
 *   void loop(void) {
 *     led.run();              // change the LED if needed
 *     ... 
 *     delay(10);
 *   }
 */
class Blinker {
  public:
    Blinker();
    void init(int LED);
    void start(int BPS);
    void update(int BPS);
    void run(void);
    void high(void);
    void low(void);
    void stop(void);
  private:
    void _blink(int level);
    int _led;                             // LED pin
    int _bps;                             // blinks per second
    unsigned long _nextMillis;            // time of next transition
    int _state;                           // LED state, HIGH || LOW
};


// constructor; requires the pin to blink
Blinker::Blinker() {
  _led = 0;
  _nextMillis = 0;
} // Blinker::Blinker(int LED)


void Blinker::init(int LED) {
  _led = LED;
  pinMode(_led, OUTPUT);
  stop();
}


// asynch run; if appropriate, will turn the LED on / off
void Blinker::run(void) {
  // shortcut: not blinking?
  if (_nextMillis == 0 || _bps == 0) {
    return;
  }

  unsigned long currentMillis = millis();
  if (_nextMillis > currentMillis) {
    // not time yet
    return;
  }
  
  if (_state == LOW) {
    _state = HIGH;
    _nextMillis = currentMillis + BLINKY_LIT_DURATION;
  } else {
    _state = LOW;
    _nextMillis = currentMillis + 1000/_bps - BLINKY_LIT_DURATION;
  }
  
  _blink(_state);
} // void Blinker::run(void)


// start blinking BPS blinks per second
void Blinker::start(int BPS) {
  stop();
  update(BPS);
  _nextMillis = millis()+1;
  _blink(LOW);
  run();
} // void Blinker::start(int BPS)


// update the blink rate
// 
// does not take effect until the next state transition
// This will not "break" the asynch blinking -- "updating"
// many times per second will not blink faster than BPS
void Blinker::update(int BPS) {
  _bps = BPS;
} // void Blinker::update(int BPS)


// stop blinking immediately
void Blinker::stop(void) {
  _nextMillis = 0;
  _state = LOW;
  _blink(LOW);
} // void Blinker::stop(void)


// stop blinking & remain HIGH
void Blinker::high(void) {
    stop();
    _blink(HIGH);
} // void Blinker::high(void)


// stop blinking & remain LOW, same as stop()
void Blinker::low(void) {
  stop();
} // void Blinker::high(void)



// private: actually blink the thingy
void Blinker::_blink(int level) { 
  digitalWrite(_led, level);
#ifdef BLINKY_DEBUG
  Serial.print(level);
  Serial.print(F(": next in Millis: "));
  Serial.println(_nextMillis);
#endif
} // void Blinker::_blink(int level)

#endif
