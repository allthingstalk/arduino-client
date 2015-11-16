#include <ESP8266WiFi.h>

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type..
#include "SerialCommands.h"


// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the smartliving.io website 


ATTDevice *Device;            //create the object that provides the connection to the cloud to manager the device.

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient ethClient;
PubSubClient pubSub(ethClient);  // mqttServer, 1883, callback,

void setup()
{         
  Serial.begin(9600);                         // init serial link for debugging                                                              
}

char inputBuffer[255];							//the input buffer for receiving commands
String receivedPayload;
int receivedPin;
bool dataReceived = false;

void setup_wifi(char* startOfParams) {
  delay(10);
  char* pwd = strchr(startOfParams, ';');			//first param is ssid
  *pwd = 0;
  ++pwd;											//this now points to second param = pwd	
  
  WiFi.begin(startOfParams, pwd);
  while (WiFi.status() != WL_CONNECTED) {
	delay(500);
  }
  Serial.println(STR_RESULT_OK);
}

void init(char* startOfParams)
{
	char* clientId = strchr(startOfParams, ';');			//first param is ssid
	*clientId = 0;
	++clientId;
	char* clientKey = strchr(clientId, ';');			//first param is ssid
	*clientKey = 0;
	++clientKey;
	Device = new ATTDevice(startOfParams, clientId, clientKey);
	Serial.println(STR_RESULT_OK);
}

void connect(char* httpServer)
{
	Device->Connect(&ethClient, httpServer);
	Serial.println(STR_RESULT_OK);
}

void addAsset(char* startOfParams)   
{
	char* name = strchr(startOfParams, ';');	
	*name = 0;
	int pin = atoi(startOfParams);
	++name;
	
	char* description = strchr(name, ';');	
	*description = 0;
	++description;
	
	char* next = strchr(description, ';');
	*next = 0;
	++next;
	
	char* type = strchr(next, ';');			//first param is ssid
	*type = 0;
	++type;
	
	bool isActuator = false;
	if(strcmp(next, "true") == 0) isActuator = true;
	
	Device->AddAsset(pin, name, description, isActuator, type);
	Serial.println(STR_RESULT_OK);
}          
			 
void subscribe(char* broker)
{
	pubSub.setServer(broker, 1883);
	pubSub.setCallback(callback);
	Device->Subscribe(pubSub);
	Serial.println(STR_RESULT_OK);
}	 

void send(char* startOfParams)
{
	char* value = strchr(startOfParams, ';');			//first param is ssid
	*value = 0;
	++value;											//this now points to second param = pwd	
	 
	char* next = strchr(value, ';');			//first param is ssid
	*next = 0;
	++next;
	int pin = atoi(next);
	
	Device->Send(value, pin);
	Serial.println(STR_RESULT_OK);
}
			 
void loop()
{
	if(Serial.available() > 0){
		int len = Serial->readBytesUntil('\n', inputBuffer, size);
		char* separator = strchr(inputBuffer, ';');
		// Actually split the string in 2: replace ';' with 0
        *separator = 0;
        ++separator;
        if(strcmp(inputBuffer, CMD_AT) == 0)
			Serial.println(STR_RESULT_OK);
		else if(strcmp(inputBuffer, CMD_INIT) == 0)
			init(separator);
		else if(strcmp(inputBuffer, CMD_WIFI) == 0)
			setup_wifi(separator);
		else if(strcmp(inputBuffer, CMD_CONNECT) == 0)
			connect(separator);
		else if(strcmp(inputBuffer, CMD_ADDASSET) == 0)
			addAsset(separator);
		else if(strcmp(inputBuffer, CMD_SUBSCRIBE) == 0)
			subscribe(separator);
		else if(strcmp(inputBuffer, CMD_SEND) == 0)
			send(separator);
		else if(strcmp(inputBuffer, CMD_RECEIVE) == 0){
			Device.Process(); 
			if(dataReceived){
				dataReceived = false;
				Serial.print(receivedPin);
				Serial.print(";");
				Serial.println(receivedPayload);
			}
			else
				Serial.println(STR_RESULT_OK);
		}
	}
}

// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  {                                                           //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
	  char message_buff[length + 1];                        //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation
	  strncpy(message_buff, (char*)payload, length);        //copy over the data
	  message_buff[length] = '\0';                  //make certain that it ends with a null     
		  
	  receivedPayload = String(message_buff);
	  receivedPayload.toLowerCase();            //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  receivedPin = Device->GetPinNr(topic, strlen(topic));
  dataReceived = true;
}


