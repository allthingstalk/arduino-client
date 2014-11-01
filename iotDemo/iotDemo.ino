#include <Ethernet.h>			//for loading components required by the iot device object.
#include <PubSubClient.h>

#include <iot_att.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.
//#include <Time.h>				//so we can send values at a fixed rate.

/*
  AllThingsTalk Makers Arduino Example 

  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting

  1. 'Device' type is reported to be missing. 
  - Make sure to properly add the 'iot_att' library

*/

char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here";
char clientKey[] = "";// Your client key comes here";

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "beta.smartliving.io";                  // HTTP API Server host
char* mqttServer = "broker.smartliving.io";                   

byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E }; 	    // Adapt to your Arduino MAC Address  

String sensorId = "1";										// uniquely identify this asset. Don't use spaces in the id.
String actuatorId = "2";									// uniquely identify this asset. Don't use spaces in the id.

int ValueIn = 0;                                            // Analog 0 is the input pin
int ledPin = 8;                                             // Pin 8 is the LED output pin 

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
    Device.AddAsset(sensorId, F("Sensor_name"), F("your sensor description"), false, F("int"));   
    Device.AddAsset(actuatorId, F("actuator_name"), F("your actuator description"), true, F("bool"));
    Device.Subscribe(pubSub);						        // make certain that we can receive message from the iot platform (activate mqtt)
  }
  else 
    while(true);                                                                //can't set up the device on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.								
}

unsigned long time;							        //only send every x amount of time.
void loop()
{
  unsigned long curTime = millis();
  if (curTime > (time + 5000)) 							// publish light reading every 5 seconds to sensor 1
  {
    unsigned int lightRead = analogRead(ValueIn);			        // read from light sensor (photocell)
    Device.Send(String(lightRead), sensorId);
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
  String* idOut = NULL;
  {	                                                    //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	String topicStr = topic;							//we convert the topic to a string so we can easily work with it (use 'endsWith')
	
	Serial.print("Payload: ");			                //show some debugging.
	Serial.println(msgString);
	Serial.print("topic: ");
	Serial.println(topicStr);
	
	if (topicStr.endsWith(actuatorId)) 				                //warning: the topic will always be lowercase. This allows us to work with multiple actuators: the name of the actuator to use is at the end of the topic.
	{
	  if (msgString == "false") {
		digitalWrite(ledPin, LOW);					        //change the led	
		idOut = &actuatorId;		                        
	  }
	  else if (msgString == "true") {
		digitalWrite(ledPin, HIGH);
		idOut = &actuatorId;
	  }
	}
  }
  if(idOut != NULL)                //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
