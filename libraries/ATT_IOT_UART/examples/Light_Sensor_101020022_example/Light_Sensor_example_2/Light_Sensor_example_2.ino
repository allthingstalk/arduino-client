
#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "settings.h"                           //keep all your personal account information in a seperate file

/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy a light sensor to the AllThingsTalk IoT developer cloud. 
  This example published the Light State:  "dark", "moonlight", "sunset", "cloudy", "daylight", "sunlight"
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect USB cable to your computer
    - Connect an Arduino Grove shield
    - connect a Grove Light sensor to PIN A0 of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, WIFI_SSID,WIFI_PWD) in the settings.h file. 
  4. Optionally, change sensor names, labels as appropiate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting

  ### Note

  The Light sensor is based on a low cost LDR. The accuracy is lilmited but will do for most use cases to interprete the light condition
  
*/

// these constants won't change. They are the
// lowest and highest readings you get from your sensor:
const int sensorMin = 0;      // sensor minimum value
const int sensorMax = 1023;   // sensor maximum value


#define ARRAYSIZE 6
String light_Range[ARRAYSIZE] = { "dark", "moonlight", "sunset", "cloudy", "daylight", "sunlight" };


ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address


// Define PIN numbers for assets

#define AnalogSensor 0                                        // Analog Sensor is connected to pin A0 on grove shield 

//required for the device
void callback(int pin, String& value);


void setup()
{
  pinMode(AnalogSensor, INPUT);                                // initialize the pin as an input.               
  Serial.begin(9600);                                          // init serial link for debugging
  
  while (!Serial) ;                                            // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  Serial.println("Starting the Genuino 101 board");
  Serial1.begin(115200);                                       //init serial link for wifi module
  
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))           //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.StartWifi(WIFI_SSID, WIFI_PWD))                //if we can't succeed to initialize the WiFi, there is no point to continue
	Serial.println("retrying...");
  while(!Device.Connect(httpServer))                           // connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("retrying");
    
  Device.AddAsset(AnalogSensor, "Lightsensor", "Light sensor", false, "string");   // Create the Sensor asset for your device
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
	Serial.println("retrying");
}

int Prev_range = 0;
void loop()
{
  int sensorRead = analogRead(AnalogSensor);                 // read status Analog Sensor
  // Serial.println(sensorRead);
  // map the sensor range to a range of six options:
  int range = map(sensorRead, sensorMin, sensorMax, 0, 5);
  if (range != Prev_range)                                  // verify if value has changed
  {
    Serial.println(light_Range[range]);
    Device.Send(light_Range[range], AnalogSensor);
    Prev_range = range;
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

