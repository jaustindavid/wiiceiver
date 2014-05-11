#include <Wire.h>
#include <Servo.h>
#include <EEPROM.h>


#define DEBUGGING

#include "Blinker.h"


// #define DEBUGGING_CHUCK
// #define DEBUGGING_CHUCK_ACTIVITY
#define WII_ACTIVITY_COUNTER 100  // once per 20ms; 50 per second
#include "Chuck.h"

#define DEBUGGING_ESC
#include "ElectronicSpeedController.h"

// #define DEBUGGING_SMOOTHER
#include "Smoother.h"

// #define DEBUGGING_THROTTLE
#define THROTTLE_MIN 0.05        // the lowest throttle to send the ESC
#define THROTTLE_CC_BUMP 0.002   // CC = 0.1% throttle increase; 50/s = 10s to hit 100% on cruise
#define THROTTLE_SMOOTHNESS 0.05  // default "smoothing" factor
#define THROTTLE_MIN_CC 0.45     // minimum / inital speed for cruise crontrol
#include "Throttle.h"


#include "pinouts.h"


Chuck chuck;
ElectronicSpeedController ESC;
Blinker green, red;
Throttle throttle;




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
  digitalWrite(pinLocation(GREEN_LED_ID), HIGH);
  digitalWrite(pinLocation(RED_LED_ID), HIGH);
  delay(500);
  for (i = 0; i < 10; i++) {
    digitalWrite(pinLocation(GREEN_LED_ID), HIGH);
    digitalWrite(pinLocation(RED_LED_ID), LOW);
    delay(50);
    digitalWrite(pinLocation(GREEN_LED_ID), LOW);
    digitalWrite(pinLocation(RED_LED_ID), HIGH);
    delay(50);
  }
  digitalWrite(pinLocation(GREEN_LED_ID), HIGH);
  digitalWrite(pinLocation(RED_LED_ID), HIGH);
  delay(500);
  digitalWrite(pinLocation(GREEN_LED_ID), LOW);    
  digitalWrite(pinLocation(RED_LED_ID), LOW);  
} // void splashScreen(void)


// flash the LEDs to indicate throttle position
void updateLEDs(Throttle throttle) {
  if (throttle.getThrottle() == 0) {
    green.update(1);
    red.update(1);
  } else {
    int bps = abs(int(throttle.getThrottle() * 20));

    if (throttle.getThrottle() > 0) {
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
  pinMode(pinLocation(WII_POWER_ID), OUTPUT);
  digitalWrite(pinLocation(WII_POWER_ID), HIGH);
} // setup_pins()


/*
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
*/


// returns true if the chuck appears "active"
bool startChuck() {
  int tries = 0;
  
  while (tries < 5) {
#ifdef DEBUGGING
    Serial.println("(Re)starting the nunchuck");
#endif
    chuck.setup();
    chuck.readEEPROM();
    tries ++;
    if (chuck.isActive()) {
      return true;
    }
  }
  return false;
} // startChuck()



void handleInactivity() {
#ifdef DEBUGGING
  Serial.print(millis());
  Serial.println(": handling inactivity");
#endif
  // lastThrottle = 0; // kills cruise control
  // smoother.zero();  // kills throttle history
  throttle.zero();
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

  green.init(pinLocation(GREEN_LED_ID));
  red.init(pinLocation(RED_LED_ID));

  setup_pins();
  ESC.init(pinLocation(ESC_PPM_ID));
  
  splashScreen();

  delay(5000); // hold for nunchuck powerup

#ifdef DEBUGGING
  Serial.println("Starting the nunchuck ...");
#endif
  green.high();
  red.high();
  if (startChuck()) {
    maybeCalibrate();
  } else {
    handleInactivity();
  }
#ifdef DEBUGGING
  Serial.println("Nunchuck is active!");
#endif

  green.start(10);
  red.start(10);
  
  green.update(1);
  red.update(1);
} // void setup()



void loop() {
  static float lastThrottleValue = 0;
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
    float throttleValue = throttle.update(chuck);
    ESC.setLevel(throttleValue);
    if (throttleValue != lastThrottleValue) {
      updateLEDs(throttle);
#ifdef DEBUGGING
      Serial.print("y=");
      Serial.print(chuck.Y, 4);
      Serial.print(", ");
      Serial.print("c=");
      Serial.print(chuck.C);      
      Serial.print(", z=");
      Serial.print(chuck.Z);
      Serial.print(", ");
      Serial.println(throttleValue, 4); 
#endif
      lastThrottleValue = throttleValue;
    }
    int delayMS = constrain(startMS + 20 - millis(), 5, 20);
    // Serial.print("sleeping "); Serial.println(delayMS);
    delay(delayMS);
  } // if (chuck.isActive())
}


