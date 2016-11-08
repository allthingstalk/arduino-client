/*
   Copyright 2014-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include <ESP8266WiFi.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type..

/*
  AllThingsTalk Makers Arduino Demo Sketch
  version 2.0 dd 25/02/2015
  
  This file is an example sketch to deploy a digital Actuator in the AllThingsTalk.io IoT platform.
  
  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - connect a Digital Actuator to Pin D4 of the Arduino shield (example relay)

  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting
  
  for PIN layout on the NodeMCU, check: https://github.com/esp8266/Arduino/blob/master/doc/boards.md#nodemcu-1-0


*/

// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the AllThingsTalk.io website 

char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here;
char clientKey[] = ""; // Your client key comes here;

const char* ssid     = "name of your network";
const char* password = "pwd for your network";


ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.com"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 


// Define PIN numbers & id's for assets

int DigitalActuator = D0;                                    // Digital Actuator is connected to pin D0 on nodemcu -> built in led
int DigitalActuatorId = 4;                                   // the pin numbers go above 9, so we use a separate id for the assets

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback,ethClient);  

void setup()
{      
  pinMode(DigitalActuator, OUTPUT);                             // initialize the digital pin as an output.         
  Serial.begin(9600);                                           // init serial link for debugging
  
  setup_wifi();
  delay(1000);                                                  //give the Ethernet shield a second to initialize:
  while(!Device.Connect(&ethClient, httpServer))                // connect the device with the IOT platform.
    Serial.println("retrying");
  Device.AddAsset(DigitalActuatorId, "YourDigitalActuatorname", "Digital Actuator Description", true, "boolean");   // Create the Digital Actuator asset for your device
  while(!Device.Subscribe(pubSub))                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying");                                                               
}


void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
                    
void loop()
{
  Device.Process(); 
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                           //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
  char message_buff[length + 1];                        //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation
  strncpy(message_buff, (char*)payload, length);        //copy over the data
  message_buff[length] = '\0';                  //make certain that it ends with a null     
      
  msgString = String(message_buff);
  msgString.toLowerCase();            //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {                                                       //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
      int pinNr = Device.GetPinNr(topic, strlen(topic));
  
      Serial.print("Payload: ");                            //show some debugging
      Serial.println(msgString);
      Serial.print("topic: ");
      Serial.println(topic);

      if (pinNr == DigitalActuatorId)  
      {
        if (msgString == "false") {
            digitalWrite(DigitalActuator, HIGH);        //change the actuator status to false -> the pin appears to be inversed, so set high
            idOut = &DigitalActuatorId;                           
        }
        else if (msgString == "true") {
            digitalWrite(DigitalActuator, LOW);              //change the actuator status to true  -> the pin appears to be inversed, so set high
            idOut = &DigitalActuatorId;
        }
      }  
  }
  if(idOut != NULL)                                           //Let the iot platform know that the operation was succesful
    Device.Send(msgString, *idOut);    
}


