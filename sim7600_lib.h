/* sim7600_lib header */
#ifndef SIM7600_LIB_H
#define SIM7600_LIB_H
#define DEBUG false

// constants for module status
#define INACTIVE 0
#define ACTIVE 1

// constants for sensor status.
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

// constants for user responses.
#define NO 0
#define YES 1
#define NOREPLY 2
inline int reply;

// constants for configured modes.
#define CONFIG_NONE 0
#define CONFIG_SAVED 1
#define CONFIG_PUSHED 2

#include <string.h>
#include <stdio.h>
#include <Arduino.h>
#include <ctype.h>

// MISC. variables
inline String phoneNum = "+12269357857";
inline const int MESSAGE_WAIT_TIMEOUT = 60000;
inline char buffer[64];                                  //For notifications
inline boolean POOR_CONNECTION = false;
inline boolean acknowledge = false;

// struct for unit data storage
  struct Sensor{
    byte ID = 0;
    byte status = INACTIVE;
    byte state = NullState;
    byte tilt = OFF;
    byte light = OFF;
    byte conductivity = OFF;
    byte BatLevel = 0;
    String name = "";
    String datetime = "";
    int configured = 0;
    unsigned long int timeout = 0;
    char address[6];
  };
inline Sensor device[4];

String readSMS(const int timeout, int slot);
String updateSMS(int mode);
String sendCMD(String cmd, const int timeout);
String getDateTime();
String CurrConfig(Sensor device);
String CurrState(Sensor device);
Sensor ReqCommand(String cmd, Sensor device);
Sensor AlarmOn(Sensor device);
Sensor Disarm(Sensor device);
Sensor ChangeConfig(Sensor device);
Sensor storeStatus(Sensor device);
boolean checkSMS(String message, int slot, boolean debug);
boolean sendSMS(String message);
int getID(String message);
int getYN(unsigned long int time);
void clearSMS();
void Status(Sensor device);
void Help(boolean debug);
void Alarm(Sensor device);
void setDeviceID();
void setDeviceAddress();


#endif /* SIM7600_LIB_H */