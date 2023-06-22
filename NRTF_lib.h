/* NRTF_lib header */
#ifndef NRTF_LIB_H
#define NRTF_LIB_H
#include <string.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#define DEBUG true

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

// SPI stuff
inline static SPIClassSAMD rfSPI (&sercom1, 28, 26, 25, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
// Function Edge_Pin Port SAMD21G_pin Pad
// MISO D12 PA19 28 Pad3
// MOSI D11 PA16 25 Pad0
// SCK D13 PA17 26 Pad1

// radio stuff
#define CE_PIN 3
#define CSN_PIN 4
inline RF24 radio(CE_PIN, CSN_PIN);


inline const byte address[6] = "00001";

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

boolean getPayload(const byte address[]);
boolean sendPayload(const byte address[]);
boolean pingRF(const byte address[]);
boolean configureRadio(const byte address[]);
boolean PushConfig(Sensor device);
void loadPayload(Sensor device, byte message_type);
void DumpDatabase(Sensor device);


#endif  /* NRTF_LIB_H */