/* sim7600_lib header */
#ifndef SIM7600_LIB_H
#define SIM7600_LIB_H
#define DEBUG true

// constants for module status and sensor status. 
#define OFF 0
#define ON 1
#define TRIGGERED 2
#define FAILED 3
#define INACTIVE 0
#define ACTIVE 1

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

// struct for unit data storage
  struct Sensor{
    int ID = 0;
    String name = "";
    String status = "INACTIVE"; // CHANGE SOON
    int state = NullState;
    String datetime = "";
    int tilt = OFF;
    int light = OFF;
    int conductivity = OFF;
    char RFaddress[6] = "00000";
    int configured = 0;
    int BatLevel = 0;
  };
inline Sensor device[8];

String readSMS(const int timeout, int slot, boolean debug);
String updateSMS(int mode, boolean debug);
String sendCMD(String cmd, const int timeout, boolean debug);
String getYN(int time, boolean debug);
String getDateTime(boolean debug);
String CurrConfig(Sensor device, boolean debug);
Sensor ReqCommand(String cmd, Sensor device, boolean debug);
Sensor AlarmOn(Sensor device, boolean debug);
Sensor Disarm(Sensor device, boolean debug);
Sensor ChangeConfig(Sensor device, boolean debug);
Sensor storeStatus(Sensor device);
boolean checkSMS(String message, int slot, boolean debug);
boolean sendSMS(String message);
int getID(String message, boolean debug);
void clearSMS(boolean debug);
void Status(Sensor device, boolean debug);
void Help(boolean debug);
void Alarm(Sensor device, boolean debug);
void Example(String message, boolean debug);
void setDeviceID(boolean debug);
// boolean getReadyInstall(Sensor device, boolean debug);


#endif /* SIM7600_LIB_H */