#include <ESP8266WiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <PubSubClient.h>
#include <ATT_IOT.h>                            //AllThingsTalk IoT library
#include <SPI.h>                                //required to have support for signed/unsigned long type..
#include "SerialCommands.h"

#define DEBUG

// Enter below your client credentials. 
//These credentials can be found in the configuration pane under your device in the smartliving.io website 


ATTDevice *Device = NULL;            //create the object that provides the connection to the cloud to manager the device.

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
WiFiClient ethClient;
PubSubClient *pubSub = NULL;  // mqttServer, 1883, callback,

#define SERIALSPEED 115200
#define INPUTBUFFERSIZE 255
char inputBuffer[INPUTBUFFERSIZE];                          //the input buffer for receiving commands
String receivedPayload;
int receivedPin;
bool dataReceived = false;
char* serverName = NULL;


void configModeCallback () {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
}

void setup()
{         
  Serial.begin(SERIALSPEED);                         // init serial link 
  delay(200);                                       //give serial a little time to init
}

bool startWifi(bool forceReset)
{
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
	delay(200);
    //reset settings - for testing
	if(forceReset){											//if we reset the wifi, then the client lib will ask to start the wifi a second time, that's when we handle it properly
		if(Device)
			Device->Close();									//make certain that any previous connections are closed, if required.
		wifiManager.resetSettings();
		delay(500);											//give the serial a little bit of time to write + the wifi module has to update it's flash, which also takes a little time	
	}
	
	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);
	//fetches ssid and pass and tries to connect
	//if it does not connect it starts an access point with the specified name, here  "ESP8266 wifi"
	//and goes into a blocking loop awaiting configuration
	if(!wifiManager.autoConnect("ESP8266 wifi")) {
		Serial.println("failed to connect and hit timeout");
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(1000);
		return false;
	} 
	else{
		Serial.println("connected");                        //if you get here you have connected to the WiFi
		return true;
	}
}

void serialFlush(){
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
} 

void init(char* startOfParams)
{
    char* clientId = strchr(startOfParams, ';');            //first param is ssid
    *clientId = 0;
    ++clientId;
    char* clientKey = strchr(clientId, ';');            //first param is ssid
    *clientKey = 0;
    ++clientKey;
    if(Device != NULL){
        delete Device;
        Device = NULL;
    }
    Device = new ATTDevice(startOfParams, clientId, clientKey);
    serialFlush();                                 //make certain that there are no other commands in the buffer -> the remote needs to send a new command after the ack
    Serial.println(CMD_INIT_OK);
}

void connect(char* httpServer)
{
    size_t len = strlen(httpServer);
    if(serverName){
      delete serverName;
      serverName = NULL;
    }
    serverName = new char[len + 1];
    strncpy(serverName, httpServer, len);
    serverName[len] = 0;

    bool res = Device->Connect(&ethClient, serverName);
    serialFlush();                                 //make certain that there are no other commands in the buffer -> the remote needs to send a new command after the ack
	if(res)
		Serial.println(CMD_CONNECT_OK);
	else
		Serial.println(STR_RESULT_NOK);
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
    
    char* type = strchr(next, ';');         
    *type = 0;
    ++type;
    
    bool isActuator = false;
    if(strcmp(next, "true") == 0) isActuator = true;

    Device->AddAsset(pin, name, description, isActuator, type);
    delay(100);										//the client needs some time to sync, without this, the client doesn't know that the operation stopped
    serialFlush();                                 //make certain that there are no other commands in the buffer -> the remote needs to send a new command after the ack
    Serial.println(CMD_ADDASSET_OK);
}          
             
void subscribe(char* broker)
{
    if(pubSub){
        delete pubSub;
        pubSub = NULL;
    }
    pubSub = new PubSubClient(ethClient);
    pubSub->setServer(broker, 1883);
    pubSub->setCallback(callback);
    bool res = Device->Subscribe(*pubSub);
    serialFlush();                                 //make certain that there are no other commands in the buffer -> the remote needs to send a new command after the ack
	if(res)
		Serial.println(CMD_SUBSCRIBE_OK);
	else
		Serial.println(STR_RESULT_NOK);
}    

void send(char* startOfParams)
{
    char* pinStr = strchr(startOfParams, ';');          //first param is ssid
    *pinStr = 0;
    ++pinStr;                                           //this now points to second param = pwd 

    int pin = atoi(pinStr);
    
    Device->Send(startOfParams, pin);
    serialFlush();                                 //make certain that there are no other commands in the buffer -> the remote needs to send a new command after the ack
    Serial.println(CMD_SEND_OK);
}


bool CommsDone = false;                                         // true when we hav received at least 1 command from client. Used to keep initializing the serial connection, so that user doesn't have to push the reset button on the wifi module before comms can begin.
 
void loop()
{
    if(Serial.available() > 0){
        int len = Serial.readBytesUntil('\n', inputBuffer, INPUTBUFFERSIZE);
    if(len > 0)
        inputBuffer[len - 1] = 0; // bytes until \n always end with \r, so get rid of it (-1)
    else
        inputBuffer[0] = 0;
        char* separator = strchr(inputBuffer, ';');
        // Actually split the string in 2: replace ';' with 0
        if(separator){
            *separator = 0;
            ++separator;
        }
		if(strcmp(inputBuffer, CMD_AT_RESET_WIFI) == 0){
			serialFlush();
            if(startWifi(true))
                Serial.println(CMD_AT_RESET_WIFI_OK);
            else
                Serial.println(STR_RESULT_NOK);
            CommsDone = true;
		}
        else if(strcmp(inputBuffer, CMD_AT) == 0){
            serialFlush();
            if(startWifi(false))
                Serial.println(CMD_AT_OK);
            else
                Serial.println(STR_RESULT_NOK);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_INIT) == 0){
            init(separator);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_CONNECT) == 0){
            connect(separator);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_ADDASSET) == 0){
            addAsset(separator);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_SUBSCRIBE) == 0){
            subscribe(separator);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_SEND) == 0){
            send(separator);
            CommsDone = true;
        }
        else if(strcmp(inputBuffer, CMD_RECEIVE) == 0){
            Device->Process(); 
            if(dataReceived){
                dataReceived = false;
                Serial.print(receivedPin);
                Serial.print(";");
                Serial.println(receivedPayload);
            }
            else
                Serial.println(STR_RESULT_OK);
            CommsDone = true;
        }
    }
    if(CommsDone == false){
        Serial.begin(SERIALSPEED);                         // rest serial link speed, if we don't do this, the handshake with the other device might fail
        delay(1000);
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


