#ifndef CHUCK_H
#define CHUCK_H

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
class Chuck {
#ifndef WII_ACTIVITY_COUNTER
#define WII_ACTIVITY_COUNTER 250  // consecutive static reads & it's considered inactive
#endif
#define DEFAULT_Y_ZERO 128

private:
  byte status[6];
  byte Y0, Ymin, Ymax;
  word lastActivity, activitySamenessCount;
public:
  float X, Y;
  bool C, Z;


private:

  /*
   * maintain the bits of the accelerometer axes & both buttons
   * as a proxy for "the controller is still moving"
   * awesomely, these are held in the last status byte
   * ... but memorex only uses an 8-bit accelerometer, so we 
   * add more bits from the other axes
   */
  word getActivity() {
    word activity;
    activity = status[0] & B01;
#ifdef DEBUGGING_CHUCK_ACTIVITY
    Serial.print("get activity: ");
    Serial.print(activity, BIN);
#endif

    activity <<= 1;
    activity += status[1] & B01;
#ifdef DEBUGGING_CHUCK_ACTIVITY
    Serial.print(" ");
    Serial.print(activity, BIN);
#endif

    for (int i = 2; i < 5; i++) {
      activity <<= 2;
      activity += status[i] & B011;
#ifdef DEBUGGING_CHUCK_ACTIVITY
    Serial.print(" ");
    Serial.print(activity, BIN);
#endif    

    }
    activity = word(activity, status[5]);
#ifdef DEBUGGING_CHUCK_ACTIVITY
    Serial.print(" : ");
    Serial.println(activity, BIN);
#endif
    return activity;
  } // word getActivityWord()

  
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
    } 
    else if (centeredY > 0) {
      Y = 1.0 * centeredY / (Ymax - Y0);
    } 
    else {
      Y = -1.0 * centeredY / (Ymin - Y0); 
    }

    C = ((status[5] & B00000010) >> 1) == 0;
    Z = (status[5] & B00000001) == 0;


    word activity = getActivity();
    if (activity == lastActivity) { 
      if (activitySamenessCount < WII_ACTIVITY_COUNTER) {
        activitySamenessCount ++;
      }
    } else {
      activitySamenessCount = 0;
      lastActivity = activity;
    }
    
#ifdef DEBUGGING_CHUCK
    Serial.print("CHUCK: ");
    for (int i = 0; i < 5; i++) {
      Serial.print(status[i], DEC);
      Serial.print(" ");
    }
    Serial.print(status[5], BIN);
    Serial.print("; sameness ");
    Serial.print(activitySamenessCount);    
    Serial.println();
#endif


  } // _computeStatus(void)


public:

  void readEEPROM() {
    byte storedY;

    storedY = EEPROM.read(0);
#ifdef DEBUGGING
    Serial.print(F("Reading stored value: Y="));
    Serial.println(storedY);
#endif

    // sanity check: they shouldn't differ by more than 25 units (~10%)
    if (abs(storedY - DEFAULT_Y_ZERO) <= 25) {
      Y0 = storedY;
#ifdef DEBUGGING
      Serial.println("Using stored value");
#endif
} 
    else {
#ifdef DEBUGGING
      Serial.println("Ingoring stored value");
#endif
    }
  } // readEEPROM()


  void writeEEPROM() {
    EEPROM.write(0, Y0);
#ifdef DEBUGGING
    Serial.print("Storing value: Y=");
    Serial.println(Y0);	
#endif	    
  } // writeEEPROM()


  void calibrateCenter() {
    Y0 = status[1];
  } // calibrateCenter()


  void setup(void) {
    Y0 = 128;
    Ymin = 15;
    Ymax = 200;

    Wire.begin();
    Wire.beginTransmission(0x52);       // device address
    Wire.write(0xF0);
    Wire.write(0x55);
    Wire.endTransmission();
    delay(1);
    Wire.beginTransmission(0x52);
    Wire.write(0xFB);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission();

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
    Wire.requestFrom (0x52, 6); // request data from nunchuck
    int cnt = 0;
    while (Wire.available ()) {
      status[cnt] = Wire.read();
      cnt++;
    }

    _computeStatus();
#ifdef DEBUGGING_CHUCK
   Serial.print("Active? ");
   Serial.println(isActive() ? "yes" : "no");
#endif

    // send one 0 to initiate transfer
    Wire.beginTransmission(0x52); 
    Wire.write(0);         
    Wire.endTransmission();

  } // void update(void)


  // is the controller "active" -- being held by a human & reporting
  // changing accelerometer values?
  bool isActive(void) {
    return activitySamenessCount < WII_ACTIVITY_COUNTER;
  } // bool isActive(void)

};

#endif
