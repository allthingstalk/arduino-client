/*
   Copyright 2014-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.
#define FAST_MQTT


#include "ATT_IOT.h"

#define RETRYDELAY 5000					//the nr of milliseconds that we pause before retrying to create the connection
#define ETHERNETDELAY 1000		//the nr of milliseconds that we pause to give the ethernet board time to start
#define MQTTPORT 1883

#ifdef DEBUG
char HTTPSERVTEXT[] = "connection HTTP Server";
char MQTTSERVTEXT[] = "connection MQTT Server";
char FAILED_RETRY[] = " failed,retry";
char SUCCESTXT[] = " established";
#endif

//create the object
ATTDevice::ATTDevice(String deviceId, String clientId, String clientKey): _client(NULL), _mqttclient(NULL)
{
	_deviceId = deviceId;
	_clientId = clientId;
	_clientKey = clientKey;
}

//connect with the http server
bool ATTDevice::Connect(Client* httpClient, char httpServer[])
{
	_client = httpClient;
	_serverName = httpServer;					//keep track of this value while working with the http server.
	
	#ifdef DEBUG
	Serial.print("Connecting to ");
    Serial.println(httpServer);
	#endif

	if (!_client->connect(httpServer, 80)) 		// if you get a connection, report back via serial:
	{
		#ifdef DEBUG
		Serial.print(HTTPSERVTEXT);
		Serial.println(FAILED_RETRY);
		#endif
		return false;									//we have created a connection succesfully.
	}
	else{
		#ifdef DEBUG
		Serial.print(HTTPSERVTEXT);
		Serial.println(SUCCESTXT);
		#endif
		delay(ETHERNETDELAY);							// another small delay: sometimes the card is not yet ready to send the asset info.
		return true;									//we have created a connection succesfully.
	}
}

//closes any open connections (http & mqtt) and resets the device. After this call, you 
//can call connect and/or subscribe again. Credentials remain stored.
void ATTDevice::Close()
{
	CloseHTTP();
	_mqttUserName = NULL;
	_mqttpwd = NULL;
	if(_mqttclient){
		_mqttclient->disconnect();
		_mqttclient = NULL;
	}
}

//closes the http connection, if any.
void ATTDevice::CloseHTTP()
{
	if(_client){
		#ifdef DEBUG
		Serial.println(F("Stopping HTTP"));
		#endif
		_client->flush();
		_client->stop();
		_client = NULL;
	}
}

//create or update the specified asset.
void ATTDevice::AddAsset(int id, String name, String description, bool isActuator, String type)
{
    // Make a HTTP request:
	{
		String idStr(id);
		_client->println("PUT /device/" + _deviceId + "/asset/" + idStr  + " HTTP/1.1");
	}
    _client->print(F("Host: "));
    _client->println(_serverName);
    _client->println(F("Content-Type: application/json"));
    _client->print(F("Auth-ClientKey: "));_client->println(_clientKey);
    _client->print(F("Auth-ClientId: "));_client->println(_clientId); 
	
	int typeLength = type.length();
	_client->print(F("Content-Length: "));
	{																					//make every mem op local, so it is unloaded asap
		int length = name.length() + description.length() + typeLength;
		if(isActuator) 
			length += 8;
		else 
			length += 6;
		if (typeLength == 0)
			length += 39;
		else if(type[0] == '{')
			length += 49;
		else
			length += 62;
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
	if(typeLength == 0)
		_client->print(F("\""));
	else if(type[0] == '{'){
		_client->print(F("\",\"profile\": "));
		_client->print(type);
	}
	else{
		_client->print(F("\",\"profile\": { \"type\":\""));
		_client->print(type);
		_client->print(F("\" }"));
	}
	_client->print(F("}"));
	_client->println();
    _client->println();
	
	unsigned long maxTime = millis() + 1000;
	while(millis() < maxTime)		//wait, but for the minimum amount of time.
	{
		if(_client->available()) break;
		else delay(10);
	}
	GetHTTPResult();			//get the response from the server and show it.
}

//connect with the http server and broker
bool ATTDevice::Subscribe(PubSubClient& mqttclient)
{
	Serial.println("subscribing");
	if(_clientId && _clientKey){
		String brokerId = _clientId + ":" + _clientId;
		return Subscribe(mqttclient, brokerId.c_str(), _clientKey.c_str());
	}
	else{
		#ifdef DEBUG
		Serial.print(MQTTSERVTEXT);
		Serial.println("failed: invalid credentials");
		#endif
		return false;
	}
}

/*Stop http processing & make certain that we can receive data from the mqtt server, given the specified username and pwd.
  This Subscribe function can be used to connect to a fog gateway
returns true when successful, false otherwise*/
bool ATTDevice::Subscribe(PubSubClient& mqttclient, const char* username, const char* pwd)
{
	_mqttclient = &mqttclient;	
	_serverName = "";					//no longer need this reference.
	CloseHTTP();
	_mqttUserName = username;
	_mqttpwd = pwd;
	return MqttConnect();
}

