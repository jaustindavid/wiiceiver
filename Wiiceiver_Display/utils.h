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


#ifndef UTILS_H
#define UTILS_H

#include <avr/eeprom.h>

// #define DEBUGGING_TIMER

class Timer {
  private:
    unsigned long startMS;
    int stopMS;

  public:
    Timer(int newStopMS) {
      reset(newStopMS);
    } // Constructor
    
    
   void reset(void) {
     startMS = millis();
   } // reset()
   
   
   void reset(int newStopMS) {
      stopMS = newStopMS;
      reset();
   }
   
   bool isExpired(void) {
     #ifdef DEBUGGING_TIMER
     Serial.print(F("Expired? "));
     Serial.print(startMS);
     Serial.print(F("+"));
     Serial.print(stopMS);
     Serial.print(F(" <> now@"));
     Serial.println(millis());
     #endif
     return millis() >= startMS + stopMS;
   } // bool isExpired()
   
};


/********************
 *
 * fuel gauge persistence stuff
 *
 ********************/

#define EEPROM_DISCHARGE_HEAD 16


int _block2addy(int block) {
  return EEPROM_DISCHARGE_HEAD + sizeof(int) * block;
} // int _block2addy(block)



static int NR_BLOCKS = (1023 - (EEPROM_DISCHARGE_HEAD + sizeof(int))) 
                      / sizeof(int);

int _findUnusedBlock(void) {
  int block = 0;
  while (block <= NR_BLOCKS && 
         EEPROM.read(_block2addy(block)) != 255) {
    block++;
  }
  
  if (block > NR_BLOCKS) {
    block = 0;
  }
  
  return block;
} // _findUnusedBlock()


// reads the total net discharge saved in flash
int readTotalDischarge(void) {
  int block = _findUnusedBlock();
  if (block == 0) {
    block = NR_BLOCKS;
  } else {
    block --;
  }
  Serial.print("[found block #");
  Serial.print(block);
  Serial.print("] ");
  int discharge = eeprom_read_word((word *)_block2addy(block));
  
  if (discharge < 0) {
    discharge = 0;
  } 
  
  return discharge;
} // int readTotalDischarge()


// saves the new total net discharge to flash
int saveTotalDischarge(int discharge) {
  int block = _findUnusedBlock();
  eeprom_write_word((word *)_block2addy(block), discharge);
  Serial.print("wrote to block #");
  Serial.println(block);

  block ++;
  if (block > NR_BLOCKS) {
    block = 0;
  }
  
  EEPROM.write(_block2addy(block), 255);
  return block;
} // saveTotalDischarge(discharge)


#endif
