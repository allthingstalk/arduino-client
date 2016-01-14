/*
  SmartLiving Makers Arduino UART Demo Sketch
  version 1.0 dd 25/11/2015
  
  This file is an example sketch to deploy a digital sensor and actuator in the SmartLiving.io IoT platform.
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield
    - Connect USB cable to your computer
    - connect a digital sesnor to Pin D8 of the Arduino shield (example push button)
    - connect a digital sensor to Pin 7
    - Grove wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch



*/

#include "ATT_IOT_UART.h"                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"

// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the smartliving.io website 

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address


// Define PIN numbers for assets
#define DigitalSensor 8                                        // Digital Sensor is connected to pin D8 on grove shield 
#define DigitalActuator 7

//required for the device
void callback(int pin, String& value);



void setup()
{
  pinMode(DigitalSensor, INPUT);                                // initialize the digital pin as an input.          
  pinMode(DigitalActuator, OUTPUT);                             // initialize the digital pin as an output.          
  Serial.begin(57600);                                          // init serial link for debugging
  while(!Serial);
  Serial.println("Starting sketch");
  Serial1.begin(115200);                                        //init software serial link for wifi
  while(!Serial1);
  
  while(!Device.StartWifi())
    Serial.println("retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))            //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.Connect(httpServer))                            // connect the device with the IOT platform. No point to continue if we can't succeed at this
    Serial.println("retrying");
  
  Device.AddAsset(DigitalSensor, "sensor", "Digital Sensor Description", false, "boolean");   // Create the Digital Sensor asset for your device
  Device.AddAsset(DigitalActuator, "acuator", "Digital Sensor Description", true, "boolean");   // Create the Digital Sensor asset for your device
  
  delay(1000);                                                  //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))                // make certain that we can receive message from the iot platform (activate mqtt). This stops the http connection
    Serial.println("retrying");
}


bool sensorVal = false;                                         //only send every x amount of time.
void loop()
{
  bool sensorRead = digitalRead(DigitalSensor);                 // read status Digital Sensor
  if (sensorVal != sensorRead)                                  // verify if value has changed
  {
    sensorVal = sensorRead;
    if (sensorRead == 1)
        Device.Send("true", DigitalSensor);
    else
        Device.Send("false", DigitalSensor);
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    if(pin == DigitalActuator)
    {
        Serial.print("incoming data for: ");               //display the value that arrived from the cloud.
        Serial.print(pin);
        Serial.print(", value: ");
        Serial.print(value);
        if(value == "true")
            digitalWrite(pin, HIGH);
        else    
            digitalWrite(pin, LOW);
        Device.Send(value, pin);                            //send the value back for confirmation
    }
   
}

