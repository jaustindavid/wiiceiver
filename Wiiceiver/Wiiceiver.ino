#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>


// #define DEBUGGING

#include "Blinker.h"

// #define DEBUGGING_SMOOTHER
#include "Smoother.h"

// #define DEBUGGING_CHUCK
// #define DEBUGGING_CHUCK_ACTIVITY
#define WII_ACTIVITY_COUNTER 100  // once per 20ms; 50 per second
#include "Chuck.h"

// #define DEBUGGING_ESC
#include "ElectronicSpeedController.h"


// #define DEBUGGING_THROTTLE

/*
 * Wiiceiver hardware definitions
 * don't change these without changing the board 
 */
#define GREEN_LED 7   // any digital pin; DIP13
#define RED_LED 8     // any digital pin; DIP14

#define ESC_PPM 10    // PWM required; DIP16
#define ESC_GROUND 00 // hard-wired

#define WII_SCL 19    // aka A5 -- DO NOT USE  (Wire needs it)
#define WII_SDA 18    // aka A4 -- DO NOT USE  (Wire needs it)
#define WII_POWER 9   // any digital pin; DIP15
#define WII_GROUND 00 // hard-wired




Chuck chuck = Chuck();
ElectronicSpeedController ESC;
Blinker green = Blinker(GREEN_LED);
Blinker red = Blinker(RED_LED);
Smoother smoother = Smoother(0.2);


// maybe calibrate the joystick:
//   read the C button 50 times, once per 100ms (5s total); if it's constantly
//   down, calibrate the joystick
void maybeCalibrate(void) {
  int ctr = 0;
  int i = 0;

  chuck.update();
  while (i < 250 && chuck.C) {
    chuck.update();
    red.run();
    green.run();
    i++;
    ctr += chuck.C;
    delay(20);
  }

#ifdef DEBUGGING  
  Serial.print("C = ");
  Serial.println(ctr);
#endif

  if (chuck.C == 1 && chuck.isActive()) {
    chuck.calibrateCenter();
    chuck.writeEEPROM();
#ifdef DEBUGGING
    Serial.println("Calibrated");
#endif
  }

  red.update(1);
  green.update(1);
}


// an unambiguous startup display
void splashScreen() {
  int i;
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(500);
  for (i = 0; i < 10; i++) {
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(RED_LED, LOW);
    delay(50);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    delay(50);
  }
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, HIGH);
  delay(500);
  digitalWrite(GREEN_LED, LOW);    
  digitalWrite(RED_LED, LOW);  
} // void splashScreen(void)


// flash the LEDs to indicate throttle position
void updateLEDs(float throttle) {
  if (throttle == 0) {
    green.update(1);
    red.update(1);
  } else {
    int bps = abs(int(throttle * 20));

    if (throttle > 0) {
      green.update(bps);
      red.update(1);
    } else {
      green.update(1);
      red.update(bps);
    }
  }
} // updateLEDs(float throttle)


// the nunchuck appears to be static: we lost connection!
// go "dead" for up to 5s, but keep checking the chuck to see if
// it comes back
void freakOut(void) {
  unsigned long targetMS = millis() + 5000;
  bool redOn = false;
  byte blinkCtr = 0;

#ifdef DEBUGGING
    Serial.print(millis());
    Serial.println(": freaking out");
#endif

  red.stop();
  green.stop();
  while (!chuck.isActive() && targetMS > millis()) {
//  while (targetMS > millis()) {
    if (blinkCtr >= 4) {
      blinkCtr = 0;
      if (redOn) {
        red.high();
        green.low();
        redOn = false;
      } 
      else {
        red.low();
        green.high();
        redOn = true;
      }
    }
    blinkCtr ++;
    chuck.update();
    delay(20);
  }
  green.start(1);
  red.start(1);
} // void freakOut(void)



void setup_pins() {
  /*
  pinMode(ESC_GROUND, OUTPUT);
  digitalWrite(ESC_GROUND, LOW);
  pinMode(WII_GROUND, OUTPUT);
  digitalWrite(WII_GROUND, LOW);
  */
  pinMode(WII_POWER, OUTPUT);
  digitalWrite(WII_POWER, HIGH);
} // setup_pins()



// dead code
void stopChuck() {
#ifdef DEBUGGING
    Serial.println("Nunchuck: power off");
#endif
  digitalWrite(WII_POWER, LOW);
  delay(250);
#ifdef DEBUGGING
    Serial.println("Nunchuck: power on");
#endif
digitalWrite(WII_POWER, HIGH);
  delay(5000);
} // stopChuck()


