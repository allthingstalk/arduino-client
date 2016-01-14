/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the Ultrasonic Ranger (101020010) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect USB cable to your computer
    - Connect the Arduino Grove shield
    - connect the Ultrasonic Ranger sensor to PIN D7 of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, WIFI_SSID,WIFI_PWD) in the settings.h file. 
  4. Optionally, change sensor names, labels as appropiate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

*/

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           	//keep all your personal account information in a seperate file
#include "Ultrasonic.h"


ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address


// Define PIN numbers for assets
#define ultrasonicPin 7                                        // Ultrasonic ranger Sensor is connected to pin D7 on grove shield 
Ultrasonic ultrasonic(ultrasonicPin);

//required for the device
void callback(int pin, String& value);


void setup()
{          

  Serial.begin(57600);                                         // init serial link for debugging
  
  while (!Serial) ;                                            // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting sketch");
  Serial1.begin(115200);                                       //init serial link for wifi module
  while(!Serial1);
  
  while(!Device.StartWifi())
    Serial.println("retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))           //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.Connect(httpServer))                           // connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("retrying");
    
  Device.AddAsset(ultrasonicPin, "Ultrasonic sensor", "Ultrasonic Ranger sensor", false, "{\"type\": \"number\", \"minimum\": 0, \"unit\": \"cm\"}");   // Create the Sensor asset for your device
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
	Serial.println("retrying");
   
}

long prev_Value = 0;
void loop()
{
  long RangeInCentimeters;
  RangeInCentimeters = ultrasonic.MeasureInCentimeters();    // read Ultrasonic Ranger Sensor  
  if (RangeInCentimeters != prev_Value) {                            // verify if value has changed
    Serial.print("RangeInCentimeters = ");
    Serial.println(RangeInCentimeters);
    Device.Send(String(RangeInCentimeters), ultrasonicPin);
    prev_Value = RangeInCentimeters;
    }
  Device.Process(); 
  delay(1000);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
        Serial.print("incomming data for: ");               //display the value that arrived from the AllThingsTalk IOT developer cloud.
        Serial.print(pin);
        Serial.print(", value: ");
        Serial.print(value);
        Device.Send(value, pin);                            //send the value back for confirmation   
}

