#include <TinyWireM.h>                  // I2C Master lib for ATTinys which use USI
#include <Blinky.h>
#include <SoftwareServo.h> 

/*
       assignments from http://highlowtech.org/?p=1695

                                +---\/---+
                            VCC | 1   14 | GND
                             10 | 2   13 | 0 (Analog input 0, AREF)
                              9 | 3   12 | 1 (Analog input 1)
                          RESET | 4   11 | 2 (Analog input 2)
                        (PWM) 8 | 5   10 | 3 (Analog input 3)
        (PWM, Analog input 7) 7 | 6    9 | 4 (Analog input 4, SCK)
  (MOSI, PWM, Analog input 6) 6 | 7    8 | 5 (Analog input 5, PWM, MISO)
                                +--------+
Notes:
   ISP uses RESET, MOSI, MISO, SCK for Uno pins 10-13
   I2C uses SDA==MOSI, SCL==SCK
   
   For ISC, add 2k pullups for SCA, SCL

 */

#define RED_PIN 0
#define GREEN_PIN 1
#define ESC_PIN 8
// #define WII_SCL 4 // TinyWire uses SCK  
// #define WII_SDA 6 // TinyWire uses MOSI
#define WII_POWER_PIN 10

/*
 *    ESC wrapper class
 *
 *    To initialize an ESC:
 *       write 90
 *       wait 3s
 *
 *    Tested on exactly 2 ESCs, flawlessly
 */ 

class ElectronicSpeedControl {
#define ESC_CENTER 90       // angle of the "center"; probably always 90
#define ESC_MAX_ANGLE 135   // angle of "max" deflection

public:
    void init(int pin);
    void setLevel(float level);
    void sleep(int ms);
  private:
    SoftwareServo _esc;
};
  
  
  void ElectronicSpeedControl::init(int pin) {
    // s.print("attaching to pin");
    // Serial.println(pin);
    _esc.attach(pin);

    // Serial.println("initializing ESC...");
    
    _esc.write(90);
    this->sleep(3000);
    // Serial.println("done");
  }
  

  // input: -1 .. 1
  // output: 0 .. 180
  void ElectronicSpeedControl::setLevel(float level) {
    int angle = (int)(ESC_CENTER + (ESC_MAX_ANGLE - ESC_CENTER) * level);
    // Serial.print("ESC angle: ");
    // Serial.println(angle);
    _esc.write(angle);
  }


  void ElectronicSpeedControl::sleep(int ms) {
    unsigned long targetMillis = millis() + ms;
    while (millis() < targetMillis) {
      SoftwareServo::refresh();
      delay(20);
    }
  }


/*
 *  A "tiny" Wii Nunchuck class
 *  Borrows heavily from:
 *
 *   Nunchuck -- Use a Wii Nunchuck
 *   Tim Hirzel http://www.growdown.com
 * who cites:
 *
 * * This file is an adaptation of the code by these authors:
 * Tod E. Kurt, http://todbot.com/blog/
 *
 * The Wii Nunchuck reading code is taken from Windmeadow Labs
 * http://www.windmeadow.com/node/42
 *
 * Conversion to Arduino 1.0 by Danjovic
 * http://hotbit.blogspot.com
 *
 * Included the Fix from Martin Peris by Leopold Klimesch
 * http://blog.martinperis.com/2011/04/arduino-wiichuck.html
 *
 *
 * Taking all of the above into account, this class is heavily adapted
 * for the specific usage here: a single-channel (~1.5), activity-sensitive 
 * controller for an electric skateboard.  I've deliberately removed or not used
 * a fair amount of the data available from the 'chuck, in the interest of 
 * keeping memory down for the ATtiny series.
 *
 */
class TinyChuck {
#define WII_ACTIVITY_COUNTER 50   // consecutive static reads & it's considered inactive
  private:
    byte status[6];
    byte Y0, Ymin, Ymax;
    int lastActivity, activitySamenessCount;
  public:
    float X, Y;
    bool C, Z;
    

  private:
    // tracks the max-observed deflection (high & low)
    void _selfCalibrateDeflection(void) {
      byte joyY = status[1];
      
      if (joyY < Ymin) {
        Ymin = joyY;
      }
      
      if (joyY > Ymax) {
        Ymax = joyY;
      }
    } // void _selfCalibrateDeflection(void)


    void _computeStatus(void) {
      byte joyY = status[1];
      _selfCalibrateDeflection();
      int centeredY = joyY - Y0;
      if (centeredY == 0) {
        Y = 0;
      } else if (centeredY > 0) {
        Y = 1.0 * centeredY / (Ymax - Y0);
      } else {
        Y = -1.0 * centeredY / (Ymin - Y0); 
      }
      
      C = ((status[5] & B00000010) >> 1) == 0;
      Z = (status[5] & B00000001) == 0;
      
      // maintain the bits of the accelerometer axes & both buttons
      // as a proxy for "the controller is still moving"
      // awesomely, these are held in the last status byte
      
      if (status[5] == lastActivity) {
        activitySamenessCount ++;
      } else {
        activitySamenessCount = 0;
      }
      lastActivity = status[5];
    }


