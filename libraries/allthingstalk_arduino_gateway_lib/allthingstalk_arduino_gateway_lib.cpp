/*

Not yet supported

	allthingstalk_arduino_gateway_lib.cpp - SmartLiving.io Arduino library 
	provides a way to create devices & assets + send & receives asset values to/from the cloud.
	
	Author: Jan Bogaerts
	first version: october 2014
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.


#include "allthingstalk_arduino_gateway_lib.h"

#define RETRYDELAY 5000					//the nr of milliseconds that we pause before retrying to create the connection
#define ETHERNETDELAY 1000		        //the nr of milliseconds that we pause to give the ethernet board time to start
#define MQTTPORT 1883

#ifdef DEBUG
char HTTPSERVTEXT[] = "connection HTTP Server";
char MQTTSERVTEXT[] = "connection MQTT Server";
char FAILED_RETRY[] = " failed,retry";
char SUCCESTXT[] = " established";
#endif

//create the object
ATTGateway::ATTGateway(String clientId, String clientKey)
{
	_clientId = clientId;
	_clientKey = clientKey;
}

//connect with the http server
bool ATTGateway::Connect(byte mac[], char httpServer[])
{
	_serverName = httpServer;					//keep track of this value while working with the http server.
	if (Ethernet.begin(mac) == 0) 				// Initialize the Ethernet connection:
	{	
		Serial.println(F("DHCP failed,end"));
		return false;							//we failed to connect
	}
	delay(ETHERNETDELAY);							// give the Ethernet shield a second to initialize:
	
	#ifdef DEBUG
	Serial.println(F("Connecting"));
	#endif

	while (!_client.connect(httpServer, 80)) 		// if you get a connection, report back via serial:
	{
		#ifdef DEBUG
		Serial.print(HTTPSERVTEXT);
		Serial.println(FAILED_RETRY);
		#endif
		delay(RETRYDELAY);
	}

	#ifdef DEBUG
	Serial.print(HTTPSERVTEXT);
	Serial.println(SUCCESTXT);
	#endif
	delay(ETHERNETDELAY);							// another small delay: sometimes the card is not yet ready to send the asset info.
	return true;									//we have created a connection succesfully.
}

void ATTGateway::AddDevice(String deviceId, String name, String description)
{
	// Make a HTTP request:
	_client.println("POST /api/device HTTP/1.1");
    _client.print(F("Host: "));
    _client.println(_serverName);
    _client.println(F("Content-Type: application/json"));
    _client.print(F("Auth-ClientKey: "));_client.println(_clientKey);
    _client.print(F("Auth-ClientId: "));_client.println(_clientId); 

	_client.print(F("Content-Length: "));
	{																					//make every mem op local, so it is unloaded asap
		int length = name.length() + description.length() + deviceId.length() + 43;
		_client.println(length);
	}
	
    _client.println();
	_client.print(F("{\"id\":\"xbee_")); //12
	_client.print(deviceId);
	_client.print(F("\", \"name\":\"")); //11
	_client.print(name);
	_client.print(F("\",\"description\":\"")); //17
	_client.print(description);
	_client.print(F("\" }"));  //3
	_client.println();
 
    delay(ETHERNETDELAY);
	if(CheckHTTPResult('2', '0','1')){
		Serial.println("Device created");
		MqttSubscribe(deviceId);
	}
	else
		Serial.println("Failed to create device");
	delay(ETHERNETDELAY);							// another small delay: sometimes the card is not yet ready to send the asset info.
}

//check if the device already exists or not
bool ATTGateway::DeviceExists(String deviceId)
{
	// Make a HTTP request:
	_client.println("GET /api/device/xbee_" + deviceId + " HTTP/1.1");
    _client.print(F("Host: "));
    _client.println(_serverName);
    _client.println(F("Content-Type: application/json"));
    _client.print(F("Auth-ClientKey: "));_client.println(_clientKey);
    _client.print(F("Auth-ClientId: "));_client.println(_clientId); 
    _client.println();
 
    delay(ETHERNETDELAY);
	return CheckHTTPResult('2', '0','0');
}

//checks the result of the http request by comparing the characters at pos 9,10 & 11
//ex: CheckHTTPResult('2','0','0')  will check if the http server returned 200 OK
bool ATTGateway::CheckHTTPResult(char a, char b, char c)
{
	if(_client.available()){							//check the result
		int count = 0;
		while (_client.available()) {
			count++;
			char c = _client.read();
			if((count == 10 && c != a) ||
               (count == 11 && c != b) ||
			   (count == 12 && c != c) ){					//if the result starting at pos 10 is 200, then the device exists, otherwise it doesn't
				_client.flush();							//make certain that there is nothing left in the ethernet buffer, cause this can screw up the other actions.
				return false;
			}
			else if(count > 12)							//when we have read more then 11 bytes, we know the result, so discard anything else that came in
				_client.flush();
		}
		return count > 12;								//if the count > 11, and we get here, then the http result contained '200', so the device exists.
	}	
	return false;
}

//create or update the specified asset.
void ATTGateway::AddAsset(String deviceId, char id, String name, String description, bool isActuator, String type)
{
    // Make a HTTP request:
	_client.print(F("PUT /api/asset/xbee_"));
	_client.print(deviceId);
	_client.print(F("_"));
	_client.print(id);
	_client.println(" HTTP/1.1");
    
	_client.print(F("Host: "));
    _client.println(_serverName);
    _client.println(F("Content-Type: application/json"));
    _client.print(F("Auth-ClientKey: "));_client.println(_clientKey);
    _client.print(F("Auth-ClientId: "));_client.println(_clientId); 
	
	_client.print(F("Content-Length: "));
	{																					//make every mem op local, so it is unloaded asap
		int length = name.length() + description.length() + type.length() + deviceId.length() + 82;
		if(isActuator) 
			length += 8;
		else 
			length += 6;
		_client.println(length);
	}
    _client.println();
    
	_client.print(F("{\"name\":\"")); //9
	_client.print(name);
	_client.print(F("\",\"description\":\""));  //17
	_client.print(description);
	_client.print(F("\",\"is\":\"")); //8
	if(isActuator) 
		_client.print(F("actuator"));
	else
		_client.print(F("sensor"));
    _client.print(F("\",\"profile\": { \"type\":\"")); //23
	_client.print(type);
	_client.print(F("\" }, \"deviceId\":\"xbee_"));  //22
	_client.print(deviceId);
	_client.print(F("\" }"));  //3
	_client.println();
 
    delay(ETHERNETDELAY);
	if(CheckHTTPResult('2', '0','1'))
		Serial.println("Asset created");
	else
		Serial.println("Failed to create asset");
	delay(ETHERNETDELAY);							// another small delay: sometimes the card is not yet ready to send the asset info.
}

//connect with the http server and broker
void ATTGateway::Subscribe(PubSubClient& mqttclient)
{
	_mqttclient = &mqttclient;
	MqttConnect();
}

//tries to create a connection with the mqtt broker. also used to try and reconnect.
void ATTGateway::MqttConnect()
{
	char mqttId[23]; // Or something long enough to hold the longest file name you will ever use.
	int length = _clientId.length();
	length = length > 22 ? 22 : length;
    _clientId.toCharArray(mqttId, length);
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
}

//check for any new mqtt messages.
void ATTGateway::Process()
{
	_mqttclient->loop();
}

//send a data value to the cloud server for the sensor with the specified id.
void ATTGateway::Send(String deviceId, char sensorId, String value)
{
	if(_mqttclient->connected() == false)
	{
		Serial.println(F("Lost broker connection,restarting")); 
		MqttConnect();
	}

	char* message_buff;
	{																					//put in a sub block so 'length' can be freed asap.
		int length = value.length() + 3;
		message_buff = new char[length];
		sprintf(message_buff, "0|%s", value.c_str());
		message_buff[length-1] = 0;
	}
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("Publish to ")); Serial.print(sensorId); Serial.print(" : "); 
	Serial.println(message_buff);
	#endif																
	
	char* Mqttstring_buff;
	{
		int length = _clientId.length() + deviceId.length() + 30;
		Mqttstring_buff = new char[length];
		sprintf(Mqttstring_buff, "client/%s/out/asset/xbee_%s_%c/state", _clientId.c_str(), deviceId.c_str(), sensorId);      
		Mqttstring_buff[length-1] = 0;
	}
	_mqttclient->publish(Mqttstring_buff, message_buff);
	delay(100);													//give some time to the ethernet shield so it can process everything.       
	delete(message_buff);
	delete(Mqttstring_buff);
}


//subscribe to the mqtt topic so we can receive data from the server.
void ATTGateway::MqttSubscribe(String deviceId)
{
	String MqttString = "client/" + _clientId + "/in/device/xbee_" + deviceId + "/asset/+/command";  //arduinos are only intersted in actuator command, no management commands
	char Mqttstring_buff[MqttString.length()+1];
    MqttString.toCharArray(Mqttstring_buff, MqttString.length()+1);
    _mqttclient->subscribe(Mqttstring_buff);

	#ifdef DEBUG
    Serial.println(F("MQTT Client subscribed"));
	#endif
}

