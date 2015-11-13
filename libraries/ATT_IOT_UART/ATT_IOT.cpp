/*
	ATT_IOT.cpp - SmartLiving.io Arduino library 
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.


#include "ATT_IOT.h"
#include "SerialCommands.h"

#define RETRYDELAY 5000					//the nr of milliseconds that we pause before retrying to create the connection
#define ETHERNETDELAY 1000		//the nr of milliseconds that we pause to give the ethernet board time to start
#define MQTTPORT 1883

#ifdef DEBUG
char UARTINITTEXT[] = "initialization of UART";
char HTTPSERVTEXT[] = "connection HTTP Server";
char MQTTSERVTEXT[] = "connection MQTT Server";
char FAILED_RETRY[] = " failed,retry";
char SUCCESTXT[] = " established";
#endif

//create the object
ATTDevice::ATTDevice(Stream* stream)
{
	_stream = stream;
}

void ATTDevice::writeCommand(const char* command, String& param1, String& param2, String& param3)
{
}

bool ATTDevice::waitForOk()
{
}

//connect with the http server
bool ATTDevice::Init(String deviceId, String clientId, String clientKey)
{
	#ifdef DEBUG
	Serial.println(F("initializing serial connection with wifi module"));
	#endif
	writeCommand(CMD_INIT, deviceId, clientId, clientKey);
	bool res = waitForOk();
	#ifdef DEBUG
	if(res == false){
		Serial.print(UARTINITTEXT);
		Serial.println(FAILED_RETRY);
	}
	#endif
	return res;
}

//connect with the http server
bool ATTDevice::Connect(char httpServer[])
{
	#ifdef DEBUG
	Serial.println(F("Connecting"));
	#endif

	writeCommand(CMD_CONNECT, deviceId, clientId, clientKey);
	bool res = waitForOk();
	#ifdef DEBUG
	if(res == false){
		Serial.print(HTTPSERVTEXT);
		Serial.println(FAILED_RETRY);
	}
	else
	{
		#ifdef DEBUG
		Serial.print(HTTPSERVTEXT);
		Serial.println(SUCCESTXT);
		delay(ETHERNETDELAY);							// another small delay: sometimes the card is not yet ready to send the asset info.
	}
	#endif
	return res;
}

//create or update the specified asset.
void ATTDevice::AddAsset(int id, String name, String description, bool isActuator, String type)
{
    // Make a HTTP request:
	_client->println("PUT /asset/" + _deviceId +  (char)(id + 48) + " HTTP/1.1");
    _client->print(F("Host: "));
    _client->println(_serverName);
    _client->println(F("Content-Type: application/json"));
    _client->print(F("Auth-ClientKey: "));_client->println(_clientKey);
    _client->print(F("Auth-ClientId: "));_client->println(_clientId); 
	
	_client->print(F("Content-Length: "));
	{																					//make every mem op local, so it is unloaded asap
		int length = name.length() + description.length() + type.length() + _deviceId.length();
		if(isActuator) 
			length += 8;
		else 
			length += 6;
		if(type[0] == '{')
			 length += 64;
		 else
			 length += 77;
		_client->println(length);
	}
    _client->println();
    
	_client->print(F("{\"name\":\"")); 
	_client->print(name);
	_client->print(F("\",\"description\":\""));
	_client->print(description);
	_client->print(F("\",\"is\":\""));
	if(isActuator) 
		_client->print(F("actuator"));
	else 
		_client->print(F("sensor"));
	if(type[0] == '{'){
		_client->print(F("\",\"profile\": "));
		_client->print(type);
	}
	else{
		_client->print(F("\",\"profile\": { \"type\":\""));
		_client->print(type);
		_client->print(F("\" }"));
	}
	_client->print(F(", \"deviceId\":\""));
	_client->print(_deviceId);
	_client->print(F("\" }"));
	_client->println();
    _client->println();
 
    delay(ETHERNETDELAY);
	GetHTTPResult();			//get the response from the server and show it.
}

//connect with the http server and broker
void ATTDevice::Subscribe(PubSubClient& mqttclient)
{
	_mqttclient = &mqttclient;	
	_serverName = NULL;					//no longer need this reference.
	#ifdef DEBUG
	Serial.println(F("Stopping HTTP"));
	#endif
	_client->flush();
	_client->stop();
	_client = NULL;
	MqttConnect();
}

//tries to create a connection with the mqtt broker. also used to try and reconnect.
void ATTDevice::MqttConnect()
{
	char mqttId[23]; // Or something long enough to hold the longest file name you will ever use.
	int length = _deviceId.length();
	length = length > 22 ? 22 : length;
    _deviceId.toCharArray(mqttId, length);
	mqttId[length] = 0;
	String brokerId = _clientId + ":" + _clientId;
	while (!_mqttclient->connect(mqttId, (char*)brokerId.c_str(), (char*)_clientKey.c_str())) 
	{
		#ifdef DEBUG
		Serial.print(MQTTSERVTEXT);
		Serial.println(FAILED_RETRY);
		#endif
		delay(RETRYDELAY);
	}
	#ifdef DEBUG
	Serial.print(MQTTSERVTEXT);
	Serial.println(SUCCESTXT);
	#endif
	MqttSubscribe();
}

//check for any new mqtt messages.
void ATTDevice::Process()
{
	_mqttclient->loop();
}

//builds the content that has to be sent to the cloud using mqtt (either a csv value or a json string)
char* ATTDevice::BuildContent(String value)
{
	char* message_buff;
	int length;
	if(value[0] == '[' || value[0] == '{'){
		length = value.length() + 16;
		message_buff = new char[length];
		sprintf(message_buff, "{\"value\":%s}", value.c_str());
	}
	else{
		length = value.length() + 3;
		message_buff = new char[length];
		sprintf(message_buff, "0|%s", value.c_str());
	}
	message_buff[length-1] = 0;
	return message_buff;
}


//send a data value to the cloud server for the sensor with the specified id.
void ATTDevice::Send(String value, int id)
{
	if(_mqttclient->connected() == false)
	{
		Serial.println(F("Lost broker connection,restarting")); 
		MqttConnect();
	}

	char* message_buff = BuildContent(value);
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("Publish to ")); Serial.print(id); Serial.print(" : "); 
	#endif
	Serial.println(message_buff);																	//this value is still useful and generated anyway, so no extra cost.
	
	char* Mqttstring_buff;
	{
		int length = _clientId.length() + _deviceId.length() + 26;
		Mqttstring_buff = new char[length];
		sprintf(Mqttstring_buff, "client/%s/out/asset/%s%c/state", _clientId.c_str(), _deviceId.c_str(), (char)(id + 48));      
		Mqttstring_buff[length-1] = 0;
	}
	_mqttclient->publish(Mqttstring_buff, message_buff);
	delay(100);													//give some time to the ethernet shield so it can process everything.       
	delete(message_buff);
	delete(Mqttstring_buff);
}


//subscribe to the mqtt topic so we can receive data from the server.
void ATTDevice::MqttSubscribe()
{
	String MqttString = "client/" + _clientId + "/in/device/" + _deviceId + "/asset/+/command";  //the arduino is only interested in the actuator commands, no management commands
	char Mqttstring_buff[MqttString.length()+1];
    MqttString.toCharArray(Mqttstring_buff, MqttString.length()+1);
    _mqttclient->subscribe(Mqttstring_buff);

	#ifdef DEBUG
    Serial.print(F("MQTT Client subscribed"));
	#endif
}

//returns the pin nr found in the topic
int ATTDevice::GetPinNr(char* topic, int topicLength)
{
	return topic[topicLength - 9] - 48;
}

void ATTDevice::GetHTTPResult()
{
	// If there's incoming data from the net connection, send it out the serial port
	// This is for debugging purposes only
	if(_client->available()){
		while (_client->available()) {
			char c = _client->read();
			Serial.print(c);
		}
		Serial.println();
	}
}

