
#include "ATT_IOT_UART.h"                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include <SoftwareSerial.h>
/*
  SmartLiving Makers Arduino UART Demo Sketch
  version 1.0 dd 25/11/2015
  
  This file is an example sketch to deploy a digital sensor and actuator in the SmartLiving.io IoT platform.
  
  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - connect a digital sesnor to Pin D8 of the Arduino shield (example push button)
    - connect a digital sensor to Pin 7
    - Grove UART wifi to pin D2

  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting


*/

// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the smartliving.io website 

char deviceId[] = "devId";                                      // Your device id comes here
char clientId[] = "clientId";                                   // Your client id comes here;
char clientKey[] = "clientKey";                                 // Your client key comes here;

SoftwareSerial wifi(2, 3);                                      // 2=RX, 3=TX
ATTDevice Device(&wifi);                  
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
  Serial.begin(9600);                                           // init serial link for debugging
  Serial.println("starting");
  wifi.begin(9600);                                             //init software serial link for wifi
  
  while(!Device.Init(deviceId, clientId, clientKey))            //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("failed to init device, retrying");
  Device.StartWifi("ssid", "pwd");          
    
  while(!Device.Connect(httpServer))                                // connect the device with the IOT platform. No point to continue if we can't succeed at this
    Serial.println("failed to connect to http server, retrying");
  Device.AddAsset(DigitalSensor, "sensor", "Digital Sensor Description", false, "boolean");   // Create the Digital Sensor asset for your device
  Device.AddAsset(DigitalActuator, "acuator", "Digital Sensor Description", true, "boolean");   // Create the Digital Sensor asset for your device
  delay(1000);                                                      //give the wifi some time to finish everything
  Device.Subscribe(mqttServer, callback);                           // make certain that we can receive message from the iot platform (activate mqtt). This stops the http connection
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
  delay(1000);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    if(pin == DigitalActuator)
    {
        Serial.print("incomming data for: ");               //display the value that arrived from the cloud.
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

