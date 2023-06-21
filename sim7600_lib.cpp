#include <Arduino.h>
#include "sim7600_lib.h"
#include "NRTF_lib.h"


//Sends SMS message to phone.
boolean sendSMS(String message){
  unsigned long int time = millis();
  unsigned long int timeout = time + 10000;
  String cmd = "AT+CMGS=\"" + phoneNum + "\"";

  sendCMD(cmd, 1000, DEBUG);                                                  // sends " AT+CMGS= "+12269357857" "
  delay(100);

  Serial1.print(message);                                                     // print message to send to SIM7600 Serial.
  delay(100);
  Serial1.write(26);                                                          // ASCII code for Ctrl-Z.

  String serial = "";
  while ((serial.indexOf("+CMGS: ") < 0) && (serial.indexOf("ERROR") < 0)) {  // loops until the message is confirm sent.
    if(Serial1.available()) 
      serial += char(Serial1.read());
    delay(1);
    if(timeout < millis()){                                                   // timeout when cell service is bad.                      
      SerialUSB.print("\nVERY POOR CONNECTION!");
      POOR_CONNECTION = true;
      timeout += 10000;
    }
  }

  if(DEBUG){SerialUSB.println(serial);}

  while(Serial1.available() && serial.indexOf("OK") < 0)                      // look for OK response.
    serial += char(Serial1.read());


  if(serial.indexOf("ERROR") != -1){                                          // error catch if message fails to send.
    if(DEBUG){SerialUSB.println(serial);}
    SerialUSB.println("Failed to send message.");
    return false;
    }
  else
    SerialUSB.println("Message sent successfully.");
    return true;
}



//Reads SMS message comming from phone.
String readSMS(const int timeout, int slot){

  String serial = "";
  String message = "";
  String CMD = "AT+CMGR="+ String(slot);
  int charCount = 0;

  delay(100);
  Serial1.println(CMD);                                                      // prints cmd which prints out message received.
  delay(100);

  while((Serial1.available())){                                              // looks for AT cmd response message.
    char c = Serial1.read();                                                    
    if((c == '\n') && (serial.indexOf("+CMGR: \"REC UNREAD\"") >= 0))        // parses out the junk part of message.
      break;

    serial += c;
    delay(1);
  }

  if(DEBUG){SerialUSB.println(serial);}                                      // for debugging, prints out junk part of message.

  if(serial.indexOf("OK") == -1) {                                           // if no message was received, only "OK" will be read.
    long int time = millis();
    boolean loop = true;

    while ((time + timeout) > millis() && loop){                             // stores actual message sent by SMS.
      while (Serial1.available()){
        char c = Serial1.read();
        if((c == '\n')){
          loop = false;                                                      // breaks loop once message is read.
          break; 
        }

        message += c;
        delay(1);
      }
    }
    if(DEBUG){SerialUSB.println("New unread message: "+ message);}             // prints out message 
    return message; 
  }

  else
    if (DEBUG) SerialUSB.println("No unread messages");

  clearSMS();
  return "";
}



// Clears all old SMS messages
void clearSMS () {
  String CMD = "";
  String serial = "";

  for (int i = 0; i <= 49; i++) {                                 // runs through all SMS memory slots (50).
    CMD = "AT+CMGD=" + String(i);    
    sendCMD(CMD, 10, DEBUG);
    //Serial1.println(CMD);    
    delay(1);
    
    while (Serial1.available()) {
      char c = Serial1.read();
      serial += c;
    }//while

    if (serial.indexOf("ERROR") != -1) {                        // checks for errors in +CMGD cmd
      SerialUSB.println("Failed to clear messages");
      return;
    }//if
    
    if (DEBUG) {                                                 // prints out AT cmd responses. for debugging.
      while (Serial1.available()) { 
        SerialUSB.write(Serial1.read());
        delay(1);
      }
    }
  }
}



