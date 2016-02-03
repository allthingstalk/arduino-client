/****
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the grove temperature sensor (101020015) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield, 
    - Connect USB cable to your computer
    - connect a Grove push button to PIN D@ of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, WIFI_SSID,WIFI_PWD) in the settings.h file. 
  4. Optionally, change sensor names, labels as appropiate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch
 */

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           //keep all your personal account information in a seperate file

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define PIN numbers for assets
#define DigitalSensor 2                                        // Analog Sensor is connected to pin A0 on grove shield 

//required for the device
void callback(int pin, String& value);


void setup() 
{
  pinMode(DigitalSensor, INPUT);                                // initialize the digital pin as an input.          
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
    
  Device.AddAsset(DigitalSensor, "push button", "push button", false, "boolean");   // Create the Sensor asset for your device
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
    Serial.println("retrying");
}

bool sensorVal = false;
bool currentValue = false;

void loop() 
{
  bool sensorRead = digitalRead(DigitalSensor);                 // read status Digital Sensor
  if (sensorVal != sensorRead)                              // verify if value has changed
  {
    sensorVal = sensorRead;
	if(sensorVal){												//only send the value when pressed down.
		currentValue = !currentValue;							//before sending the value, invert it, cause the button was pressed, so the state has changed.
		SendValue();
	}
  }
}

void SendValue()
{
  Serial.print("button changed to: ");
  Serial.println(currentValue);
  if(currentValue)
    Device.Send("true", DigitalSensor);
  else
    Device.Send("false", DigitalSensor);
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(int pin, String& value) 
{ 
    Serial.print("incoming data for: ");               //display the value that arrived from the AllThingsTalk IOT developer cloud.
    Serial.print(pin);
    Serial.print(", value: ");
    Serial.print(value);
    Device.Send(value, pin);                            //send the value back for confirmation   
}

