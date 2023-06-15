/* sim7600_lib header */
#ifndef SIM7600_LIB_H
#define SIM7600_LIB_H
#define DEBUG true

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

#include <string.h>
#include <stdio.h>
#include <Arduino.h>

// MISC. variables
inline String phoneNum = "+12269357857";
inline const int MESSAGE_WAIT_TIMEOUT = 60000;
inline String cmd = "AT+CMGS=\"" + phoneNum + "\"";
inline char buffer[64];                                  //For notifications
inline boolean POOR_CONNECTION = false;

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
    char address[6] = "00000";
  };
inline Sensor device[4];

String readSMS(const int timeout, int slot);
String updateSMS(int mode);
String sendCMD(String cmd, const int timeout, boolean debug);
String getYN(int time);
String getDateTime();
String CurrConfig(Sensor device, boolean debug);
Sensor ReqCommand(String cmd, Sensor device, boolean debug);
Sensor AlarmOn(Sensor device);
Sensor Disarm(Sensor device);
Sensor ChangeConfig(Sensor device, boolean debug);
Sensor storeStatus(Sensor device);
boolean checkSMS(String message, int slot, boolean debug);
boolean sendSMS(String message);
int getID(String message, boolean debug);
void clearSMS(boolean debug);
void Status(Sensor device, boolean debug);
void Help(boolean debug);
void Alarm(Sensor device);
void Example(String message, boolean debug);
void setDeviceID();
void setDeviceAddress();
// boolean getReadyInstall(Sensor device, boolean debug);


#endif /* SIM7600_LIB_H */