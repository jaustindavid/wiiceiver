#include <Arduino.h>
#include <Wire.h>

#include "elapsedMillis.h";


#define TXMIT_INTERVAL 5 // ms

// #define DEBUGGING_CHUCK
// #define DEBUGGING_CHUCK_ACTIVITY
#define WII_ACTIVITY_COUNTER 100  // once per 20ms; 50 per second
#include "Chuck.h"

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

Chuck chuck;

void setup() {
  Serial.begin(115200);
  chuck.setup();

  // NOTE: pinMode for Radio pins handled by RadioDriver
  if (!RadioManager.init())   // Defaults after init are 2.402 GHz (channel 2), 2Mbps, 0dBm
    Serial.println("init failed");
} // setup()


void loop() {
  elapsedMillis timeElapsed = 0;
  chuck.update();
  
      Serial.print(millis());
      Serial.print(F(": "));
      Serial.print(F("y="));
      Serial.print(chuck.Y, 4);
      Serial.print(F(", "));
      Serial.print(F("c="));
      Serial.print(chuck.C);      
      Serial.print(F(", z="));
      Serial.println(chuck.Z);

  if (! RadioManager.sendto(chuck.status, sizeof(chuck.status), SERVER_ADDRESS)) {
    Serial.println(F("RadioManager.sendto failed"));
  }

  if (timeElapsed < TXMIT_INTERVAL) {
    delay(TXMIT_INTERVAL - timeElapsed);
  }
}
