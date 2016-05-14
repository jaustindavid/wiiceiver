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
 * latest software & hardware: https://github.com/jaustindavid/wiiceiver
 *
 * Enjoy!  Be safe! 
 * 
 * (CC BY-NC-SA 4.0) Austin David, austin@austindavid.com
 * 12 May 2014
 * 02 Apr 2016
 *
 */

#ifndef TXRX_H
#define TXRX_H

// nrf24_reliable_datagram_client.pde
#define CLIENT_ADDRESS 1
#define SERVER_ADDRESS 2

  // Singleton instance of the radio driver
  RH_NRF24 driver(9,10);
  
  // Class to manage message delivery and receipt, using the driver declared above
  RHReliableDatagram manager(driver, SERVER_ADDRESS);
 
  struct StatusMessage_t {
    float chuckX, chuckY;    // [-1 .. 1]
    bool buttons[8];         // chuck C, chuck Z, future TBD
    float batteryLevel;      // [0 .. 1]
  };

  typedef union StatusPacket_t {
    StatusMessage_t message;
    byte bytes[sizeof(StatusMessage_t)];
  };
  
  void setup_txmitter() {
    if (!manager.init()) {
      Serial.println("init failed");
      // TODO: crash/burn
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
    // driver.setRF(RH_NRF24::DataRate2Mbps, RH_NRF24::TransmitPower0dBm);
    driver.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPowerm18dBm); 
  } // setup_txmitter()

  

#endif
