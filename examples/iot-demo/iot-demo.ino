#include <Ethernet.h>			//for loading components required by the iot device object.
#include <PubSubClient.h>

#include <allthingstalk_arduino_standard_lib.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.

/*
  AllThingsTalk Makers Arduino Example 

  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Add 'allthingstalk_arduino_standard_lib' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting

  1. 'Device' type is reported to be missing. 
  - Make sure to properly add the arduino/libraries/allthingstalk_arduino_standard_lib/ library

*/

char deviceId[] = "YOUR_DEVICE_ID_HERE";
char clientId[] = "YOUR_CLIENT_ID_HERE";
char clientKey[] = "YOUR_CLIENT_KEY_HERE";

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "api.smartliving.io";                  // HTTP API Server host
char* mqttServer = "broker.smartliving.io";                   

byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E}; 	    // Adapt to your Arduino MAC Address  

int knobPin = 0;                                            // Analog 0 is the input pin + identifies the asset on the cloud platform
int ledPin = 8;                                             // Pin 8 is the LED output pin + identifies the asset on the cloud platform

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  pinMode(ledPin, OUTPUT);					                  // initialize the digital pin as an output.         
  Serial.begin(9600);							          // init serial link for debugging
  
  if(Device.Connect(mac, httpServer))					          // connect the device with the IOT platform.
  {
    Device.AddAsset(knobPin, "knob", "rotary switch",false, "int");
    Device.AddAsset(ledPin, "led", "light emitting diode", true, "bool");
    Device.Subscribe(pubSub);						        // make certain that we can receive message from the iot platform (activate mqtt)
  }
  else 
    while(true);                                                                //can't set up the device on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.								
}

unsigned long time;							        //only send every x amount of time.
unsigned int prevVal =0;
void loop()
{
  unsigned long curTime = millis();
  if (curTime > (time + 1000)) 							// publish light reading every 5 seconds to sensor 1
  {
    unsigned int lightRead = analogRead(knobPin);			        // read from light sensor (photocell)
    if(prevVal != lightRead){
      Device.Send(String(lightRead), knobPin);
      prevVal = lightRead;
    }
    time = curTime;
  }
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {	                                                    //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	char message_buff[length + 1];						//need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	strncpy(message_buff, (char*)payload, length);		//copy over the data
	message_buff[length] = '\0';							    //make certain that it ends with a null			
		  
	msgString = String(message_buff);
	msgString.toLowerCase();							//to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {	                                                    //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	int topicLength = strlen(topic);
	
	Serial.print("Payload: ");			                //show some debugging.
	Serial.println(msgString);
	Serial.print("topic: ");
	Serial.println(topic);
	
	if (topic[topicLength - 9] == (ledPin + 48))        //warning: only a max of 10 actuators supported (0-9)
	{
	  if (msgString == "false") {
            digitalWrite(ledPin, LOW);					        //change the led	
            idOut = &ledPin;		                        
	  }
	  else if (msgString == "true") {
	    digitalWrite(ledPin, HIGH);
            idOut = &ledPin;
	  }
	}
  }
  if(idOut != NULL)                //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
