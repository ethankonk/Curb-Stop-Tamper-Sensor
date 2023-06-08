/* NRTF_lib header */
#ifndef NRTF_LIB_H
#define NRTF_LIB_H
#define DEBUG true

#include <string.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// radio stuff <----- IMPORTANT ----->
#define TRANSMIT 0;
#define LISTEN 1;
RF24 radio(7,8);

inline const byte address[6] = "00001";
inline boolean radioMode = TRANSMIT;                   // false: transmitting | true: listening

// NRF24L01 buffer limit is 32 bytes (max struct size)
struct payloadtype { 
  byte NodeID;                                    // Sensor node id from 3 pins (ID1,ID2,ID3)
  byte ControllerID;                              // Placeholder will be filled after first controller exchange
  byte Sensor1;                                   // sensor state: OK, Triggered, OFF
  byte Sensor2;
  byte Sensor3;
  byte MessageType;                               // determines action to be taken (Status, Request Parameters, Request command, Alarming, Armed)
  byte Mode;                                      // indicates what mode device is in during outgoing message. Uses same constants as "CurrentState"
  byte State;
  byte Future1;
  byte BatLevel;                                  // always provide current battery level to controller.
  word AlarmDelay;                                // indicates how long sensor is to wait before enabling alrm interrupts. provides an installation window time.
  word MessageCount;                              // will be incremented with each outgoing exchange. Primarily used for debugging.
}; 
inline payloadtype payload;
inline payloadtype Message;
inline unsigned long lastSignalMillis = 0;

payloadtype updateRF(const byte address[]);
boolean sendPacket(const byte address[], char msg_packet[])
void DumpDatabase();

#endif  /* NRTF_LIB_H */