#include <Arduino.h>
#include <Wire.h>

#include "elapsedMillis.h";
#include "Blinker.h"

// setting aside some EEPROM blocks 
#define EEPROM_TXRX 0
#define EEPROM_CHUCK 10

#define DEBUGGING

#define DEBUGGING_CHUCK
// #define DEBUGGING_CHUCK_ACTIVITY
#define WII_ACTIVITY_COUNTER 100  // once per 20ms; 50 per second
#include "FakeChuck.h"

/*-----( Import needed libraries )-----*/
// SEE http://arduino-info.wikispaces.com/Arduino-Libraries  !!
// NEED the RadioHead Library installed!
// http://www.airspayce.com/mikem/arduino/RadioHead/RadioHead-1.23.zip
#include <RHReliableDatagram.h>
#include <RH_NRF24.h>
#include <SPI.h>


#define CLIENT_ADDRESS 1      // For Radio Link
#define SERVER_ADDRESS 2

// Create an instance of the radio driver
RH_NRF24 RadioDriver;

// Create an instance of a manager object to manage message delivery and receipt, using the driver declared above
RHReliableDatagram RadioManager(RadioDriver, CLIENT_ADDRESS);// sets the driver to NRF24 and the client adress to 1


#define TXMIT_INTERVAL 5 // ms

#define DEBUGGING_TXRX
#define TESTING_TXRX
#include "TXRX.h"


Chuck chuck;
TXRX txrx;

void setup() {
  Serial.begin(115200);
  txrx.init();
  chuck.setup();
  pinMode(3, OUTPUT);
  digitalWrite(3, LOW);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, OUTPUT);
  
  // NOTE: pinMode for Radio pins handled by RadioDriver
  if (!RadioManager.init()) {  // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    Serial.println("init failed");
    // TODO: self-reset or somehow crash & burn
  }
  /*
   * RH_NRF24::DataRate2Mbps
   * RH_NRF24::DataRate1Mbps
   * RH_NRF24::DataRate250kbps
   * 
   * TransmitPowerm18dBm
   * TransmitPowerm12dBm
   * TransmitPowerm6dBm
   * TransmitPower0dBm
   */
  // RadioDriver.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm);
  RadioDriver.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm); 
} // setup()


elapsedMillis lastStatus = 0;
void loop() {
  elapsedMillis timeElapsed = 0;
  chuck.update();
  checkBattery(5);

  if (lastStatus > 1000) {
    Serial.print(millis());
    Serial.print(F(": "));
    Serial.print(F("y="));
    Serial.print(chuck.Y, 4);
    Serial.print(F(", "));
    Serial.print(F("c="));
    Serial.print(chuck.C);      
    Serial.print(F(", z="));
    Serial.println(chuck.Z);
    lastStatus = 0;
  }
  
  if (! RadioManager.sendto(chuck.status, sizeof(chuck.status), SERVER_ADDRESS)) {
    Serial.println(F("RadioManager.sendto failed"));
  }

  byte suggested_delay = constrain(TXMIT_INTERVAL - timeElapsed, 0, 5);
  if (suggested_delay > 0) {
    #ifdef DEBUGGING
      Serial.print("Delay: ");
      Serial.println(suggested_delay);
    #endif
    delay(suggested_delay);
  } else {
    #ifdef DEBUGGING
      Serial.print("Time elapsed (ms): ");
      Serial.println(timeElapsed);
    #endif
  }
}


void checkBattery(byte pin) {
  float voltage = 3.3 * analogRead(A1) / 1024;
  if (lastStatus > 1000) {
    Serial.print("Voltage: ");
    Serial.println(voltage);
  }
  if (voltage < 3.2) {
    digitalWrite(pin, HIGH);
  } else {
    breathe(pin);
  }
}

void breathe(byte pin) {
  static int level = 0;
  static int direction = 1;
  static elapsedMillis breathTimer = 0;
  static bool waiting = false;

  if (waiting) {
    if (breathTimer >= 3000) { // ms between breaths
      waiting = false;
    }
  } else {
    level += direction;
    if (level >= 255) {
      direction = -1;
    } else if (level <= 0) {
      direction = 1;
      waiting = true;
      breathTimer = 0;
    }
    analogWrite(pin, level);
  }
}