// returns true if the chuck appears "active"
bool startChuck() {
  int tries = 0;
  
  while (tries < 5) {
#ifdef DEBUGGING
    Serial.println("(Re)starting the nunchuck");
#endif
    chuck.setup();
    tries ++;
    if (chuck.isActive()) {
      return true;
    }
  }
  return false;
} // startChuck()



#define MIN_THROTTLE 0.05      // the lowest throttle to send the ESC
#define THROTTLE_CC_BUMP 0.003 // CC = 0.1% throttle increase; 50/s = 10s to hit 100% on cruise
float lastThrottle = 0;        // global-ish
float getThrottle() {
  float throttle;

#ifdef DEBUGGING_THROTTLE
    Serial.print("Throttle ");
#endif

  if (chuck.C) { // cruise control!
#ifdef DEBUGGING_THROTTLE
    Serial.print("CC: last = ");
    Serial.print(lastThrottle);
    Serial.print(", ");
#endif
    throttle = lastThrottle;
    if (chuck.Y > 0.5 && throttle < 1.0) {
      throttle += THROTTLE_CC_BUMP;
    } else if (chuck.Y < -0.5 && throttle > -1.0) {
      throttle -= THROTTLE_CC_BUMP;
    } 
      
  } else {
    throttle = chuck.Y;

    if (abs(throttle) < MIN_THROTTLE) {
      throttle = 0;
    }
  }

#ifdef DEBUGGING_THROTTLE
    Serial.print("setting ");
    Serial.println(throttle);
#endif
  
  lastThrottle = throttle;
  return smoother.compute(throttle);
} // float getThrottle()



void handleInactivity() {
#ifdef DEBUGGING
  Serial.print(millis());
  Serial.println(": handling inactivity");
#endif
  lastThrottle = 0; // kills cruise control
  smoother.zero();  // kills throttle history
  ESC.setLevel(0);
  do {    
    freakOut();
    if (! chuck.isActive()) {
      // stopChuck();
      // delay(250);
      startChuck();
    }
  } while (! chuck.isActive());
  
  // active -- now wait for zero
#ifdef DEBUGGING
  Serial.print(millis());
  Serial.println("Waiting for 0");
#endif  
  while (chuck.Y > 0.1 || chuck.Y < -0.1) {
    chuck.update();
    delay(20);
  }
  
#ifdef DEBUGGING
  Serial.print(millis());
  Serial.println(": finished inactivity -- chuck is active");
#endif  

} // handleInactivity()


void setup() {
  Serial.begin(115200);
#ifdef DEBUGGING
  Serial.print("Wiiceiver (");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.print(__TIME__);
  Serial.println(") starting");
#endif

  green.init();
  red.init();

  setup_pins();
  splashScreen();
  
  delay(5000); // hold for nunchuck powerup

#ifdef DEBUGGING
  Serial.println("Starting the nunchuck ...");
#endif
  green.high();
  red.high();
  if (! startChuck()) {
    handleInactivity();
  }
#ifdef DEBUGGING
  Serial.println("Nunchuck is active!");
#endif

  green.start(10);
  red.start(10);
  
  green.update(1);
  red.update(1);
  
  ESC.init(ESC_PPM);
} // void setup()



void loop() {
  static float lastThrottle = 0;
  unsigned long startMS = millis();
  green.run();
  red.run();
  chuck.update();
  
  if (!chuck.isActive()) {
#ifdef DEBUGGING
    Serial.println("INACTIVE!!");
#endif
    handleInactivity();
    // delay(100);
  } else {
    float throttle = getThrottle();
    if (throttle != lastThrottle) {
      updateLEDs(throttle);
      ESC.setLevel(throttle);
#ifdef DEBUGGING
      Serial.print("y=");
      Serial.print(chuck.Y, 4);
      Serial.print(", ");
      Serial.print("c=");
      Serial.print(chuck.C);      
      Serial.print(", z=");
      Serial.print(chuck.Z);
      Serial.print(", ");
      Serial.println(throttle, 4); 
#endif
      lastThrottle = throttle;
    }
    int delayMS = constrain(startMS + 20 - millis(), 5, 20);
    // Serial.print("sleeping "); Serial.println(delayMS);
    delay(delayMS);
  } // if (chuck.isActive())
}


