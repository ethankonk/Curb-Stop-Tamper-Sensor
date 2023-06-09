/* NRTF_lib header */
#ifndef NRTF_LIB_H
#define NRTF_LIB_H
#define DEBUG true

#include <string.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// constants for different sensor to controller message types.
#define STATUS 0
#define ReqParms 1
#define ReqCmd 2
#define ALARM 3
#define Armed 4     

// constants for different controller to sensor message types.
#define ReqStatus 0
#define LoadParms 1
#define GoToArm 2
#define GoSilent 3
#define GoToSleep 4
#define Acknowledge 5

// constants for module status and sensor status. 
#define OFF 0
#define ON 1
#define TRIGGERED 2
#define FAILED 3

// device state constants
#define NullState 0
#define Asleep 1
#define Connecting 2
#define WaitForCmd 3
#define Arming 4
#define Armed 5
#define Alarming 6
#define CommsFail 7
#define Configured 8
#define AmSilent 9
#define CantArm 10

// radio stuff
#define TRANSMIT 0
#define LISTEN 1
inline RF24 radio(7,8);

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

boolean updateRF(const byte address[]);
boolean sendPacket(char address[]);
boolean pingRF(char address[]);
void loadPacket(Sensor device);
void DumpDatabase();


#endif  /* NRTF_LIB_H */