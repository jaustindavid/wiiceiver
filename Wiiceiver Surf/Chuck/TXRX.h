/*
 * (CC BY-NC-SA 4.0) 
 * http://creativecommons.org/licenses/by-nc-sa/4.0/
 *
 * WARNING WARNING WARNING: attaching motors to a *board is 
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
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 20 Feb 2015
 *
 */

#ifndef TXRX_H
#define TXRX_H

#include <EEPROM.h>
#include <util/crc16.h>

#define EEPROM_TXRX_ADDRESS   10
#define EEPROM_TXRX_IDENTITY  11  // 4 bytes


/*
 * A wrapper class for transmit/receive operations
 * 
 * binding strategy: 
 *  1. make a random address
 *  2. listen on it for 3s, be sure nobody else is talking
 *  3. reliably send that address ("bind <addy><crc>") on channel 0, which is where receivers listen
 *  4. expect an ack from the correct address ("bound <addy+100><crc>")
 *  5. if checksums match, use the new address. 
 *  
 * identity:
 *   a 4-byte array: 3 random + checksum
 *   all 255s == wildcard (uninitialized)
 */

class TXRX {
  private:
    byte address;
    byte identity[4];
    
  public:
    void init() {
      #ifdef DEBUGGING_TXRX
        Serial.println("txrx initializationlolol");
      #endif

      #ifdef TESTING_TXRX
        test();
      #endif

      address = 1; 
      readEEPROM();

      #ifdef DEBUGGING_TXRX
        Serial.print("Address: ");
        Serial.print(address);
        Serial.print("; Identity: ");
        printIdentity();
        Serial.println();
      #endif        
    } // init()

    
    void test() {
      memset(identity, 0, sizeof(identity));
      Serial.print("Before: ");
      printIdentity();
      
      createNewIdentity();
      Serial.print("; after: ");
      printIdentity();
  
      if (verifyChecksum(identity, sizeof(identity)-1)) {
        Serial.print("; checksum (0x");
        Serial.print(calcChecksum(identity, sizeof(identity)-1), HEX);
        Serial.println(") matches!");
      } else {
        Serial.println("; checksum FAIL :(");
      }

      byte data[4] = { 1, 2, 3, 4 };
      byte buf[28];
      byte len = sizeof(data);
      memcpy(buf, data, len);
      memcpy(buf+len, identity, sizeof(identity));
      insertChecksum(buf, sizeof(data) + sizeof(identity));

      Serial.print("all: ");
      for (byte i = 0; i < sizeof(data) + sizeof(identity) + 1; i++) {
        Serial.print("0x");
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.print("cksum: 0x");
      Serial.print(calcChecksum(buf, sizeof(data) + sizeof(identity)), HEX);
      Serial.println();
    } // testTXRX()


// RadioManager.sendto(chuck.status, sizeof(chuck.status), SERVER_ADDRESS))
    // sends an array of bytes to an address
    bool send(uint8_t *data, uint8_t len, uint8_t dest) {
      uint8_t buf[28]; // RH_NRF24_MAX_MESSAGE_LEN];
      memcpy(buf, data, len);
      memcpy(buf+len, identity, sizeof(identity));
      
      insertChecksum(data, len + sizeof(identity));
      byte dataSize = len + sizeof(identity) + 1;
      bool result = RadioManager.sendto(data, dataSize, dest);

      return result;
    } // bool send(uint8_t *data, uint8_t len, uint8_t dest)
    
  private:


    void readEEPROM(void) {
      byte newAddy;

      newAddy = EEPROM.read(EEPROM_TXRX_ADDRESS);
      if (newAddy != 255) {
        // setAddress(newAddy);
      }

      for (byte i = 0; i < sizeof(identity); i++) {
        identity[i] = EEPROM.read(EEPROM_TXRX_IDENTITY + i);
      }
    } // readEEPROM()
    

    void writeEEPROM() {
      EEPROM.write(EEPROM_TXRX_ADDRESS, address);
      for (byte i = 0; i < sizeof(identity); i++) {
        EEPROM.write(EEPROM_TXRX_IDENTITY + i, identity[i]);
      }
    } // writeEEPROM()


    uint8_t calcChecksum(uint8_t* message, uint8_t messageLen) {
      uint8_t crc = 0, i;
      
      for (i = 0; i < messageLen; i++) {
        crc = _crc_ibutton_update(crc, message[i]);
      }
    
      return crc;
    } // uint8_t calcChecksum(uint8_t* message, byte messageLen) 
    
    
    uint8_t insertChecksum(uint8_t* message, uint8_t messageLen) {
      uint8_t crc = calcChecksum(message, messageLen);
      message[messageLen] = crc;
      return crc;
    } // uint8_t insertChecksum(uint8_t* message, uint8_t messageLen)
    
    
    bool verifyChecksum(uint8_t* message, uint8_t messageLen) {
      uint8_t crc = calcChecksum(message, messageLen);
      return crc == message[messageLen];
    } // bool verifyChecksum(uint8_t* message, uint8_t messageLen)
    
    
    // creates a new 4-byte identity
    void createNewIdentity() {
      randomSeed(millis());
    
      // first n-1 bytes are random ...
      for (byte i = 0; i < sizeof(identity) - 1; i++) {
        identity[i] = random(255);
      }
      // last byte is a checksum
      insertChecksum(identity, sizeof(identity) - 1);
    } // void createNewIdentity()
    
    
    void printIdentity() {
      for (byte i = 0; i < sizeof(identity); i++) {
        Serial.print("0x");
        Serial.print(identity[i], HEX);
        Serial.print(" ");
      }
    } // printIdentity()


    void setAddress() {
    }


}; // end class TXRX


#endif
