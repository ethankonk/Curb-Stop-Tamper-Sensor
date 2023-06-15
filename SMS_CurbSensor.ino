#include <SPI.h>
#include <RF24.h>
#include "wiring_private.h"
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
#define DEBUG false

// MISC. variables
String response = "";

void setup() {

  SerialUSB.begin(115200);                                                // initialize the USB serial port.
  Serial1.begin(115200);                                                  // initialize the SIM7600 module (Hardwired to Serial1).
  rfSPI.begin();
  pinPeripheral(11, PIO_SERCOM);
  pinPeripheral(12, PIO_SERCOM);
  pinPeripheral(13, PIO_SERCOM);

  pinMode(LTE_RESET_PIN, OUTPUT);                                        
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
  setDeviceID();
  setDeviceAddress();

  delay(100);

  response = sendCMD("AT+CGMM", 3000, DEBUG);

  while(response.indexOf("PB DONE") < 0 && response.indexOf("SIMCOM_SIM7600A-H") < 0){
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
  clearSMS();                                                // clear up SMS storage
  delay(100);

  configureRadio(address);

  SerialUSB.println("SMS TESTING START!");
  sendSMS("SIM7600 Online!");                              // test send message
  if(POOR_CONNECTION){  sendSMS("WARNING! POOR LTE CONNECTION. MESSAGES MAY TAKE LONGER TO SEND.");}
  sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\ns# arm\nhelp");
  delay(100);
}

void loop() {
  updateSMS(0);

  //for(int i=0; i<3; i++){
    //if(device[i].configured)
  if(getPayload(address)){
    device[0] = storeStatus(device[0]);
    //sendCMD("AT+CGMM", 1000, DEBUG);
  }
  if(device[0].state == Alarming && Message.Mode == Alarming)
    Alarm(device[0]);     
  //}

  
  delay(1);
}


