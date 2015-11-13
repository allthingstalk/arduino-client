#include <Ethernet.h>
#include <EthernetClient.h>

#include <PubSubClient.h>

#include <ATT_IOT.h>       // AllThingstalk IoT Library
#include <SPI.h>                                      // required to have support for signed/unsigned long type.

/*
  SmartLiving Makers Arduino Experiment 

  ### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - vibration motor to D8
  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
     For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Upload the sketch

  ### Troubleshooting


*/

char deviceId[] = ""; // Your device id comes here
char clientId[] = ""; // Your client id comes here
char clientKey[] = "";// Your client key comes here

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "api.smartliving.io";                  // HTTP API Server host
char* mqttServer = "broker.smartliving.io";                   

int VMotor = 8;                                        // Pin 8 is the vibration motor output pin and it's also used to construct the AssetID

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

void setup()
{
  pinMode(VMotor, OUTPUT);					                  // initialize the digital pin as an output.         
  Serial.begin(9600);							          // init serial link for debugging
  
  byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E }; 	  // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0) 				                // Initialize the Ethernet connection:
  {	
    Serial.println(F("DHCP failed,end"));
    while(true);							        //we failed to connect, halt execution here. 
  }
  delay(1000);							                //give the Ethernet shield a second to initialize:
  
  if(Device.Connect(&ethClient, httpServer))					          // connect the device with the IOT platform.
  {
    Device.AddAsset(VMotor, "Alerter", "Vibration Motor", true, "boolean");
    Device.Subscribe(pubSub);						        // make certain that we can receive message from the iot platform (activate mqtt)
  }
  else 
    while(true);                                                                //can't set up the device on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.								
}

unsigned long time;							        //only send every x amount of time.
void loop()
{
  Device.Process(); 
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                            //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	char message_buff[length + 1];	                      //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	strncpy(message_buff, (char*)payload, length);        //copy over the data
	message_buff[length] = '\0';		              //make certain that it ends with a null			
		  
	msgString = String(message_buff);
	msgString.toLowerCase();			      //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {	                                                      //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	int pinNr = Device.GetPinNr(topic, strlen(topic));
	
	Serial.print("Payload: ");                            //show some debugging.
	Serial.println(msgString);
	Serial.print("topic: ");
	Serial.println(topic);
	
	if (pinNr == VMotor)     
	{
	  if (msgString == "false") {
            digitalWrite(VMotor, LOW);		     //change the vibration motor status to false
            idOut = &VMotor;		                        
	  }
	  else if (msgString == "true") {
	    digitalWrite(VMotor, HIGH);              //change the vibration motor status to true
            idOut = &VMotor;
	  }
	}
  }
  if(idOut != NULL)                //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
    Device.Send(msgString, *idOut);    
}
