#include <Arduino.h>
#include "sim7600_lib.h"
#include "NRTF_lib.h"


/*Sends SMS message too phone*/
// parameters: 
//  - message, message you want to send.
boolean sendSMS(String message){
  unsigned int timeout = 10000;
  unsigned long int time = millis();
  timeout = time + timeout;

  sendCMD(cmd, 1000, DEBUG);                                                // sends " AT+CMGS= "+12269357857" " (Will have to make function to change phone#)
  delay(100);

  Serial1.print(message);                                                   // print message to send to SIM7600 Serial
  delay(500);
  Serial1.write(26);                                                        // ASCII code for Ctrl-Z

  String serial = "";
  while((serial.indexOf("+CMGS: ") < 0) && (serial.indexOf("ERROR") < 0)){  // loops until the message is confirm sent.
    if(Serial1.available()) 
      serial += char(Serial1.read());
    delay(1);
    if(timeout < millis()){                                                  
      SerialUSB.print("\nVERY POOR CONNECTION!");
      POOR_CONNECTION = true;
      timeout += 10000;
    }
  }

  if(DEBUG){SerialUSB.println(serial);}

  String response = "";
  while(Serial1.available() && response.indexOf("OK") < 0)                  // look for OK response.
    response += char(Serial1.read());


  if(serial.indexOf("ERROR") != -1 || response.indexOf("ERROR") != -1){
    if(DEBUG){SerialUSB.println(response);}
    SerialUSB.println("Failed to send message");
    return false;
    }
  else
    SerialUSB.println("Message sent successfully");
    return true;
}



/*Reads SMS message comming from phone. NEEDS TO BE USED IN UpdateSMS()*/
// parameters:
//  - timeout, time the loop will wait before throwing up an error.
//  - slot, message slot number. IMPORTANT FOR READING THE CORRECT MESSAGE.
//  - debug, just to see whether to throw up the debug stuff.
String readSMS(const int timeout, int slot){

  String response = "";
  String message = "";
  String CMD = "AT+CMGR="+ String(slot);
  int charCount = 0;

  delay(100);
  Serial1.println(CMD);                                                        // prints cmd which prints out message received.
  delay(100);

  while((Serial1.available())){                                                // finds first part of received message and reads it out.
    char c = Serial1.read();                                                    
    if((c == '\n') && (response.indexOf("+CMGR: \"REC UNREAD\"") >= 0))        // parses out the junk part of message.
      break;

    response += c;
    delay(1);
  }

  if(DEBUG){SerialUSB.println(response);}                                      // for debugging, prints out junk part of message.

  if(response.indexOf("OK") == -1) {                                           // if no message was received, only "OK" will be read.
    long int time = millis();
    boolean loop = true;

    while ((time + timeout) > millis() && loop){                               // stores actual message sent by SMS.
      while (Serial1.available()){
        char c = Serial1.read();
        if((c == '\n')){                                                       // only reads message up to the newline.
          loop = false;                                                        // breaks loop once message is read.
          break; 
        }

        message += c;
        delay(1);
      }//while
    }//while
    if(DEBUG){SerialUSB.println("New unread message: "+ message);}             // prints out message 
    
    return message; 

  }//if
  else
    if(DEBUG){SerialUSB.println("No unread messages");}

  clearSMS();
  return "";
}



/*Clears all old SMS messages*/
// parameters:
//  - debug, debugging purposes
void clearSMS(){
  String CMD = "";
  String response = "";

  for(int i = 0; i <= 49; i++){                                 // clears all 50 stored messages.
    CMD = "AT+CMGD=" + String(i);    
    sendCMD(CMD, 10, DEBUG);
    //Serial1.println(CMD);    
    delay(1);
    
    while (Serial1.available()){
      char c = Serial1.read();
      response += c;
    }//while

    if(response.indexOf("ERROR") != -1){                        // checks for errors in +CMGD cmd
      SerialUSB.println("Failed to clear messages");
      return;
    }//if
    
    if (DEBUG){                                                 // for debugging
      while(Serial1.available()){
        SerialUSB.write(Serial1.read());
        delay(1);
      }//while
    }//if
  }//for
}//function