//Checks for any new SMS messages. Looks for new message prompt. Needs to be put in a loop.
String updateSMS (int mode) {
  char* bufPtr = buffer;
  String message;
  
  if (Serial1.available()) {
    int slot = 0;

    do {                                                           // reads SIM7600 Serial.
      *bufPtr = Serial1.read(); 
      
      if(DEBUG){SerialUSB.write(*bufPtr);} 
      delay(1);
    } while((*bufPtr++ != '\n') && (Serial1.available()));         

    *bufPtr = 0;
    if (1 == (sscanf(buffer, "+CMTI: \"SM\",%d", &slot))){         // looks for new message prompt. slot number is parsed from the message.

      if (DEBUG) {
        SerialUSB.print("slot: ");
        SerialUSB.println(slot);
      }

      message = readSMS(1000, slot);                               // read message at the slot number found.

      if (mode == 0) {                                             // checks the message for key words.
        if (!checkSMS(message, slot, DEBUG))                         
          if (DEBUG) SerialUSB.println("NOT A VALID COMMAND");
      }//if

      else if(mode == 1){                                          // second mode which only returns the message.
        return message;
      }//else if
    }//if

    /*DEBBUGGING*/
    if(DEBUG) SerialUSB.write(bufPtr);                             // prints the message (i think).
    
  }//if
  Serial1.flush();                                                 
  return "";                                                       // return nothing if new message notif not found.
}



//Sends command too SIM7600 Serial
String sendCMD (String cmd, const int timeout, boolean debug) {
    String serial = "";
    Serial1.flush();

    Serial1.println(cmd);                                           // prints command to SIM7600 Serial.

    long int time = millis();
    while ((time + timeout) > millis()){                            // reads any error or OK responses spat out from the SIM7600 Serial.
      while (Serial1.available()){
        char c = Serial1.read();
        serial += c;
      }//while
    }//while

    if (debug) SerialUSB.print(serial);                           // prints response to command. for debugging.

    return serial;                                                // returns SIM7600 response for debugging. will change too true false maybe.
}



//Looks through SMS message for certain key words.
boolean checkSMS(String message, int slot, boolean debug) {
  int ID;
  message.toLowerCase();                                               // message put too lower case for better user experience.
  
  // gets sensor ID from user input.
  if (message.indexOf("s") == 0) {                                     
    ID = getID(message);
    if(ID == -1)
      return false;
    
    // parse out ID from the message
    message.remove(0,3);                                               
    SerialUSB.println("CMD Type: "+ message);
    
    // checks for "status" cmd. 
    if (message.indexOf("status") == 0) {                              
      if(debug) SerialUSB.println("SENDING STATUS");
      Status(device[ID-1]);                                            // gets device status.
      return true;
    }

    // checks for "configure" cmd.
    else if (message.indexOf("configure") == 0) {                      
      if(debug) SerialUSB.println("SENDING CONFIGURE");
      device[ID-1] = ChangeConfig(device[ID-1]);                       // runs through the change config process.
      return true;
    }      
    
    // checks for "disarm" cmd.
    else if (message.indexOf("disarm") == 0) {                         
      if(debug) SerialUSB.println("SENDING DISARM");
      device[ID-1] = Disarm(device[ID-1]);                             // runs through device disarming process.
      return true;
    }

    // checks for "arm" cmd.
    else if (message.indexOf("arm") == 0) {                            
      if(debug){SerialUSB.println("SENDING ARM");}
      device[ID-1] = AlarmOn(device[ID-1]);                            // runs through arming process.
      return true;
    }
    
    // checks for "ping" cmd.
    else if (message.indexOf("ping") == 0) {                           
      if (!(device[ID-1].configured)) {
        sendSMS("S"+ String(device[ID-1].ID) +" has not been configured yet. Ping canceled.");
        return false;
      }
      
      if (DEBUG) SerialUSB.println("SENDING PING");                 
      sendSMS("Pinging...");
      SerialUSB.println((pingRF(address) ? "Ping Success" : "Ping Failed"));
      return true;
    }

    // catch for unknown cmd.
    else {
      sendSMS("ERROR: Unknown command.\nPlease type \"help\" for list of valid commands.");
      return false;
    }
  }

  // NON DEVICE RELATED COMMANDS

  // checks for "help" cmd.  
  else if (message.indexOf("help") == 0) {                                            
    Help(debug);
    return true;
  }
  
  // checks for "changenum" cmd.
  else if (message.indexOf("changenum") == 0) {                       
    String new_phone_number = message;
    
    // get and store new phonenumber.
    int first = new_phone_number.indexOf("\"");
    first++;
    new_phone_number.remove(0, first);
    int last = new_phone_number.indexOf("\"");
    last;
    new_phone_number.remove(last, 20);
    if (DEBUG) SerialUSB.println("New phone number: "+ new_phone_number);

    sendSMS("Are you sure you would like to change the phone number too: "+ new_phone_number);

    // user has 10 minutes to respond before process times out.
    int response = getYN(600000);
    if (response == NOREPLY) {sendSMS("Process timed out. Changenum canceled."); return false;}

    // user response check.
    if (response == YES) {
      sendSMS("Phone number changed.");
      phoneNum = new_phone_number;
      SerialUSB.println(phoneNum);
      Help(debug);
      return true;
    } 
    else if (response == NO) { 
      sendSMS("Changenum canceled."); return true;
    }
  }

  // non command message catch.
  else {
    if(debug) SerialUSB.println("NOT AN OPTION");
    message.trim();
    sendSMS("\""+ message +"\" is not a known command. Please type \"help\" for a list of commands.");    
    return false;
  }
}



