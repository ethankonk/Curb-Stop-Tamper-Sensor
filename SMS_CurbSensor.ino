#include <string.h>
#include <stdio.h>
#include <time.h>
#include "sim7600_lib.h"
#include "NRTF_lib.h"

/*
Water Curb Box Sensor
- E. Konkolowicz May 2023
- send/receive messages from or too cellular device over SMS
- sends and receives RF transmits from multiple sensor devices
- hardware bult on Maduino SIM7600A chip
*/

// digital pin assignments
#define DTR_PIN 9
#define RI_PIN 8

#define LTE_PWRKEY_PIN 5
#define LTE_RESET_PIN 6
#define LTE_FLIGHT_PIN 7


// debug toggle
#define DEBUG true

// MISC. variables
String response = "";


void setup() {
  setDeviceID(DEBUG);

  SerialUSB.begin(115200);                        // initialize the USB serial port
  Serial1.begin(115200);                          // initialize the SIM7600 module (Somehow automatically on Serial1)

  if(!radio.begin()){SerialUSB.println("Radio failed to start");}
  radio.setDataRate(RF24_1MBPS);                 // set high data rate for longer distance.
  radio.setPALevel(RF24_PA_HIGH);  

  pinMode(LTE_RESET_PIN, OUTPUT);                 // bunch of initialization stuff 
  digitalWrite(LTE_RESET_PIN, LOW);

  pinMode(LTE_PWRKEY_PIN, OUTPUT);
  digitalWrite(LTE_RESET_PIN, LOW);
  delay(100);
  digitalWrite(LTE_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(LTE_PWRKEY_PIN, LOW);

  pinMode(LTE_FLIGHT_PIN, OUTPUT);
  digitalWrite(LTE_FLIGHT_PIN, LOW);

  SerialUSB.println("Initializing...");

  delay(100);

  response = sendCMD("AT+CGMM", 3000, DEBUG);

  while(response.indexOf("PB DONE") < 0){
    if(Serial1.available()){
      char c = Serial1.read();
      response += c;
      delay(1);
    }
  }

  delay(1000);
  sendCMD("AT+CMGF=1", 1000, DEBUG);                              // set SMS text mode
  sendCMD("AT+CNMI=2,1,0,0,0", 1000, DEBUG);                      // set up notification for incoming SMS messages

  SerialUSB.println("Clearing Old SMS Messages...");              
  clearSMS(DEBUG);                                                // clear up SMS storage
  delay(100);

  SerialUSB.println("SMS TESTING START!");
  sendSMS("SIM7600 Online!");                              // test send message
  sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\nhelp");
  delay(100);
  Alarm(device[1], DEBUG);
}


void loop() {
  updateSMS(0, DEBUG);
  delay(100);
}


