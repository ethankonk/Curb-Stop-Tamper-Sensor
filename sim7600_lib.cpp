#include <string.h>
#include <stdio.h>
#include <time.h>
#include <Arduino.h>

#include "sim7600_lib.h"
#include "NRTF_lib.h"


/*Sends SMS message too phone*/
// parameters: 
//  - String message
boolean sendSMS(String message){
  unsigned int timeout = 10000;
  unsigned long int time = millis();
  timeout = time + timeout;

  sendCMD(cmd, 1000, DEBUG);                                    // sends " AT+CMGS= "+12269357857" " (Will have to make function to change phone#)
  delay(100);

  Serial1.print(message);                                       // print message to send to SIM7600 Serial
  delay(500);
  Serial1.write(26);                                            // ASCII code for Ctrl-Z

  String serial = "";
  while((serial.indexOf("+CMGS: ") < 0) && (serial.indexOf("ERROR") < 0)){                         // delay that just makes the code work
    if(Serial1.available()) 
      serial += char(Serial1.read());
    delay(1);
    if(timeout < millis()){
      SerialUSB.println("VERY POOR CONNECTION!");
      timeout += 10000;
    }
  }

  SerialUSB.println(serial);

  String response = "";
  while(Serial1.available() && response.indexOf("OK") < 0)      // look for OK response
    response += char(Serial1.read());


  if(response.indexOf("ERROR") != -1){
    if (DEBUG)
      SerialUSB.println(response);
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
String readSMS(const int timeout, int slot, boolean debug){

  String response = "";
  String message = "";
  String CMD = "AT+CMGR="+ String(slot);
  int charCount = 0;

  delay(100);
  Serial1.println(CMD);                                                         // prints cmd which prints out message received.
  delay(100);

  while((Serial1.available())){                                                 // finds first part of received message and reads it out.
    char c = Serial1.read();                                                    
    if((c == '\n') && (response.indexOf("+CMGR: \"REC UNREAD\"") >= 0))         // parses out the junk part of message.
      break;

    response += c;
    delay(1);
  }

  /*DEBUGGING*/ 
  if(debug){                                                                    // for debugging, prints out junk part of message.
    SerialUSB.println(response);
    SerialUSB.println("*****************");
  }

  if(response.indexOf("OK") == -1) {
    long int time = millis();
    boolean loop = true;

    while ((time + timeout) > millis() && loop){                               // stores actual message sent by SMS into String message
      while (Serial1.available()){
        char c = Serial1.read();
        if((c == '\n')){                                                       // only reads message up to '\n'
          loop = false;                                                        // breaks loop once message is read.
          break; 
        }

        message += c;
        delay(1);
      }//while
    }//while
    SerialUSB.println("New unread message: "+ message);                        // prints out message 
    
    return message; 

  }//if
  else
    SerialUSB.println("No unread messages");

  clearSMS(DEBUG);
  return "";
}



/*Clears all old SMS messages*/
// parameters:
//  - debug, debugging purposes
void clearSMS(boolean debug){
  String CMD = "";
  String response = "";

  for(int i = 0; i <= 49; i++){                                 // clears all 50 stored messages.
    CMD = "AT+CMGD=" + String(i);    
    sendCMD(CMD, 10, debug);
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
    
    if (debug){                                                 // for debugging
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
String updateSMS(int mode, boolean debug){
  char* bufPtr = buffer;
  String message;
  
  if(Serial1.available()){
    int slot = 0;

    do {                                                            // reads SIM7600 Serial.
      *bufPtr = Serial1.read(); 
      
      /*DEBBUGGING*/
      if(debug)
        SerialUSB.write(*bufPtr); 
      
      delay(1);
    } while((*bufPtr++ != '\n') && (Serial1.available()));         // reads until a new line or until there is nothing left to read.

    *bufPtr = 0;
    if (1 == (sscanf(buffer, "+CMTI: \"SM\",%d", &slot))){         // looks for new message prompt.
      
      /*DEBUGGING*/
      if(debug){
        SerialUSB.print("slot: ");
        SerialUSB.println(slot);
      }

      message = readSMS(1000, slot, DEBUG);                         // if found, calls readSMS and stores messsage in message variable.      
      if(mode == 0){                                                // mode 0.
        if(!checkSMS(message, slot,DEBUG)){                         // WILL CHANGE ONCE PROTOTYPE IS DONE
          
          /*DEBUGGING*/
          if(debug)
            SerialUSB.println("NOT 0, 1, 2, 3");
        }//if
      }//if

      else if(mode == 1){                                           // mode 1.
        return message;
      }//elseif
    }//if

    /*DEBBUGGING*/
    if(debug)
      SerialUSB.write(bufPtr);
    
  }//if
  Serial1.flush();                                                  // flush SIM7600 Serial.
  return "";                                                        // return nothing if new message notif not found.
}



/*Sends command too SIM7600 Serial*/
// parameters: 
//  - cmd, command you would like to send.
//  - timeout, time before loop gives up sending the command.
//  - debug, debugging stuff.
String sendCMD(String cmd, const int timeout, boolean debug){
    String response = "";
    Serial1.flush();

    Serial1.println(cmd);                                           // prints requested command to SIM7600 Serial.

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

    return response;                                                // returns SIM7600 response for error handeling purposes.
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
    
    //input is 0
    if(message.indexOf("status") == 0){                                      // checks for "status" cmd. 

      /*DEBUGGING*/
      if(debug)  
        SerialUSB.println("SENDING STATUS");

      Status(device[ID-1], DEBUG);                          // calls Status() which returns a devices' status.
      return true;
    }//if

      // input is 1
    else if(message.indexOf("configure") == 0){                           // checks for "configure" cmd.

      /*DEBUGGING*/
      if(debug)  
        SerialUSB.println("SENDING CODE 1");

      device[ID-1] = ChangeConfig(device[ID-1], DEBUG);                      // calls 
      return true;
    }      
      
      // input is 2
    else if(message.indexOf("disarm") == 0){                               // checks for "2".

      /*DEBUGGING*/
      if(debug)
        SerialUSB.println("SENDING CODE 2");

      device[ID-1] = Disarm(device[ID-1], DEBUG);
      return true;
    }

      //input is 3
    else if(message.indexOf("arm") == 0){                               // checks for "alarming"
      /*DEBUGGING*/
      if(debug)
        SerialUSB.println("SENDING CODE 3");
      device[ID-1] = AlarmOn(device[ID-1], DEBUG);
      return true;
    }

      // input is not on list. 
    else{
      sendSMS("ERROR: Unknown command.\nPlease type \"help\" for list of valid commands.");
      return false;
    }
  }// if(message.indexOf("s") == 0)


  else if(message.indexOf("help") == 0){
    Help(debug);
  }// else if


  else if(message.indexOf("example") == 0){
    message.remove(0, 8);
    Example(message, debug);
  }


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
  String config;                                        // going to change.
  config = CurrConfig(device, debug);

  message = ("----- Status -----\nDevice ID: "+ String(device.ID) 
            +"\nName: "+ device.name 
            +"\nStatus: "+ device.status 
            +"\nLast Updated: "+ device.datetime
            +"\nBattery Level: "+ device.BatLevel
            );
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

    while(userResponse.equals(""))
      userResponse = updateSMS(1, debug);

    device.name = userResponse;

    sendSMS("Activate TILT SENSOR? y/n");
    userResponse = getYN(time, debug);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.indexOf("y") == 0)
      device.tilt = ON;
    sendSMS("OK.");                                                          //THIS "OK" STUFF MIGHT NEED TO CHANGE!
    delay(100);
    
    sendSMS("Activate LIGHT SENSOR? y/n");
    userResponse = getYN(time, debug);
    if(userResponse.equals("NORESPONSE"))
      break;
    
    if(userResponse.indexOf("y") == 0)
      device.light = ON;
    sendSMS("OK.");
    delay(100);

    sendSMS("Activate CONDUCTIVITY SENSOR? y/n");
    userResponse = getYN(time, debug);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.indexOf("y") == 0)
      device.conductivity = ON;
    sendSMS("OK.");
    delay(100);

    message = CurrConfig(device, debug);
    sendSMS(message);

    sendSMS("Would you like to make any changes? y/n");
    userResponse = getYN(time, debug);
    if(userResponse.equals("NORESPONSE"))
      break;

    if(userResponse.indexOf("n") == 0){
      sendSMS("S"+ String(device.ID) +" Configured.");                   // IMPORTANT** MAKE SURE THIS PUSHES THE CONFIG TOO THE SENSOR
      device.configured = 1;                                             // VERY IMPORTANT LATER ^^^
      return device;
    }//if
    sendSMS("RECONFIGURING...");
    userResponse = "";
  }//while
  sendSMS("s"+ String(device.ID) +" POWERING OFF.");
  return device;
}//function



/*  Pushes the config made by the user and returns 
    message to send to user by SMS. 
    SAME CHANGES TO BE MADE                               */
String CurrConfig(Sensor device, boolean debug){
  String message;
  message = ("S"+ String(device.ID) + " CURRENT SENSOR CONFIG:\n----- Config -----\nInstall Address: "+ device.name +"\nTilt Sensor = "+ (device.tilt ? "ON" : "OFF") + "\nLight Sensor = "+ (device.light ? "ON" : "OFF")
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
    message = updateSMS(1, debug);

  if(message.indexOf("silent") == 0 || message.indexOf("Silent") == 0){
    sendSMS("Device set to SILENT.");                                     // *IMPORTANT* make this actually update state to silent mode
    device.status = "SILENT";
  }

  if(message.indexOf("sleep") == 0 || message.indexOf("Sleep") == 0){
    sendSMS("Device set to SLEEP.");
    device.status = "INACTIVE";
  }

  return device;

}



/*  Sets the device to ALARMING mode.
    SAME CHANGES TO BE MADE                               */
Sensor AlarmOn(Sensor device, boolean debug){
  String message;
  sendSMS("s"+ String(device.ID) +" is now ARMED.");

  device.status = "ACTIVE";
  device.state = Armed;
  
  return device;

}



Sensor Disarm(Sensor device, boolean debug){
  String message;
  int x = 1;

  sendSMS("----- DISARMING -----\nDevice: s"+ String(device.ID) +"\nAddress: "+ device.name + "\nAre you sure you would like to disarm s"+ String(device.ID) +"? (y or n)");

  while(x){
    message = getYN(1000, debug);

    if(message.indexOf("y") == 0){
      device.state = NullState;
      device.status = "INACTIVE";
      device.light = 0;
      device.tilt = 0;
      device.conductivity = 0;
      device.name = "";
      device.configured = 0;

      sendSMS("s"+ String(device.ID) +" disarmed.");
      x = 0;
    }
    else if(message.indexOf("n") == 0){
      sendSMS("Disarm canceled");
      x = 0;
    }
  }

  if(debug)
    SerialUSB.println("DISARMING COMPLETE");
  
  return device;
}



/*Gives all devices an ID*/ 
void setDeviceID(boolean debug){
  for(int i = 0; i < 8; i++){
    device[i].ID = i+1;

    if (debug)
      SerialUSB.println("Device "+ String(i) +" ID: "+ String(device[i].ID));
  }
}



String getDateTime(boolean debug){
  String date = "";
  int timeout = 1000;
  int count;

  date = sendCMD("AT+CCLK?", 1000, debug);
  delay(100);

  if(date.indexOf("ERROR") != -1){
    SerialUSB.println("ERROR GETTING DATE!");
    return "";
  }

  date.remove(0, 25);
  date.remove(17, 20);

  if (debug)
    SerialUSB.println(date);

  return date;
}



int getID(String message, boolean debug){
  int ID;

  message.remove(0,1);
  message.remove(1, 20);
  SerialUSB.println("ID: "+ message);
  ID = message.toInt();

  if(0 < ID && ID < 8){
    return ID;
  }
  sendSMS("s"+ String(ID) +" is not a device!");
  return -1;
}



String getYN(int time, boolean debug){
  int x = 1;
  String response = "";

  while(time > millis()){
    response = updateSMS(1, debug);

    if(response.equals(""))
      continue;

    else if(response.lastIndexOf("y") == 0 || response.lastIndexOf("Y") == 0)
      return "y";

    else if(response.lastIndexOf("n") == 0 || response.lastIndexOf("N") == 0)
      return "n";

    else{
      sendSMS("That is not an option. Please type either \"y\" or \"n\"");
      Serial1.flush();
    }

  delay(1);
  }
  return "NORESPONSE";
}



/*OLD FUNCTION FOR GETTING READYING THE CURB UNIT*/
// boolean getReadyInstall(Sensor device, boolean debug){
//   long int time = millis();
//   const long int timeout = 1800000;
//   String response = "";
//   int loop = 1;

//   sendSMS("Changes Saved! Type \"r\" when s"+ String(device.ID) +" is succesfully installed.\nIF NO RESPONSE IS RECIEVED WITHIN 30 MINUTES S"+ String(device.ID) + " WILL POWER OFF.");
//   while((time + timeout) > millis() && loop){
//     response = updateSMS(1, debug);

//     if(response.equals(""))
//       continue;
    
//     else if(response.indexOf("r") == 0 || response.indexOf("R") == 0){
//       sendSMS("Curb Unit Armed.");
//       SerialUSB.println("THINGY WORKS :)");
//       return true;
//     }
//     else{
//       sendSMS("Please respond with \"r\" to signal the Unit is installed and ready to be activated.");
//     }
//   }// while
//   return false;
// }



void Help(boolean debug){
  sendSMS("----- Help List -----");  
  sendSMS("Command Type: status\n USAGE EXAMPLE: s# status");
  sendSMS("Command Type: configure\n USAGE EXAMPLE: s# configure");
  sendSMS("Command Type: disarm\n USAGE EXAMPLE: s# disarm");
  sendSMS("Type \"help\" if you ever need this list of commands again.");
}



void Alarm(Sensor device, boolean debug){
  int count = 0;
  int loops = 0;
  int acknowledge = false;

  while(!acknowledge && loops < 3){
    count++;
    if(updateSMS(1, debug) != ""){
      SerialUSB.println("Acknowledgement Received");
      sendSMS("ALERT ACKNOWLEDGED.");
      acknowledge = true;
      return;
    }

    if(count == 1800000){
      sendSMS("<<ALERT>>\nDevice ID: "+ String(device.ID) + " has been tampered with!\nInstall Address: "+ device.name +
              "\nSensors Triggered: \nDate/Time: "+ device.datetime +"\nDevice Status: "+ device.status+ "\n\nTo stop alerts please type \"OK\"");
      count = 0;
      loops++;
    }//if
  delay(1);
  }//while
}



void Example(String message, boolean debug){

  if(message.indexOf("status") == 0)
    sendSMS("An example of how the \"status\" command is used is:\ns1 status\n  - The \"1\" being the ID of the unit you would like to see the status of.");
  
  else if(message.indexOf("configure") == 0)
    sendSMS("An example of how the \"configure\" command is used is:\ns4 configure\n  - The \"4\" being the ID of the unit you would like to configure.");
}



Sensor storeStatus(Sensor device){
  device.datetime = getDateTime(DEBUG);
  device.state = Message.State;
  device.BatLevel = Message.BatLevel;
  device.Sensor1 = Message.Sensor1;
  device.Sensor2 = Message.Sensor2;
  device.Sensor3 = Message.Sensor3;
}