/*Checks for any new SMS messages. Looks for new message prompt. Needs to be put in a loop.*/
// parameters:
//  - mode, dictates what kind of return the function will send.
//     mode = 0: Calls the checkSMS function.
//     mode = 1: Returns the readSMS message.
//  - debug, debugging stuff
String updateSMS(int mode){
  char* bufPtr = buffer;
  String message;
  
  if(Serial1.available()){
    int slot = 0;

    do {                                                            // reads SIM7600 Serial.
      *bufPtr = Serial1.read(); 
      
      if(DEBUG){SerialUSB.write(*bufPtr);} 
      delay(1);
    } while((*bufPtr++ != '\n') && (Serial1.available()));         // reads until a new line or until there is nothing left to read.

    *bufPtr = 0;
    if (1 == (sscanf(buffer, "+CMTI: \"SM\",%d", &slot))){         // looks for new message prompt.
      
      if(DEBUG){
        SerialUSB.print("slot: ");
        SerialUSB.println(slot);
      }

      message = readSMS(1000, slot);                               // if found, calls readSMS and stores messsage in message variable.

      if(mode == 0){                                               // checks the message for command.
        if(!checkSMS(message, slot,DEBUG))                         // WILL CHANGE ONCE PROTOTYPE IS DONE
          if(DEBUG){SerialUSB.println("NOT 0, 1, 2, 3");}
      }//if

      else if(mode == 1){                                          // simply returns the message received.
        return message;
      }//else if
    }//if

    /*DEBBUGGING*/
    if(DEBUG){SerialUSB.write(bufPtr);}
    
  }//if
  Serial1.flush();                                                 
  return "";                                                       // return nothing if new message notif not found.
}



/*Sends command too SIM7600 Serial*/
// parameters: 
//  - cmd, command you would like to send.
//  - timeout, time before loop gives up sending the command.
//  - debug, debugging stuff.
String sendCMD(String cmd, const int timeout, boolean debug){
    String response = "";
    Serial1.flush();

    Serial1.println(cmd);                                           // prints command to SIM7600 Serial.

    long int time = millis();
    while ((time + timeout) > millis()){                            // reads any error or OK responses spat out from the SIM7600 Serial.
      while (Serial1.available()){
        char c = Serial1.read();
        response += c;
      }//while
    }//while

    /*DEBBUGGING*/
    if (debug){
        SerialUSB.print(response);
    }

    return response;                                                // returns SIM7600 response for debugging.
}



/*Looks through SMS message for certain prompts. WILL NEED TO MODIFY*/
// parameters: 
//  - message, message parsed from the user's SMS.
//  - slot, message slot.
//  - debug, debugging stuff.
boolean checkSMS(String message, int slot, boolean debug){
  String newMessage;
  int ID;
  message.toLowerCase();
  
  if(message.indexOf("s") == 0){
    ID = getID(message, debug);
    if(ID == -1)
      return false;
    
    message.remove(0,3);
    SerialUSB.println("CMD Type: "+ message);
    
    if(message.indexOf("status") == 0){                                  // checks for "status" cmd. 
      if(debug){SerialUSB.println("SENDING STATUS");}
      Status(device[ID-1], DEBUG);                                       // calls Status() which returns a devices' status.
      return true;
    }//if

    else if(message.indexOf("configure") == 0){                          // checks for "configure" cmd.
      if(debug){SerialUSB.println("SENDING CODE 1");}
      device[ID-1] = ChangeConfig(device[ID-1], DEBUG);                  // calls 
      return true;
    }      
      
    else if(message.indexOf("disarm") == 0){                             // checks for "2".
      if(debug){SerialUSB.println("SENDING CODE 2");}
      device[ID-1] = Disarm(device[ID-1]);
      return true;
    }

    else if(message.indexOf("arm") == 0){                                // checks for "alarming"
      if(debug){SerialUSB.println("SENDING CODE 3");}
      device[ID-1] = AlarmOn(device[ID-1]);
      return true;
    }

    else{
      sendSMS("ERROR: Unknown command.\nPlease type \"help\" for list of valid commands.");
      return false;
    }
  }// if(message.indexOf("s") == 0)


  else if(message.indexOf("help") == 0){
    Help(debug);
  }// else if


  else{                                                             // if message none of the above, function returns false which signals
    /*DEBUGGING*/                                                   // that nothing of value was received. WILL CHANGE TO DELETE MESSAGES (MAYBE).
    if(debug)
      SerialUSB.println("NOT AN OPTION");


    message.trim();
    sendSMS("\""+ message +"\" is not a known command. Please type \"help\" for a list of commands.");    
    return false;
  }// else
}



/*  Gets the status of specified device. 
    NEED TO PRINT MORE STATUS STUFF. **CHANGE THIS LATER**
*/
void Status(Sensor device, boolean debug){
  String message;
  String config;                                                        // going to change.
  config = CurrConfig(device, debug);

  message = ("----- Status -----\nDevice ID: "+ String(device.ID) 
            +"\nName: "+ device.name 
            +"\nStatus: "+ (device.status ? "ACTIVE" : "INACTIVE")
            +"\nLast Updated: "+ device.datetime
            +"\nBattery Level: "+ device.BatLevel +"%");
  sendSMS(message);
  SerialUSB.println(message);
  
  message = CurrConfig(device, debug);
  sendSMS(message);

  SerialUSB.println(message);
}



