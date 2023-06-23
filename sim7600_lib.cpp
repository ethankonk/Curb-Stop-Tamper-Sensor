#include <Arduino.h>
#include "sim7600_lib.h"
#include "NRTF_lib.h"


//Sends SMS message to phone.
boolean sendSMS(String message){
  unsigned long int time = millis();
  unsigned long int timeout = time + 10000;
  String cmd = "AT+CMGS=\"" + phoneNum + "\"";

  sendCMD(cmd, 1000);                                                  // sends " AT+CMGS= "+12269357857" "
  delay(100);

  Serial1.print(message);                                                     // print message to send to SIM7600 Serial.
  delay(100);
  Serial1.write(26);                                                          // ASCII code for Ctrl-Z.

  // loops until the message is confirm sent.
  String serial = "";
  while ((serial.indexOf("+CMGS: ") < 0) && (serial.indexOf("ERROR") < 0)) {  
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

  // look for OK response.
  while(Serial1.available() && serial.indexOf("OK") < 0)                      
    serial += char(Serial1.read());

  // error catch if message fails to send.
  if(serial.indexOf("ERROR") != -1){                                          
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

  // reads AT cmd response message.
  while((Serial1.available())){                                              
    char c = Serial1.read();                                                    
    if((c == '\n') && (serial.indexOf("+CMGR: \"REC UNREAD\"") >= 0))        // parses out the junk part of message.
      break;

    serial += c;
    delay(1);
  }

  if(DEBUG){SerialUSB.println(serial);}                                      // for debugging, prints out junk part of message.

  if(serial.indexOf("OK") == -1) {                                           
    long int time = millis();
    boolean loop = true;

    // stores actual message sent by SMS.
    while ((time + timeout) > millis() && loop){                             
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

  // runs through all SMS memory slots (50).
  for (int i = 0; i <= 49; i++) {                                 
    CMD = "AT+CMGD=" + String(i);    
    sendCMD(CMD, 10);
    //Serial1.println(CMD);    
    delay(1);
    
    while (Serial1.available()) {
      char c = Serial1.read();
      serial += c;
    }//while

    if (serial.indexOf("ERROR") != -1) {                        // checks for errors in +CMGD cmd
      SerialUSB.println("Failed to clear messages.");
      return;
    }//if
    
    if (DEBUG) {                                                // prints out AT cmd responses. for debugging.
      SerialUSB.println("Clearing SMS messages.");
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

      // first mode which checks the message for key words.
      if (mode == 0) {                                             
        if (!checkSMS(message, slot, DEBUG))                         
          if (DEBUG) SerialUSB.println("NOT A VALID COMMAND");
      }

      // second mode which only returns the message.
      else if(mode == 1){                                          
        return message;
      }
    }

    if(DEBUG) SerialUSB.write(bufPtr);                             // prints the message (i think). for debugging.  
  }
  Serial1.flush();                                                 
  return "";                                                       // return nothing if new message notif not found.
}



//Sends command too SIM7600 Serial
String sendCMD (String cmd, const int timeout) {
    String serial = "";
    Serial1.flush();

    Serial1.println(cmd);                                         // prints command to SIM7600 Serial.

    long int time = millis();
    while ((time + timeout) > millis()){                          // reads any error or OK responses spat out from the SIM7600 Serial.
      while (Serial1.available()){
        char c = Serial1.read();
        serial += c;
      }//while
    }//while

    if (DEBUG) SerialUSB.println("Sending command: "+ cmd);                           // prints response to command. for debugging.

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
    ID--;
    
    // parse out ID from the message
    message.remove(0,3);                                               
    SerialUSB.println("CMD Type: "+ message);
    
    // checks for "status" cmd. 
    if (message.indexOf("status") == 0) {                              
      if(debug) SerialUSB.println("SENDING STATUS");
      Status(device[ID]);                                            // gets device status.
      return true;
    }

    // checks for "configure" cmd.
    else if (message.indexOf("configure") == 0) {                      
      if(debug) SerialUSB.println("SENDING CONFIGURE");
      device[ID] = ChangeConfig(device[ID]);                       // runs through the change config process.
      return true;
    }      
    
    // checks for "disarm" cmd.
    else if (message.indexOf("disarm") == 0) {                         
      if(debug) SerialUSB.println("SENDING DISARM");
      device[ID] = Disarm(device[ID]);                             // runs through device disarming process.
      return true;
    }

    // checks for "arm" cmd.
    else if (message.indexOf("arm") == 0) {                            
      if(debug){SerialUSB.println("SENDING ARM");}
      device[ID] = AlarmOn(device[ID]);                            // runs through arming process.
      return true;
    }
    
    // checks for "ping" cmd.
    else if (message.indexOf("ping") == 0) {                           
      if (!(device[ID].configured)) {
        sendSMS("S"+ String(device[ID].ID) +" has not been configured yet. Ping canceled.");
        return false;
      }
      
      if (DEBUG) SerialUSB.println("SENDING PING");                 
      sendSMS("Pinging...");
      SerialUSB.println((pingRF(address) ? "Ping Success" : "Ping Failed"));
      return true;
    }

    // check for "quickconfig" cmd. debuging tool.
    else if (message.indexOf("quickconfig") == 0) {
      message = "";
      if (DEBUG) SerialUSB.println("SENDING QUICKCONFIG");

      // get device config.
      unsigned long int time = millis();
      time = time + 600000;
      while(millis() < time){
        // get device address.
        sendSMS("What is the devices address?");
        while (message.equals("")) message = updateSMS(1);
        message.trim();
        device[ID].name = message;

        // get device configuration.
        message = "";
        sendSMS("Please send the desired configuration.");
        while (message.equals("")) message = updateSMS(1);
        
        if (message.length() > 4) {
          sendSMS("Invalid config format. Please only send 3 digits comprised of 1 and 0. Ex: 110 for TILT on, LIGHT on, CONDUCTIVITY off.");
          continue;
        }
        int config = message.toInt();
        for (int i = 0; i < 3; i++){
          int c = config % 2;
          switch (c) {
            case 1:
              if (i == 0) device[ID].conductivity = ON;
              else if (i == 1) device[ID].light = ON;
              else if (i == 2) device[ID].tilt = ON;
              break;
            case 0:               
              if (i == 0) device[ID].conductivity = OFF;
              else if (i == 1) device[ID].light = OFF;
              else if (i == 2) device[ID].tilt = OFF;
          }
          config /= 10;
        }

        // send confirmation to user.
        sendSMS(CurrConfig(device[ID]));
        sendSMS("Would you like to make any changes? y/n");
        reply = getYN(time);
        if (reply == NOREPLY) {
          sendSMS("Process timed out. Quickconfig canceled.");
          return device;
        }
        else if (reply == YES) {
          sendSMS("Reconfiguring...");
          message = "";
          continue;
        }
        else if (reply == NO) {
          sendSMS("S"+ String(device[ID].ID) +" Configuration Saved. Pushing configuration, this may take a moment.");
          device[ID].configured = CONFIG_SAVED;
          break;
        }
      }

      // push config to device.
      if (!PushConfig(device[ID])) return false; 

      delay(5000);
      sendSMS("Config pushed succesfully!");

      // return to main menu.
      sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\ns# arm\ns# ping\nhelp");
      device[ID].configured = CONFIG_PUSHED;
      clearSMS();                 
      return device;
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



//Gets the status of specified device. 
void Status (Sensor device) {
  String message;
  String config = CurrConfig(device);
  String state = CurrState(device);

  // create and send status message.
  message = ("----- Status -----\nDevice ID: "+ String(device.ID) 
            +"\nName: "+ device.name 
            +"\nStatus: "+ (device.status ? "ACTIVE" : "INACTIVE")
            +"\nState: "+ state
            +"\nLast Updated: "+ device.datetime
            +"\nBattery Level: "+ device.BatLevel +"%");
  sendSMS(message);
  SerialUSB.println(message);
  
  // create and send config message.
  message = CurrConfig(device);
  sendSMS(message);
  SerialUSB.println(message);
}



// Process for changing the config for individual devices.
Sensor ChangeConfig (Sensor device) {
  String userResponse = "";
  const int timeout = 600000;
  long int time = millis();
  int loop = 1;
  time = time + timeout;

  // process for device that is already configured.
  if (device.configured == CONFIG_PUSHED) {
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
  
  // process for device that has configuration saved.
  else if (device.configured == CONFIG_SAVED) {
    sendSMS("A config has been made for this device. Would you like push it to s"+ String(device.ID) +" ? y/n");
    reply = getYN(time);
    switch (reply) {
      case YES:
        if(!PushConfig(device)) return device;
      case NO:
        break;
      case NOREPLY:
        sendSMS("S"+ String(device.ID) +" configuration canceled. Process timed out.");
        return device;
    }
  }

  // device configuration process.
  while (time > millis() || loop) {

    // get user response for device name.
    sendSMS("What is the address of the installation of s"+ String(device.ID) +"?");
    while (userResponse.equals("") && time > millis())
      userResponse = updateSMS(1);
    userResponse.trim();
    device.name = userResponse;

    // toggle tilt sensor.
    sendSMS("Activate TILT SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;

    if (reply == YES) device.tilt = ON;
    sendSMS("OK.");                                                          
    delay(100);
    
    // toggle light sensor;
    sendSMS("Activate LIGHT SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;
    
    if (reply == YES) device.light = ON;
    sendSMS("OK.");
    delay(100);

    // toggle conductivity sensor.
    sendSMS("Activate CONDUCTIVITY SENSOR? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;

    if (reply == YES) device.conductivity = ON;
    sendSMS("OK.");
    delay(100);

    // user confirmation for config.
    sendSMS(CurrConfig(device));
    sendSMS("Would you like to make any changes? y/n");
    reply = getYN(time);
    if (reply == NOREPLY) break;


    // configure sensor.
    if (reply == NO) {
      sendSMS("S"+ String(device.ID) +" Configuration Saved. Pushing configuration, this may take a moment.");
      device.configured = CONFIG_SAVED;

      if (!PushConfig(device)) return device;

      delay(5000);
      sendSMS("Config pushed succesfully!");

      // return to main menu.
      sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\ns# arm\ns# ping\nhelp");
      device.configured = CONFIG_PUSHED;
      clearSMS();                 
      return device;
    }

    // loops back in order too reconfigure.
    sendSMS("RECONFIGURING...");
    device.conductivity = OFF; device.light = OFF; device.tilt = OFF; device.name = "";
    userResponse = "";
  }

  // process timeout.
  sendSMS("S"+ String(device.ID) +" configuration canceled. Proccess timed out.");
  return device;
}



// Creates message with the current device config.
String CurrConfig (Sensor device) {
  String message;

  // create config message.
  message = ("S"+ String(device.ID) 
            +" CURRENT SENSOR CONFIG:\n----- Config -----\nInstall Address: "+ device.name 
            +"\nTilt Sensor = "+ (device.tilt ? "ON" : "OFF") 
            +"\nLight Sensor = "+ (device.light ? "ON" : "OFF")
            +"\nConductivity Sensor = "+ (device.conductivity ? "ON" : "OFF"));

  return message;
}



// Creates message with the current device state.
String CurrState (Sensor device) {

  // returns device state in a string form.
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

  // begin arming process.
  sendSMS("Arming s"+ String(device.ID) +". This may take a few moments.");
  device.status = ACTIVE;
  device.state = Armed;
  radio.flush_rx();

  // send command to arm.
  loadPayload(device, GoToArm);
  if (!sendPayload(address)) {
    sendSMS("Failed to arm.");
    return device;
  }
  delay(5000);

  // get device response.
  if (!getPayload(address)) {sendSMS("Failed to reach module."); return device;}
  while(!(radio.available()));
  if (!getPayload(address)) {sendSMS("Failed to reach module."); return device;}

  // check response.
  switch (Message.State) {
    case CantArm: 
      sendSMS("Failed to activate sensors. Make sure sensors are not activated while arming.");
      return device;
  }

  // confirm to user device is armed.
  sendSMS("s"+ String(device.ID) +" is now ARMED.");
  acknowledge = false;
  return device;
}



// Disarms sensor module.
Sensor Disarm (Sensor device) {
  long int timeout = 600000;

  // check if device is configured.
  if (!device.configured) { 
    sendSMS("S"+ String(device.ID) +" has not been configured yet. Disarm canceled.");
    return device;
  }

  // get user confirmation.
  sendSMS("----- DISARMING -----\nDevice: s"+ String(device.ID) 
         +"\nAddress: "+ device.name 
         +"\nAre you sure you would like to disarm s"+ String(device.ID) +"? (y or n)");
  
  // get user reply.
  unsigned long long int time = millis();
  time = time + timeout;
  while (time > millis()) {
    reply = getYN(time);

    // check user reply.
    if (reply == YES) {
      device.status = INACTIVE;
      loadPayload(device, GoToSleep);                                               // put device too sleep.
      if (!(sendPayload(address))) {
        sendSMS("Failed to communicate with the device. Disarm canceled.");
        return device;
      }
      while (!(getPayload(address)));                                               // check for proper response.
      switch (Message.State) {
        case Asleep:
          break;
        case CommsFail:
          sendSMS("Failed to disarm. Could not communicate with the device.");
          return device;
      }

      // confirm too user device disarmed.
      sendSMS("s"+ String(device.ID) +" disarmed.");
      break;
    }

    else if (reply == NO) {
      sendSMS("Disarm canceled");
      break;                                                                        // cancel disarm process.
    }

    else if (reply == NOREPLY) {
      sendSMS("Process timed out. Disarm canceled.");                               // process timeout.
      break;
    }
  }
  if(DEBUG) SerialUSB.println("DISARMING COMPLETE");
  return device;
}



// Gives all devices an ID
void setDeviceID () {
  for (int i = 0; i <= 3; i++) {
    device[i].ID = i+1;
    if (DEBUG) SerialUSB.println("Device "+ String(i) +" ID: "+ String(device[i].ID));
  }
}


// Gets date/time from Maduino board.
String getDateTime () {
  String date = "";

  // send get date command.
  date = sendCMD("AT+CCLK?", 1000);
  delay(100);

  // check for error response.
  if (date.indexOf("ERROR") != -1) {
    SerialUSB.println("ERROR GETTING DATE!");
    return "";
  }

  // parse out date.
  int first = date.indexOf("\"");
  int last = date.indexOf("\"");
  date.remove(0, first+1);
  date.remove(last-1, 20);
  date.trim();

  if (DEBUG) SerialUSB.println(date);

  return date;
}



// Gets the device ID the user inputed.
int getID (String message) {
  int ID;

  // parse out the ID.
  message.remove(0,1);
  message.remove(1, 20);
  SerialUSB.println("ID: "+ message);
  ID = message.toInt();

  // make sure the ID inputed is within the range.
  if (1 <= ID && ID <= 4) return ID;

  sendSMS("s"+ String(ID) +" is not a device!");
  return -1;
}


// Processes the yes or no answers from the user.
int getYN (unsigned long int time) {
  String userResponse = "";

  // get the user's input.
  while (time > millis()) {
    userResponse = updateSMS(1);
    userResponse.toLowerCase();

    // no input.
    if (userResponse.equals("")) continue;

    // "y" input.
    else if (userResponse.indexOf("y") == 0) return YES;

    // "n" input.
    else if (userResponse.indexOf("n") == 0) return NO;

    // invalid input.
    else {
      sendSMS("That is not an option. Please type either \"y\" or \"n\"");
      Serial1.flush();
    }

  delay(1);
  }
  // timeout (no input).
  return NOREPLY;
}



// Prints out the list of commands available.
void Help (boolean debug) {
  sendSMS("----- Help List -----");  
  sendSMS("Command Type: status\n USAGE EXAMPLE: s# status");
  sendSMS("Command Type: configure\n USAGE EXAMPLE: s# configure");
  sendSMS("Command Type: disarm\n USAGE EXAMPLE: s# disarm");
  sendSMS("Command Type: arm\n  Usage EXAMPLE: s# arm");
  sendSMS("Command Type: ping\n USAGE EXAMPLE: s# ping")
  sendSMS("Type \"help\" if you ever need this list of commands again.");
}



// Sends alarm messages to user's phone when tampering is detected.
void Alarm (Sensor device) {
  int loops = 0;                                              // 3 alerts go out, counts the amount of loops Alarm goes through.
  unsigned long int time = millis();
  
  // sends alarm messages until the user acknowledges the alarm or 3 messages have been sent.
  while (!(acknowledge) && loops < 3) {
    count++;

    // check for user acknowledgement.
    if (updateSMS(1) != "") {
      SerialUSB.println("Acknowledgement Received");
      sendSMS("ALERT ACKNOWLEDGED.");
      acknowledge = true;
      loadPayload(device, GoSilent);
      sendPayload(address);
      return;
    }

    // sends an alarm message every 10 minutes (1800000 milliseconds ish).
    if (millis() > time) {
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
      time += 1800000;
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
