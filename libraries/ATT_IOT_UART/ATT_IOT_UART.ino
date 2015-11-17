
#include "ATT_IOT_UART.h"                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
/*
  SmartLiving Makers Arduino Demo Sketch
  version 2.0 dd 25/02/2015
  
  This file is an example sketch to deploy a digital sensor in the SmartLiving.io IoT platform.
  
  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - connect an analog sesnor to Pin D8 of the Arduino shield (example push button)

  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting


*/

// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the smartliving.io website 

char deviceId[] = "devId"; // Your device id comes here
char clientId[] = "clientId"; // Your client id comes here;
char clientKey[] = "clientKey"; // Your client key comes here;


ATTDevice Device(&Serial);         
char httpServer[] = "api.smartliving.io";                   // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";		    // MQTT Server Address


// Define PIN numbers for assets

int DigitalSensor = 8;                                        // Digital Sensor is connected to pin D8 on grove shield 

//required for the device
void callback(int pin, String& value);

void setup()
{
  pinMode(DigitalSensor, INPUT);					            // initialize the digital pin as an input.          
  Serial.begin(9600);							            // init serial link for debugging
  
  while(!Device.Init(deviceId, clientId, clientKey))
    Serial.println("failed to init device, retrying");
  Device.StartWifi("ssid", "pwd");
    
  while(!Device.Connect(httpServer))					            // connect the device with the IOT platform.
    Serial.println("failed to connect to http server, retrying");
  Device.AddAsset(DigitalSensor, "YourDigitalSensorname", "Digital Sensor Description", false, "boolean");   // Create the Digital Sensor asset for your device
  Device.Subscribe(mqttServer, callback);                        // make certain that we can receive message from the iot platform (activate mqtt)
}


bool sensorVal = false;							        //only send every x amount of time.
void loop()
{
  bool sensorRead = digitalRead(DigitalSensor);			        // read status Digital Sensor
  if (sensorVal != sensorRead) 				                // verify if value has changed
  {
    sensorVal = sensorRead;
    delay(100);
    if (sensorRead == 1)
    {
       Device.Send("true", DigitalSensor);
    }
    else
    {
       Device.Send("false", DigitalSensor);
    }
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
	if(pin == DigitalSensor)
	{
		//activate 
	}
    Device.Send(value, pin);    
}