/**/
Sensor ChangeConfig(Sensor device, boolean debug){
  String message;
  String userResponse = "";
  const int timeout = 600000;
  long int time = millis();
  int loop = 1;

  time = time + timeout;

  if(device.configured){
    sendSMS("This device has already been configured and cannot be modified. Please disarm the device and manually reboot it to reconfigure.");
    return device;
  }

  sendSMS("----- Configuring s"+ String(device.ID) +" -----\nIF LEFT UNCONFIGURED FOR MORE THAN 10 MINUTES, S"+ String(device.ID) +" WILL POWER OFF.");
  while(time > millis() || loop){

    sendSMS("What is the address of the installation of s"+ String(device.ID) +"?");

    while(userResponse.equals("") && time > millis())
      userResponse = updateSMS(1);

    userResponse.trim();
    device.name = userResponse;

    sendSMS("Activate TILT SENSOR? y/n");
    userResponse = getYN(time);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.indexOf("y") == 0)
      device.tilt = ON;
    sendSMS("OK.");                                                          //THIS "OK" STUFF MIGHT NEED TO CHANGE!
    delay(100);
    
    sendSMS("Activate LIGHT SENSOR? y/n");
    userResponse = getYN(time);
    if(userResponse.equals("NORESPONSE"))
      break;
    
    if(userResponse.equals("y"))
      device.light = ON;
    sendSMS("OK.");
    delay(100);

    sendSMS("Activate CONDUCTIVITY SENSOR? y/n");
    userResponse = getYN(time);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.equals("y"))
      device.conductivity = ON;
    sendSMS("OK.");
    delay(100);

    message = CurrConfig(device, debug);
    sendSMS(message);

    sendSMS("Would you like to make any changes? y/n");
    userResponse = getYN(time);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.equals("n")){
      sendSMS("S"+ String(device.ID) +" Configuration Saved.");
      sendSMS("----- CMD List -----\ns# status\ns# configure\ns# disarm\ns# arm\nhelp");
      device.configured = 1;
      clearSMS();                 
      return device;
    }//if
    sendSMS("RECONFIGURING...");
    device.conductivity = OFF; device.light = OFF; device.tilt = OFF;
    userResponse = "";
  }//while
  sendSMS("S"+ String(device.ID) +" configuration canceled. Proccess timed out.");
  return device;
}//function



/*  Pushes the config made by the user and returns 
    message to send to user by SMS. 
    SAME CHANGES TO BE MADE                               */
String CurrConfig(Sensor device, boolean debug){
  String message;
  message = ("S"+ String(device.ID) 
            +" CURRENT SENSOR CONFIG:\n----- Config -----\nInstall Address: "+ device.name 
            +"\nTilt Sensor = "+ (device.tilt ? "ON" : "OFF") 
            +"\nLight Sensor = "+ (device.light ? "ON" : "OFF")
            +"\nConductivity Sensor = "+ (device.conductivity ? "ON" : "OFF"));

  return message;
}// function



/*  Sets the device to command requesting mode which 
    just waits for the user to tell the device what state
    it should go in.
    SAME CHANGES TO BE MADE                               */
Sensor ReqCommand(String cmd, Sensor device, boolean debug){
  String message = "";
  sendSMS("Type \"silent\" for silent mode or \"sleep\" for sleep mode");
  
  while(message == "")
    message = updateSMS(1);

  if(message.indexOf("silent") == 0 || message.indexOf("Silent") == 0){
    sendSMS("Device set to SILENT.");                                     // *IMPORTANT* make this actually update state to silent mode
    device.status = INACTIVE;
    device.state = GoSilent;
  }

  if(message.indexOf("sleep") == 0 || message.indexOf("Sleep") == 0){
    sendSMS("Device set to SLEEP.");
    device.status = INACTIVE;
    device.state = GoToSleep;
  }

  return device;

}



/*  Sets the device to ALARMING mode.
    SAME CHANGES TO BE MADE                               */
Sensor AlarmOn(Sensor device){
  String message;

  if(!device.configured){
    sendSMS("S"+ String(device.ID) +" has not been configured yet. Arming canceled.");
    return device;
  }
  device.status = ACTIVE;
  device.state = Armed;

  // SEND COMMAND TO ARM TO RADIO
  loadPayload(device, LoadParms);
  if(!sendPayload(address)){
    sendSMS("Failed to load config.");
    return device;  
  }
  delay(1000);
  loadPayload(device, GoToArm);
  if(!sendPayload(address)){
    sendSMS("Failed to arm.");
    return device;
  }
  delay(2000);
  if(Message.State == CantArm){ 
    sendSMS("Failed to activate sensors. Make sure sensors are not activated while arming.");
    loadPayload(device, GoToSleep);
    if(!sendPayload(address)){  sendSMS("EVERYTHING IS FALLING APART AAAAAAAAAAAAA"); return device;}
  }

  sendSMS("s"+ String(device.ID) +" is now ARMED.");
  acknowledge = false;
  return device;
}



