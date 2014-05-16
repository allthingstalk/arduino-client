#include <Ethernet.h>			//for loading components required by the iot device object.
#include <PubSubClient.h>

#include <iot_att.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.
#include <Time.h>				//so we can send values at a fixed rate.

/*
AllThingsTalk Makers Arduino Example 
*/

//create the object that provides the connection to the cloud to manager the device.
ATTDevice Device("Your device id comes here.", "your client id comes here", "your client key comes here");


// Update these with values suitable for your network.
byte mac[]    = {  0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x4A }; 	    // please adopt to your Personal MAC Address !!! see on backside Arduino 

int ValueIn = 0;												// Analog 0 is the input pin
char sensorName[] = "Potentiometer";							// name of the sensor, used to define the sensor on the iot platform.

int ledPin = 8;													// Pin 8 is the LED output pin	
char actuatorName[] = "blue-led";								// name of the actuator, used to identify the actuator on the iot platform.


char htmlServer[] = "att-1.apphb.com";  					  	// HTTP Server
byte mqttServer[] = { 188, 64, 53, 92 };					  	// MQTT(ATT1) Part
void callback(char* topic, byte* payload, unsigned int length);					//required for the device
EthernetClient ethClient;														//required for the device
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);						//required for the device

void setup()
{
	pinMode(ledPin, OUTPUT);									// initialize the digital pin as an output.
	Serial.begin(9600);											// init serial link for debugging
	if(Device.Connect(mac, htmlServer))							// connect the device with the IOT platform.
	{
		Device.AddAsset(sensorName, "Just a simple turn knob", false, "int");	// make certain that the iot platform knows the assets of the device: this is a sensor.
		Device.AddAsset(actuatorName, "Just a fancy LED", true, "bool");		// this is an actuator.
		Device.Subscribe(pubSub);												// make certain that we can receive message from the iot platform (activate mqtt)
	}
	else
		while(true);											//can't set up the device on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.
}

unsigned long time;												//only send every x amount of time.
void loop()
{
	unsigned long curTime = millis();
	if (curTime > (time + 5000)) 								// publish light reading every 5 seconds to sensor 1
	{
		unsigned int lightRead = analogRead(ValueIn);			// read from light sensor (photocell)
		Device.Send(String(lightRead), sensorName);
		time = curTime;
	}
	Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{  
	char message_buff[length + 1];						//need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	int i = 0;
	for(; i < length; i++) 								//create character buffer with ending null terminator (string)
		message_buff[i] = payload[i];
	message_buff[i] = '\0';								//make certain that it ends with a null			
	  
	String msgString = String(message_buff);
	msgString.toLowerCase();							//to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
	String topicStr = topic;							//we convert the topic to a string so we can easily work with it (use 'endsWith')
	
	Serial.println("Payload: " + msgString);			//show some debugging.
 	Serial.println("topic: " + topicStr);
	
	if (topicStr.endsWith(actuatorName)) 				//warning: the topic will always be lowercase. This allows us to work with multiple actuators: the name of the actuator to use is at the end of the topic.
	{
		if (msgString == "false") {
			digitalWrite(ledPin, LOW);					//change the led	
			Device.Send(msgString, actuatorName);		//also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
		}
		else if (msgString == "true") {
			digitalWrite(ledPin, HIGH);
			Device.Send(msgString, actuatorName);
		}
	}
}