    void calibrateCenter() {
      Y0 = status[1];
    }
    
    
  public:
  
    void setup(void) {
      Y0 = 128;
      Ymin = 15;
      Ymax = 200;
      
      TinyWireM.begin();
      TinyWireM.beginTransmission(0x52);       // device address
      TinyWireM.send(0xF0);
      TinyWireM.send(0x55);
      TinyWireM.endTransmission();
      delay(1);
      TinyWireM.beginTransmission(0x52);
      TinyWireM.send(0xFB);
      TinyWireM.send((uint8_t)0x00);
      TinyWireM.endTransmission();

      // do enough updates to prime the activity checker ...
      for (int i = 0; i < WII_ACTIVITY_COUNTER; i++) {
        update();
        delay(1);
      }
      // calibrateCenter();
    } // void setup(void)
    
    
    void update(void) {

      // TODO: estimate the actual delay required between the request 
      // & data available on bus
      // delay(1);

      // read 6 bytes
      TinyWireM.requestFrom (0x52, 6); // request data from nunchuck
      int cnt = 0;
      while (TinyWireM.available ()) {
        status[cnt] = TinyWireM.receive();
        cnt++;
      }
      
      _computeStatus();
      
      // send one 0 to initiate transfer
      TinyWireM.beginTransmission(0x52); 
      TinyWireM.send(0);         
      TinyWireM.endTransmission();

    } // void update(void)


    // is the controller "active" -- being held by a human & reporting
    // changing accelerometer values?
    bool isActive(void) {
      return activitySamenessCount < WII_ACTIVITY_COUNTER;
    } // bool isActive(void)
    
};


Blinky red = Blinky(RED_PIN);
Blinky green = Blinky(GREEN_PIN);
TinyChuck chuck;
ElectronicSpeedControl esc;


void sleep(int ms) {
  unsigned long target = millis() + ms;
  while(millis() < target) {
    red.run();
    green.run();
    SoftwareServo::refresh();
    delay(5);
  }
} // void sleep(int ms)


void updateLEDs(float throttle) {
  int magnitude = abs((int)15*throttle);
  
  if (throttle == 0) {
    red.update(1);
    green.update(1);
  } else if (throttle > 0) {
    green.update(magnitude);
    red.update(1);
  } else {
    green.update(1);
    red.update(magnitude);
  }
} // void updateLEDs



// the nunchuck appears to be static: we lost connection!
// go "dead" for 5s, but keep checking the chuck to see if
// it comes back
void freakOut(void) {
  bool redOn = false;
  byte blinkCtr = 0;
  
  while (!chuck.isActive()) {
    if (blinkCtr >= 4) {
      blinkCtr = 0;
      if (redOn) {
        red.high();
        green.stop();
        redOn = false;
      } else {
        red.stop();
        green.high();
        redOn = true;
      }
    }
    blinkCtr ++;
    chuck.update();
    sleep(20);
  }
  green.start(1);
  red.start(1);
} // void freakOut(void)


// DEAD CODE
// initialize the controller
// will continually power-cycle until it gets changing readings 
// from the accelerometer or buttons
void initializeController(void) {
  pinMode(WII_POWER_PIN, OUTPUT);
  
  do {
    // power-cycle
    digitalWrite(WII_POWER_PIN, LOW);
    sleep(100);
    digitalWrite(WII_POWER_PIN, HIGH);
    chuck.setup();
    
    // check it for up to 1s
    for (int i = 0; i < 50 && !chuck.isActive(); i++) {
      sleep(20);
      chuck.update();
    }
  } while (! chuck.isActive());
} // void initializeController(void)



void setup() {
  // start the LEDs
  green.init();
  red.init();

  chuck.setup();  
  // initializeController();

  // initialize the ESC
  green.high();
  red.high();
  esc.init(ESC_PIN);
  
  // proceed to normal operation
  red.start(1);
  green.start(1);
}


void loop() {
/*
  chuck.update();
  if (chuck.isActive()) {
    chuck.C ? red.high() : red.low();
    chuck.Z ? green.high() : green.low();
    sleep(20);
  } else {
    while (!chuck.isActive()) {
      freakOut();
    }
  }
  return;
*/

  chuck.update();
  if (chuck.isActive()) {
    float throttle = chuck.Y * (chuck.Z ? 1.0 : 0.5);
    esc.setLevel(throttle);
    updateLEDs(throttle);
  } else {
    esc.setLevel(0);
    freakOut();
  }
  sleep(20);
}

