/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the Grove sunlight sensor (101020089) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield
	- Connect USB cable to your computer
    - Connect a Grove sunlight sensor to PIN I2C of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Fill in the missing strings (deviceId, clientId, clientKey) in the keys.h file. 
  4. Optionally, change sensor names, labels as appropriate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch
*/

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           	//keep all your personal account information in a seperate file
#include "SI114X.h"

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define the assets
// For digital and analog sensors, we recommend to use the physical pin id as the asset id.  
// For other sensors (I2C and UART), you can select any other (unique) number as id for the asset.
SI114X SI1145 = SI114X();
#define visLightId 0
#define irLightId 1
#define uvLightId 2

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
    
  Device.AddAsset(visLightId, "visual light", "visual light", false, "{\"type\": \"integer\", \"minimum\": 0, \"unit\": \"lm\"}");   // Create the Sensor asset for your device
  Device.AddAsset(irLightId, "infra red", "infra red light", false, "{\"type\": \"integer\", \"minimum\": 0, \"unit\": \"lm\"}");
  Device.AddAsset(uvLightId, "UV", "ultra violet light", false, "{\"type\": \"number\", \"minimum\": 0, \"unit\": \"UV index\"}");
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
	Serial.println("retrying");
	
  Serial.println("Beginning Si1145!");
  while (!SI1145.Begin()) {
    Serial.println("Si1145 is not ready!");
    delay(1000);
  }
  Serial.println("Si1145 is ready!");	
}

void loop()
{
  Serial.println("//--------------------------------------//");
  int intValue = SI1145.ReadVisible();
  Serial.print("Vis: "); 
  Serial.println(value);
  Device.Send(String(value), visLightId);
  
  intValue = SI1145.ReadIR();
  Serial.print("IR: "); 
  Serial.println(value);
  Device.Send(String(value), irLightId);
  
  //the real UV value must be div 100 from the reg value , datasheet for more information.
  float value = (float)SI1145.ReadUV()/100;
  Serial.print("UV: ");  
  Serial.println(value);
  Device.Send(String(value), uvLightId);
  
  Device.Process(); 
  delay(1000);
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