//tries to create a connection with the mqtt broker. also used to try and reconnect.
bool ATTDevice::MqttConnect()
{
	char mqttId[23]; // Or something long enough to hold the longest file name you will ever use.
	int length = _deviceId.length();
	length = length > 22 ? 22 : length;
    _deviceId.toCharArray(mqttId, length);
	mqttId[length] = 0;
	if(_mqttUserName && _mqttpwd){
		if (!_mqttclient->connect(mqttId, (char*)_mqttUserName, (char*)_mqttpwd)) 
		{
			#ifdef DEBUG
			Serial.print(MQTTSERVTEXT);
			Serial.println(FAILED_RETRY);
			#endif
			return false;
		}
		#ifdef DEBUG
		Serial.print(MQTTSERVTEXT);
		Serial.println(SUCCESTXT);
		#endif
		MqttSubscribe();
		return true;
	}
	else{
		#ifdef DEBUG
		Serial.print(MQTTSERVTEXT);
		Serial.println("failed: invalid credentials");
		#endif
		return false;
	}
}

//check for any new mqtt messages.
bool ATTDevice::Process()
{
	if(_mqttclient->connected() == false)
	{
		#ifdef DEBUG	
		Serial.println(F("Lost broker connection,restarting from process")); 
		#endif
		if(MqttConnect() == false)
			return false;
	}
	_mqttclient->loop();
	return true;
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
		#ifdef DEBUG	
		Serial.println(F("Lost broker connection,restarting from send")); 
		#endif
		MqttConnect();
	}

	char* message_buff = BuildContent(value);
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("Publish to ")); Serial.print(id); Serial.print(": "); Serial.println(message_buff);																
	#endif
	
	char* Mqttstring_buff;
	{
		String idStr(id);																			//turn it into a string, so we can easily calculate the nr of characters that the nr requires. 
		int length = _clientId.length() + _deviceId.length() + 33 + idStr.length();
		Mqttstring_buff = new char[length];
		sprintf(Mqttstring_buff, "client/%s/out/device/%s/asset/%s/state", _clientId.c_str(), _deviceId.c_str(), idStr.c_str());      
		Mqttstring_buff[length-1] = 0;
	}
	_mqttclient->publish(Mqttstring_buff, message_buff);
	#ifndef FAST_MQTT											//some boards like the old arduino ethernet need a little time after sending mqtt data, other boards don't.
	delay(100);													//give some time to the ethernet shield so it can process everything.       
	#endif
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
    Serial.println("MQTT Client subscribed");
	#endif
}

//returns the pin nr found in the topic
int ATTDevice::GetPinNr(char* topic, int topicLength)
{
	char digitOffset = 9;						//skip the '/command' at the end of the topic
	int result = topic[topicLength - digitOffset] - 48; 		// - 48 to convert digit-char to integer
	
	digitOffset++;
    while(topic[topicLength - digitOffset] != '/'){
		char digit = topic[topicLength - digitOffset];
		if(digit == '-')											//we found a - sign in front of the number, so return the negative result.
			return -result;
		else{
			int nextDigit = topic[topicLength - digitOffset] - 48;
			for(int i = 9; i < digitOffset; i++)
				nextDigit *= 10;
			result += nextDigit;
			digitOffset++;
		}
	}		
    return result;
}

void ATTDevice::GetHTTPResult()
{
	// If there's incoming data from the net connection, send it out the serial port
	// This is for debugging purposes only
	if(_client->available()){
		while (_client->available()) {
			char c = _client->read();
			#ifdef DEBUG
			Serial.print(c);
			#endif
		}
		#ifdef DEBUG
		Serial.println();
		#endif
	}
}

