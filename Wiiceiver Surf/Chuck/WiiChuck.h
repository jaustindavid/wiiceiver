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
 
#ifndef WIICHUCK_H
#define WIICHUCK_H

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
#define DEFAULT_X_ZERO 128

private:
  byte lastStatus[6];
  byte Y0, Ymin, Ymax, X0, Xmin, Xmax;
  word lastActivity, activitySamenessCount;
  
public:
  byte status[6];
  float X, Y;
  bool C, Z;


private:

  bool all255s(void) {
    for (int i=0; i < 6; i++) {
      if (status[i] != 255) {
        return false;
      }
    }
    return true;
  } // bool all255s(void)


  bool statusChanged(void) {
    for (int i=0; i < 6; i++) {
      if (status[i] != lastStatus[i]) {
        return true;
      }
    }
    
    return false;
  } // bool statusChanged()
  
  
  void saveLastStatus(void) {
    for (int i=0; i < 6; i++) {
      lastStatus[i] = status[i];
    }
  } // void saveLastStatus()

  
  // tracks the max-observed deflection (high & low)
  void _selfCalibrateDeflection(void) {
    byte joyX = status[0];
    Xmin = min(joyX, Xmin);
    Xmax = max(joyX, Xmax);
 
    byte joyY = status[1];

    if (joyY < Ymin) {
      Ymin = joyY;
    }

    if (joyY > Ymax) {
      Ymax = joyY;
    }
  } // void _selfCalibrateDeflection(void)


  void _computeStatus(void) {
    byte joyX = status[0];
    byte joyY = status[1];
    _selfCalibrateDeflection();
    
    int centeredX = joyX - X0;
    if (centeredX == 0) {
      X = 0;
    } 
    else if (centeredX > 0) {
      X = 1.0 * centeredX / (Xmax - X0);
    } 
    else {
      X = -1.0 * centeredX / (Xmin - X0); 
    }
    
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


    if (!statusChanged()) { 
      if (activitySamenessCount < WII_ACTIVITY_COUNTER) {
        activitySamenessCount ++;
      }
    } else {
      activitySamenessCount = 0;
      saveLastStatus();
    }
    
    #ifdef DEBUGGING_CHUCK_ACTIVITY
      Serial.print(F("CHUCK: "));
      for (int i = 0; i <= 5; i++) {
        Serial.print(F(" ["));
        Serial.print(status[i], DEC);
        Serial.print(F("]"));
      }
      Serial.print(F("; sameness "));
      Serial.print(activitySamenessCount);    
      Serial.println();
    #endif


  } // _computeStatus(void)


public:


  void calibrateCenter() {
    Y0 = status[1];
  } // calibrateCenter()


  // setup the nunchuck -- send the initialization sequence & start reading data
  void setup(void) {
    X0 = Y0 = 128;
    Xmin = Ymin = 15;
    Xmax = Ymax = 200;

#ifdef DEBUGGING_CHUCK
    Serial.print(millis());
    Serial.print(F(": Chuck.setup() ..."));
#endif
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
#ifdef DEBUGGING_CHUCK
    Serial.print(F(" transmitted @ "));
    Serial.print(millis());
#endif

    // do enough updates to prime the activity checker ...
    for (int i = 0; i < WII_ACTIVITY_COUNTER; i++) {
      update();
      delay(1);
    }
    
 #ifdef DEBUGGING_CHUCK
    Serial.print(F("; setup complete @ "));
    Serial.println(millis());
#endif   
  } // void setup(void)


  // update the status[] fields from the nunchuck
  void update(void) {
    // TODO: estimate the actual delay required between the request 
    // & data available on bus
    // delay(1);

    // read 6 bytes
    Wire.requestFrom (0x52, 6); // request data from nunchuck
    int cnt = 0;
    while (Wire.available()) {
      status[cnt] = Wire.read();
      cnt++;
    }

    _computeStatus();
#ifdef DEBUGGING_CHUCK_ACTIVITY
   Serial.print(F("Active? "));
   Serial.println(isActive() ? F("yes") : F("no"));
#endif

    // send one 0 to initiate transfer
    Wire.beginTransmission(0x52); 
    Wire.write(0);         
    Wire.endTransmission();

  } // void update(void)


  // is the controller "active" -- being held by a human & reporting
  // changing values?
  bool isActive(void) {
    return activitySamenessCount < WII_ACTIVITY_COUNTER && ! all255s();
  } // bool isActive(void)

};

#endif