Sensor Disarm(Sensor device){
  String message;

  if(!device.configured){ 
    sendSMS("S"+ String(device.ID) +" has not been configured yet. Disarm canceled.");
    return device;
  }

  sendSMS("----- DISARMING -----\nDevice: s"+ String(device.ID) 
         +"\nAddress: "+ device.name 
         +"\nAre you sure you would like to disarm s"+ String(device.ID) +"? (y or n)");

  while(1){       //MAKE THIS TIMEOUT
    message = getYN(600000);

    if(message.equals("y")){
      device.state = GoToSleep;
      device.status = INACTIVE;
      device.light = OFF;
      device.tilt = OFF;
      device.conductivity = OFF;
      device.configured = 0;

      // TELL SENSOR MODULE TOO DISARM.
      sendSMS("s"+ String(device.ID) +" disarmed.");
      break;
    }

    else if(message.equals("n")){
      sendSMS("Disarm canceled");
      break;
    }

    else if(message.equals("NORESPONSE")){
      sendSMS("Disarm canceled. Process timed out.");
      break;
    }
  }
  if(DEBUG){SerialUSB.println("DISARMING COMPLETE");} 
  return device;
}



/*Gives all devices an ID*/ 
void setDeviceID(){
  for(int i = 0; i <= 3; i++){
    device[i].ID = i+1;
    if (DEBUG){SerialUSB.println("Device "+ String(i) +" ID: "+ String(device[i].ID));}
  }
}



String getDateTime(){
  String date = "";
  int timeout = 1000;
  int count;

  date = sendCMD("AT+CCLK?", 1000, DEBUG);
  delay(100);

  if(date.indexOf("ERROR") != -1){
    SerialUSB.println("ERROR GETTING DATE!");
    return "";
  }

  int first = date.indexOf("\"");
  int last = date.indexOf("\"");
  date.remove(0, first+1);
  date.remove(last-1, 20);
  date.trim();

  if (DEBUG){SerialUSB.println(date);}

  return date;
}



int getID(String message, boolean debug){
  int ID;

  message.remove(0,1);
  message.remove(1, 20);
  SerialUSB.println("ID: "+ message);
  ID = message.toInt();

  if(1 <= ID && ID <= 4){
    return ID;
  }
  sendSMS("s"+ String(ID) +" is not a device!");
  return -1;
}



String getYN(int time){
  int x = 1;
  String response = "";

  while(time > millis()){
    response = updateSMS(1);
    response.toLowerCase();

    if(response.equals(""))
      continue;

    else if(response.indexOf("y") == 0)
      return "y";

    else if(response.indexOf("n") == 0)
      return "n";

    else{
      sendSMS("That is not an option. Please type either \"y\" or \"n\"");
      Serial1.flush();
    }

  delay(1);
  }
  return "NORESPONSE";
}



void Help(boolean debug){
  sendSMS("----- Help List -----");  
  sendSMS("Command Type: status\n USAGE EXAMPLE: s# status");
  sendSMS("Command Type: configure\n USAGE EXAMPLE: s# configure");
  sendSMS("Command Type: disarm\n USAGE EXAMPLE: s# disarm");
  sendSMS("Command Type: arm\n  Usage Example: s# arm");
  sendSMS("Type \"help\" if you ever need this list of commands again.");
}



void Alarm(Sensor device){
  int count = 1800000;
  int loops = 0;
  

  while(!(acknowledge) && loops < 3){
    count++;
    if(updateSMS(1) != ""){
      SerialUSB.println("Acknowledgement Received");
      sendSMS("ALERT ACKNOWLEDGED.");
      acknowledge = true;
      device.state = Asleep;
      loadPayload(device, GoToSleep);
      sendPayload(address);
      return;
    }

    if(count >= 1800000){
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
    }//if
  delay(1);
  }//while
}



Sensor storeStatus(Sensor device){
  device.datetime = getDateTime();
  device.state = Message.State;
  device.BatLevel = Message.BatLevel;
  device.tilt = Message.Sensor1;
  device.light = Message.Sensor2;
  device.conductivity = Message.Sensor3;
  return device;
}



void setDeviceAddress(){
  int ID;

  for(int i = 0; i <= 3; i++){
    ID = i+1;
    char c = ID + '0';
    char new_address[6] = {'0','0','0','0', c};
    //strcpy(device[i].address, new_address);
    strcpy(device[i].address, new_address);
    if(DEBUG){ SerialUSB.println("Device "+ String(ID) +" address: "+ String(device[i].address));}
  }
}