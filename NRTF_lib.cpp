#include <string.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <Arduino.h>
#include "sim7600_lib.h"
#include "NRTF_lib.h"



// receives payload from specified address.
boolean getPayload(const byte address[6]){

  if(radio.available() > 0){

    radio.read(&payload, sizeof(payload));

    if(DEBUG){
      SerialUSB.println("RF Payload Received!");
      SerialUSB.print("Payload receieved size: ");SerialUSB.println(sizeof(payload));
    }

    Message.BatLevel = payload.BatLevel;
    Message.MessageCount = payload.MessageCount;
    Message.MessageType = payload.MessageType;
    Message.AlarmDelay = payload.AlarmDelay;
    Message.ControllerID = payload.ControllerID;
    Message.Mode = payload.Mode;
    Message.NodeID = payload.NodeID;
    Message.Sensor1 = payload.Sensor1;
    Message.Sensor2 = payload.Sensor2;
    Message.Sensor3 = payload.Sensor3;
    Message.State = payload.State;
    DumpDatabase(device[0]); 
    return true;
  }

  return false;
}



// send payload too radio device at specified address
boolean sendPayload(const byte address[6]){
  boolean SentOk = false;
  int Retry = 0;

  radio.stopListening();
  delay(500);
  while ((!SentOk) && (Retry<6)){  
    SentOk = radio.write(&payload, sizeof(payload));
    Retry++;
    SerialUSB.print("S+");
    delay(1000);
  }
  if(SentOk) {
    SerialUSB.println("\nMessage sent OK.");
  }
  else {
    SerialUSB.print("Message send failure. ");
    SerialUSB.println("Reconfiguring radio.");
    configureRadio(address);
    return SentOk;
  }
  radio.startListening();
  return SentOk;
}


// debug tool
void DumpDatabase(Sensor device) { 
  SerialUSB.println("------------  Database -------------");
  //Serial.print("Current State = ");Serial.println(CurrentState);
  SerialUSB.print("NodeID ="); SerialUSB.print(payload.NodeID);SerialUSB.println(device.ID);
  SerialUSB.print("ControllerID ="); SerialUSB.print(payload.ControllerID);SerialUSB.println(Message.ControllerID);
  SerialUSB.print("Sensor 1 ="); SerialUSB.print(payload.Sensor1);SerialUSB.println(device.tilt);
  SerialUSB.print("Sensor 2 ="); SerialUSB.print(payload.Sensor2);SerialUSB.println(device.light);
  SerialUSB.print("Sensor 3 ="); SerialUSB.print(payload.Sensor3);SerialUSB.println(device.conductivity);
  SerialUSB.print("Message Type ="); SerialUSB.println(Message.MessageType);
  SerialUSB.print("Message Count ="); SerialUSB.print(payload.MessageCount);SerialUSB.println(Message.MessageCount);
  SerialUSB.print("Mode ="); SerialUSB.println(Message.Mode);
  SerialUSB.print("State ="); SerialUSB.println(Message.State);
  SerialUSB.print("Alarm delay ="); SerialUSB.println(Message.AlarmDelay);
  SerialUSB.print("Battery level ="); SerialUSB.println(Message.BatLevel);
  SerialUSB.println("------------  end -------------");
  return;
}


// radio configuration.
boolean configureRadio(const byte address[6]){
  if(!radio.begin(&rfSPI)){
    SerialUSB.println("Radio failed to start");
    return false;
    }
  radio.setDataRate(RF24_1MBPS);                 // set high data rate for longer distance.
  radio.setPALevel(RF24_PA_MAX);
  radio.setRetries(4, 10);                       // set time between retries and max no. of retries
  radio.openReadingPipe(0, address);
  radio.openWritingPipe(address);

  radio.startListening();
  return true;    
}




void loadPayload(Sensor device, byte message_type){
  payload.NodeID = device.ID;
  payload.ControllerID = 1;
  payload.Sensor1 = device.tilt;
  payload.Sensor2 = device.light;
  payload.Sensor3 = device.conductivity;
  payload.MessageType = message_type;
  payload.Mode = NullState;
  payload.State = NullState;
  payload.Future1 = 0;
  payload.BatLevel = 0;
  payload.AlarmDelay = 0;
  payload.MessageCount = 0;
}



// CHANGE SO THAT RETRY LOOP COVERS ALL ERRORS
boolean PushConfig (Sensor device) {

  // push config too the device.
  loadPayload(device, LoadParms);
  int retry = 1;
  while (retry < 6) {
    if (!sendPayload(address)) {
      sendSMS("Failed to load config. Retrying... ("+ String(retry) +")");
      retry++;
      continue;  
    }
    break;
  }
  if (retry == 6) {
    sendSMS("Could not reach device. Configuration canceled, Config Saved.");
    return false;
  }

  delay(5000);
  
  // put device to sleep.
  loadPayload(device, GoToSleep);
  if (!sendPayload(address)) {
    sendSMS("Failed to load config.");
    return false;  
  }

  radio.flush_rx();
  // check for succesful send.
  unsigned long int time = millis();
  unsigned int timeout = 30000;
  time = time+timeout;
  while (!(getPayload(address))) {
    if (millis()>time) {
      sendSMS("Radio failed to respond. Configure canceled.");
      return false;
    }
  }

  // check for proper reply
  switch (Message.Mode) {
    case WaitForCmd:
      break;
    case CommsFail:
      sendSMS("Failed to arm. Could not communicate with the device.");
      return false;
    case CantArm:
      sendSMS("Failed to arm. Make sure sensors are not activated while arming.");
      return false;
  }
  return true;
}


boolean pingRF(const byte address[]){
  unsigned long int time = millis();
  long int timeout = 15000;
  long int time_spent = 0;

  loadPayload(device[0], ReqStatus);
  if(!sendPayload(address)){ sendSMS("Device unavailable. Failed send."); return false;}

  unsigned long int time2 = millis();
  while((radio.available() == 0) && (millis() < timeout+time2)){
    SerialUSB.print(".");
    delay(1);
  }

  SerialUSB.println(radio.available());

  if(!getPayload(address)){ sendSMS("Device unavailable. Device timed out, no message returned."); return false;}
  
  device[0] = storeStatus(device[0]);
  device[0].status = ACTIVE;

  time_spent = millis() - time;
  sendSMS("Device Online. Ping succesful. Time spent: "+ String(time_spent) +" ms.");
  Status(device[0]);
  return true;
}