/*  Gets the status of specified device. 
    NEED TO PRINT MORE STATUS STUFF. **CHANGE THIS LATER**
*/
void Status (Sensor device) {
  String message;
  String config = CurrConfig(device);
  String state = CurrState(device);

  message = ("----- Status -----\nDevice ID: "+ String(device.ID) 
            +"\nName: "+ device.name 
            +"\nStatus: "+ (device.status ? "ACTIVE" : "INACTIVE")
            +"\nState: "+ state
            +"\nLast Updated: "+ device.datetime
            +"\nBattery Level: "+ device.BatLevel +"%");
  sendSMS(message);
  SerialUSB.println(message);
  
  message = CurrConfig(device);
  sendSMS(message);

  SerialUSB.println(message);
}



/**/
Sensor ChangeConfig (Sensor device) {
  String userResponse = "";
  const int timeout = 600000;
  long int time = millis();
  int loop = 1;

  time = time + timeout;

  // process for device that is already configured
  if (device.configured) {
    sendSMS("This device has already been configured. Would you like too wipe the current configuration and reconfigure? y/n");
    sendSMS(CurrConfig(device));
    
    // function timeout.
    reply = getYN(time);
    if (reply == NOREPLY) loop = 0;

    // wipe device database.
    else if (reply == YES) { 
      device.conductivity = OFF; 
      device.light = OFF; 
      device.tilt = OFF; 
      device.name = "";
      device.configured = 0;
    }

    else if (reply == NO) {
      sendSMS("S"+ String(device.ID) +" configuration canceled.");
      return device;
    }
  }

  // device configuration process.
  while (time > millis() || loop) {

    sendSMS("What is the address of the installation of s"+ String(device.ID) +"?");

    while (userResponse.equals("") && time > millis())
      userResponse = updateSMS(1);

    userResponse.trim();
    device.name = userResponse;

    sendSMS("Activate TILT SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;

    if (reply == YES) device.tilt = ON;
    sendSMS("OK.");                                                          //THIS "OK" STUFF MIGHT NEED TO CHANGE!
    delay(100);
    
    sendSMS("Activate LIGHT SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;
    
    if (reply == YES) device.light = ON;
    sendSMS("OK.");
    delay(100);

    sendSMS("Activate CONDUCTIVITY SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;

    if (reply == YES) device.conductivity = ON;
    sendSMS("OK.");
    delay(100);

    sendSMS(CurrConfig(device));

    sendSMS("Would you like to make any changes? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;


    // configure sensor.
    if (reply == NO) {
      sendSMS("S"+ String(device.ID) +" Configuration Saved. Pushing configuration, this may take a moment.");

      // send parms too sensor.
      loadPayload(device, LoadParms);
      if (!sendPayload(address)) {
        sendSMS("Failed to load config.");
        return device;  
      }
      delay(5000);
      loadPayload(device, GoToSleep);
      if (!sendPayload(address)) {
        sendSMS("Failed to load config.");
        return device;  
      }

      radio.flush_rx();
      // check for succesful send.
      unsigned long int time2 = millis();
      unsigned int timeout2 = 30000;
      time2 = time2+timeout2;
      while (!(getPayload(address))) {
        if (millis()>time2) {
          sendSMS("Radio failed to respond. Configure canceled.");
          return device;
        }
      }

      // check for proper reply
      switch (Message.Mode) {
        case WaitForCmd:
          break;
        case CommsFail:
          sendSMS("Failed to arm. Could not communicate with the device.");
          return device;
        case CantArm:
          sendSMS("Failed to arm. Make sure sensors are not activated while arming.");
          return device;
      }
      delay(5000);
      sendSMS("Config pushed succesfully!");

      // return to main menu.
      sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\ns# arm\ns# ping\nhelp");
      device.configured = 1;
      clearSMS();                 
      return device;
    }

    sendSMS("RECONFIGURING...");
    device.conductivity = OFF; device.light = OFF; device.tilt = OFF; device.name = "";
    userResponse = "";
  }//while
  sendSMS("S"+ String(device.ID) +" configuration canceled. Proccess timed out.");
  return device;
}//function



/*  Pushes the config made by the user and returns 
    message to send to user by SMS.               */
String CurrConfig (Sensor device) {
  String message;
  message = ("S"+ String(device.ID) 
            +" CURRENT SENSOR CONFIG:\n----- Config -----\nInstall Address: "+ device.name 
            +"\nTilt Sensor = "+ (device.tilt ? "ON" : "OFF") 
            +"\nLight Sensor = "+ (device.light ? "ON" : "OFF")
            +"\nConductivity Sensor = "+ (device.conductivity ? "ON" : "OFF"));

  return message;
}


String CurrState (Sensor device) {

  switch(Message.State){
    case NullState: return "Null State";
    case Asleep:  return "Asleep";
    case Connecting:  return "Connecting";
    case WaitForCmd:  return "Waiting for Cmd";
    case Arming:  return "Arming";
    case Armed: return "Armed";
    case Alarming:  return "Alarming";
    case CommsFail: return "Comms Fail";
    case Configured:  return "Configured";
    case AmSilent:  return "Silent";
    case CantArm: return "Cant Arm";
  }
}



// Arms sensor module.
Sensor AlarmOn (Sensor device) {

  // check if device is not configured.
  if (!device.configured) {
    sendSMS("S"+ String(device.ID) +" has not been configured yet. Arming canceled.");
    return device;
  }

  // beginning arming process.
  sendSMS("Arming s"+ String(device.ID) +". This may take a few moments.");
  device.status = ACTIVE;
  device.state = Armed;

  // send command to arm.
  loadPayload(device, GoToArm);
  if (!sendPayload(address)) {
    sendSMS("Failed to arm.");
    return device;
  }
  delay(5000);

  // look for sensor response.
  if (!getPayload(address)) {sendSMS("Failed to reach module."); return device;}

  // check response.
  switch (Message.State) {
    case CantArm: 
      sendSMS("Failed to activate sensors. Make sure sensors are not activated while arming.");
      return device;
  }

  sendSMS("s"+ String(device.ID) +" is now ARMED.");
  acknowledge = false;
  return device;
}



// Disarms sensor module.
Sensor Disarm (Sensor device) {
  long int timeout = 600000;

  if (!device.configured) { 
    sendSMS("S"+ String(device.ID) +" has not been configured yet. Disarm canceled.");
    return device;
  }

  sendSMS("----- DISARMING -----\nDevice: s"+ String(device.ID) 
         +"\nAddress: "+ device.name 
         +"\nAre you sure you would like to disarm s"+ String(device.ID) +"? (y or n)");


  unsigned long long int time = millis();
  time = time + timeout;
  while (time > millis()) {
    reply = getYN(time);

    if (reply == YES) {
      device.status = INACTIVE;
      loadPayload(device, GoToSleep);
      if (!(sendPayload(address))) {
        sendSMS("Failed to communicate with the device. Disarm canceled.");
        return device;
      }
      while (!(getPayload(address)));
      switch (Message.State) {
        case Asleep:
          break;
        case CommsFail:
          sendSMS("Failed to disarm. Could not communicate with the device.");
          return device;
      }

      // TELL SENSOR MODULE TOO DISARM.
      sendSMS("s"+ String(device.ID) +" disarmed.");
      break;
    }

    else if (reply == NO) {
      sendSMS("Disarm canceled");
      break;
    }

    else if (reply == NOREPLY) {
      sendSMS("Process timed out. Disarm canceled.");
      break;
    }
  }
  if(DEBUG) SerialUSB.println("DISARMING COMPLETE");
  return device;
}



/*Gives all devices an ID*/ 
void setDeviceID () {
  for (int i = 0; i <= 3; i++) {
    device[i].ID = i+1;
    if (DEBUG) SerialUSB.println("Device "+ String(i) +" ID: "+ String(device[i].ID));
  }
}



String getDateTime () {
  String date = "";

  date = sendCMD("AT+CCLK?", 1000, DEBUG);
  delay(100);

  if (date.indexOf("ERROR") != -1) {
    SerialUSB.println("ERROR GETTING DATE!");
    return "";
  }

  int first = date.indexOf("\"");
  int last = date.indexOf("\"");
  date.remove(0, first+1);
  date.remove(last-3, 20);
  date.trim();

  if (DEBUG) SerialUSB.println(date);

  return date;
}



int getID (String message) {
  int ID;

  message.remove(0,1);
  message.remove(1, 20);
  SerialUSB.println("ID: "+ message);
  ID = message.toInt();

  if (1 <= ID && ID <= 4) return ID;

  sendSMS("s"+ String(ID) +" is not a device!");
  return -1;
}



int getYN (unsigned long int time) {
  String userResponse = "";

  while (time > millis()) {
    userResponse = updateSMS(1);
    userResponse.toLowerCase();

    if (userResponse.equals("")) continue;

    else if (userResponse.indexOf("y") == 0) return YES;

    else if (userResponse.indexOf("n") == 0) return NO;

    else {
      sendSMS("That is not an option. Please type either \"y\" or \"n\"");
      Serial1.flush();
    }

  delay(1);
  }
  return NOREPLY;
}



void Help (boolean debug) {
  sendSMS("----- Help List -----");  
  sendSMS("Command Type: status\n USAGE EXAMPLE: s# status");
  sendSMS("Command Type: configure\n USAGE EXAMPLE: s# configure");
  sendSMS("Command Type: disarm\n USAGE EXAMPLE: s# disarm");
  sendSMS("Command Type: arm\n  Usage Example: s# arm");
  sendSMS("Type \"help\" if you ever need this list of commands again.");
}



void Alarm (Sensor device) {
  int count = 1800000;                                        // starts at 10 min so that the first alert goes through.
  int loops = 0;                                              // 3 alerts go out, counts the amount of loops Alarm goes through.
  

  while (!(acknowledge) && loops < 3) {
    count++;
    if (updateSMS(1) != "") {
      SerialUSB.println("Acknowledgement Received");
      sendSMS("ALERT ACKNOWLEDGED.");
      acknowledge = true;
      loadPayload(device, GoSilent);
      sendPayload(address);
      return;
    }

    if (count >= 1800000) {
      sendSMS("<<ALERT>>\nDevice ID: "+ String(device.ID) + " has been tampered with!");
      sendSMS("Install Address: "+ device.name 
             +"\nSensors Triggered:\n"
             +((device.tilt==TRIGGERED) ? "  - TILT SENSOR\n" : "") 
             +((device.light==TRIGGERED) ? "  - LIGHT SENSOR\n" : "")
             +((device.conductivity==TRIGGERED) ? "  - CONDUCTIVITY SENSOR\n" : "")
             +"Date/Time: "+ device.datetime 
             +"\nDevice Status: "
             +((device.status==ACTIVE) ? "ACTIVE" : "INACTIVE"));
      sendSMS("To stop alerts please type \"OK\"");
      count = 0;
      loops++;
    }
  delay(1);
  }
}



Sensor storeStatus (Sensor device) {
  device.datetime = getDateTime();
  device.state = Message.State;
  device.BatLevel = Message.BatLevel;
  device.tilt = Message.Sensor1;
  device.light = Message.Sensor2;
  device.conductivity = Message.Sensor3;
  return device;
}



void setDeviceAddress () {
  int ID;

  for (int i = 0; i <= 3; i++) {
    ID = i+1;
    char c = ID + '0';
    // device.address[6] = {'0','0','0','0', c};
    //strcpy(device[i].address, new_address);
    if (DEBUG) SerialUSB.println("Device "+ String(ID) +" address: "+ String(device[i].address));
  }
}
