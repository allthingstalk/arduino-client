/*
  Arduino UART Demo Sketch. This Sketch is made for an Genuino 101 IoT board with a Grove UART WiFi module 
  based on the popular ESP8266 IoT SoC to communicate to the AllThingsTalk IoT developer cloud

  The Grove UART WiFi module has a firmware installed which includes the ATT_IOT library. The UART WiFi module communicates through Serial1 of the genuino 101 board.
  
  version 1.0 dd 26/12/2015
  
  This sketch is an example sketch to deploy the grove temperature sensor (101020015) to the AllThingsTalk IoT developer cloud. 
 
  
  ### Instructions

  1. Setup the Arduino hardware
    - Use an Arduino Genuino 101 IoT board
    - Connect the Arduino Grove shield, make sure the switch is set to 3,3V (the formula below to calculate the temperature is based on a source voltage of 3,3V)
	- Connect USB cable to your computer
    - connect a Grove sunlight sensor to PIN I2C of the Arduino shield
    - Grove UART wifi to pin UART (D0,D1)

  2. Add 'ATT_IOT_UART' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, WIFI_SSID,WIFI_PWD) in the settings.h file. 
  4. Optionally, change sensor names, labels as appropiate. For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch
*/

#include "ATT_IOT_UART.h"                       //AllThingsTalk Arduino UART IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type.
#include "keys.h"                           	//keep all your personal account information in a seperate file
#include <Sodaq_TPH.h>

ATTDevice Device(&Serial1);                  
char httpServer[] = "api.smartliving.io";                       // HTTP API Server host                  
char mqttServer[] = "broker.smartliving.io";                    // MQTT Server Address

// Define the assets
#define tempId 0
#define pressuresId 1
#define humidityId 2

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
    
  Device.AddAsset(tempId, "temperature", "temperature", false, "{\"type\": \"number\", \"minimum\": -40, \"maximum\": 85, \"unit\": \"°\"}");   // Create the Sensor asset for your device
  Device.AddAsset(pressuresId, "pressure", "pressure", false, "{\"type\": \"number\", \"minimum\": 0, \"maximum\": 1100, \"unit\": \"hPa\"}");
  Device.AddAsset(humidityId, "humidity", "humidity", false, "{\"type\": \"number\", \"minimum\": 0, \"maximum\": 100, \"unit\": \"RH\"}");
  
  delay(1000);                                                 //give the wifi some time to finish everything
  while(!Device.Subscribe(mqttServer, callback))               // make sure that we can receive message from the AllThingsTalk IOT developer cloud  (MQTT). This stops the http connection
	Serial.println("retrying");
	
  tph.begin();
}

void loop()
{
  float temp = tph.readTemperature();
  float bmp_temp = tph.readTemperatureBMP();
  float sht_temp = tph.readTemperatureSHT();
  float hum = tph.readHumidity();
  float pres = tph.readPressure()/100.0;
  
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.println(" °C");
  
  Serial.print("Temperature (BMP sensor): ");
  Serial.print(bmp_temp);
  Serial.println(" °C");
  
  Serial.print("Temperature (SHT sensor): ");
  Serial.print(sht_temp);
  Serial.println(" °C");
  
  Serial.print("Humidity: ");
  Serial.print(hum);
  Serial.println(" %");
  
  Serial.print("Pressure: ");
  Serial.print(pres);
  Serial.println(" hPa");
  Serial.println();
  
  Device.Send(String(temp), tempId);
  Device.Send(String(hum), pressuresId);
  Device.Send(String(pres), humidityId);
  
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

