/*
   Copyright 2014-2017 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include <WiFi.h>

#include <PubSubClient.h>

#include <ATT_IOT.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.

/*
  AllThingsTalk Makers intel edison Example 

  ### Instructions

  - in arduino ide, make certain that the intel edison is added to the boards manager.
  - in older versions of the boards manager for the edison, there is a line missing in wstring.h. This needs
    to be added before you can continue, otherwise, the code wont compile. (valid on 1/3/2017)
	file: {local app data}\arduino15\packages\intel\hardware\i686\1.6.7+1.0\cores\arduino\wstring.h
	line: 163
	add: const char * c_str() const { return buffer; }

*/

char deviceId[] = "YOUR_DEVICE_ID_HERE";
char clientId[] = "YOUR_CLIENT_ID_HERE";
char clientKey[] = "YOUR_CLIENT_KEY_HERE";

char ssid[] = "YOUR_SSID"; //  your network SSID (name) 
char pass[] = "YOUR_PWD";    // your network password (use for WPA, or use as key for WEP)

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
#define httpServer "api.AllThingsTalk.io"                  // HTTP API Server host                  
#define mqttServer httpServer                				// MQTT Server Address 

int status = WL_IDLE_STATUS;

int knobPin = 0;                                            // Analog 0 is the input pin + identifies the asset on the cloud platform
int ledPin = 8;                                             // Pin 8 is the LED output pin + identifies the asset on the cloud platform

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  pinMode(ledPin, OUTPUT);                                  // initialize the digital pin as an output.         
  Serial.begin(9600);                                       // init serial link for debugging
  
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 
  
  String fv = WiFi.firmwareVersion();
  if( fv != "1.1.0" )
    Serial.println("Please upgrade the firmware");
  
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:    
    status = WiFi.begin(ssid, pass);
  
    // wait 10 seconds for connection:
    delay(10000);
  } 
  Serial.println("Connected to wifi");
  
  while(!Device.Connect(&ethClient, httpServer))            // connect the device with the IOT platform.
    Serial.println("retrying");
  Device.AddAsset(knobPin, "knob", "rotary switch",false, "{\"type\": \"integer\", \"minimum\": 0, \"maximum\": 1023}");
  Device.AddAsset(ledPin, "led", "light emitting diode", true, "boolean");
  while(!Device.Subscribe(pubSub))                          // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying"); 
}

unsigned long _time;                                         //only send every x amount of time.
unsigned int prevVal =0;
void loop()
{
  unsigned long curTime = millis();
  if (curTime > (_time + 1000))                              // publish light reading every 5 seconds to sensor 1
  {
    unsigned int lightRead = analogRead(knobPin);           // read from light sensor (photocell)
    if(prevVal != lightRead){
      Device.Send(String(lightRead), knobPin);
      prevVal = lightRead;
    }
    _time = curTime;
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                     //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    char message_buff[length + 1];                      //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
    strncpy(message_buff, (char*)payload, length);      //copy over the data
    message_buff[length] = '\0';                        //make certain that it ends with a null         
          
    msgString = String(message_buff);
    msgString.toLowerCase();                            //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {                                                     //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    int pinNr = Device.GetPinNr(topic, strlen(topic));
    
    Serial.print("Payload: ");                          //show some debugging.
    Serial.println(msgString);
    Serial.print("topic: ");
    Serial.println(topic);
    
    if (pinNr == ledPin)       
    {
      if (msgString == "false") {
        digitalWrite(ledPin, LOW);                      //change the led    
        idOut = &ledPin;                                
      }
      else if (msgString == "true") {
        digitalWrite(ledPin, HIGH);
        idOut = &ledPin;
      }
    }
  }
  if(idOut != NULL)                                     //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
