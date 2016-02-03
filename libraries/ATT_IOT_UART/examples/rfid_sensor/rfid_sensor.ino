/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the grove RFID sensor (113020002) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect USB cable to your computer
    - Connect the Arduino Grove shield
    - connect the Grove RFID sensor to PIN A0 of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, WIFI_SSID,WIFI_PWD) in the settings.h file. 
  4. Optionally, change sensor names, labels as appropiate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Note

  The Grove-125KHz RFID Reader is a module used to read uem4100 RFID card information with two output formats: Uart and Wiegand. 
  It has a sensitivity with maximum 7cm sensing distance.

  The RFID reader has a UART interface. 
  In this example we will communicate to this device using a softwareserial port (pins (2,3) which are found on PIN D2 on the Grove Shield.
  link between the board and the device is at 9600 bps 8-N-1

*/

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           //keep all your personal account information in a seperate file
#include <SoftwareSerial.h>


ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address


// Define  asset data and objects
#define RFIDsensor_Id 0                                             // Sensor Id represents assetId under the Device in ATT cloud
SoftwareSerial SoftSerial(2, 3);
char buffer[64];   // buffer array for data receive over serial port of the RFID device
int count=0;                // counter for buffer array 

//required for the device
void callback(int pin, String& value);


void setup()
{           
  Serial.begin(57600);                                         // init serial link for debugging
  while (!Serial) ;                                            // This line makes sure you see all output on the monitor. REMOVE THIS LINE if you want your IoT board to run without monitor !
  
  Serial.println("Starting sketch");
  Serial1.begin(115200);                                       //init serial link for wifi module
  while(!Serial1);
  
  SoftSerial.begin(9600);                                      // init serial link for the RFID device 
  
  while(!Device.StartWifi())
    Serial.println("retrying...");
  while(!Device.Init(DEVICEID, CLIENTID, CLIENTKEY))           //if we can't succeed to initialize and set the device credentials, there is no point to continue
    Serial.println("retrying...");
  while(!Device.Connect(httpServer))                           // connect the device with the AllThingsTalk IOT developer cloud. No point to continue if we can't succeed at this
    Serial.println("retrying");
    
  Device.AddAsset(RFIDsensor_Id, "RFIDsensor", "RFID reader", false, "string");   // Create the Sensor asset for your device
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
	Serial.println("retrying");
}


void clearBufferArray()                 // function to clear buffer array
{
  for (int i=0; i<count;i++)
    { buffer[i]=NULL;}                  // clear all index of array with command NULL
}


void loop()
{
  if (SoftSerial.available())              // if date is coming from software serial port ==> data is coming from SoftSerial shield
  {
    while(SoftSerial.available())          // reading data into char array 
    {
      buffer[count++] = SoftSerial.read();   // writing data into array
      if(count == 63) break;
      delay(10);                           // added delay because the GENUINO 101 seems to be t o fast for the serial port routine...
    }
    buffer[count++] = '\0';               // Needs a terminating String char.
    Serial.print("RFID tag: ");Serial.println(buffer);
    Device.Send(buffer, RFIDsensor_Id);    
    clearBufferArray();              // call clearBufferArray function to clear the stored data from the array
    count = 0;                       // set counter of while loop to zero      
  }
  Device.Process();
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

