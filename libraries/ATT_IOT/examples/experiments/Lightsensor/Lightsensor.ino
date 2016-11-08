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

#include <Ethernet2.h>
#include <EthernetClient.h>

#include <PubSubClient.h>

#include <ATT_IOT.h> // AllThingsTalk for Makers Arduino Library
#include <SPI.h>                                //required to have support for signed/unsigned long type.               

/*
  AllThingsTalk Makers Arduino Experiment
  version 1.0 09/10/2014
  
  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Grove Light sensor to A0
  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting


*/


char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here;
char clientKey[] = ""; // Your client key comes here;

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 

int lichtsensor = 0;                                            // Analog 0 is the input pin, this corresponds with the number on the Grove shiled where the Lightsensor is attached to, it's also used to construct the assetID

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  Serial.begin(9600);                                           // init serial link for debugging
  
  byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0x8D, 0x3D };         // Adapt to your Arduino MAC Address  
  
  if (Ethernet.begin(mac) == 0)                                 // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                                //we failed to connect, halt execution here. 
  }
  delay(1000);                                                  //give the Ethernet shield a second to initialize:  
  while(!Device.Connect(&ethClient, httpServer))                // connect the device with the IOT platform.
    Serial.println("retrying");
  Device.AddAsset(lichtsensor, "Lightsensor", "light sensor", false, "integer");         // Create the asset for your device
  while(!Device.Subscribe(pubSub))                              // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying"); 
}

unsigned long time;                                 //only send every x amount of time.
void loop()
{
  unsigned long curTime = millis();
  if (curTime > (time + 5000))                          // publish light reading every 5 seconds to sensor 1
  {
    unsigned int lightRead = analogRead(lichtsensor);                   // read from light sensor (photocell)
    Device.Send(String(lightRead), lichtsensor);
    time = curTime;
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                            //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    char message_buff[length + 1];                        //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
    strncpy(message_buff, (char*)payload, length);        //copy over the data
    message_buff[length] = '\0';                      //make certain that it ends with a null           
          
    msgString = String(message_buff);
    msgString.toLowerCase();                  //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  char* idOut = NULL;
  {                                                       //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    int topicLength = strlen(topic);
    
    Serial.print("Payload: ");                            //show some debugging.
    Serial.println(msgString);
    Serial.print("topic: ");
    Serial.println(topic);
    
  }
  if(idOut != NULL)                //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
