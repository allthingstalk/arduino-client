/*
	allthingstalk_arduino_standard_lib.cpp - SmartLiving.io Arduino library 
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.


#include "allthingstalk_arduino_standard_lib.h"

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
ATTDevice::ATTDevice(String deviceId, String clientId, String clientKey)
{
	_deviceId = deviceId;
	_clientId = clientId;
	_clientKey = clientKey;
}

//connect with the http server
bool ATTDevice::Connect(byte mac[], char httpServer[])
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

//create or update the specified asset.
void ATTDevice::AddAsset(int id, String name, String description, bool isActuator, String type)
{
    // Make a HTTP request:
	_client.println("PUT /api/asset/" + _deviceId +  (char)(id + 48) + " HTTP/1.1");
    _client.print(F("Host: "));
    _client.println(_serverName);
    _client.println(F("Content-Type: application/json"));
    _client.print(F("Auth-ClientKey: "));_client.println(_clientKey);
    _client.print(F("Auth-ClientId: "));_client.println(_clientId); 
	
	_client.print(F("Content-Length: "));
	{																					//make every mem op local, so it is unloaded asap
		int length = name.length() + description.length() + type.length() + _deviceId.length() + 77;
		if(isActuator) 
			length += 8;
		else 
			length += 6;
		_client.println(length);
	}
    _client.println();
    
	_client.print(F("{\"name\":\"")); 
	_client.print(name);
	_client.print(F("\",\"description\":\""));
	_client.print(description);
	_client.print(F("\",\"is\":\""));
	if(isActuator) 
		_client.print(F("actuator"));
	else 
		_client.print(F("sensor"));
    _client.print(F("\",\"profile\": { \"type\":\""));
	_client.print(type);
	_client.print(F("\" }, \"deviceId\":\""));
	_client.print(_deviceId);
	_client.print(F("\" }"));
	_client.println();
    _client.println();
 
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
	_client.flush();
	_client.stop();
	MqttConnect();
}

//tries to create a connection with the mqtt broker. also used to try and reconnect.
void ATTDevice::MqttConnect()
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
	MqttSubscribe();
}

//check for any new mqtt messages.
void ATTDevice::Process()
{
	_mqttclient->loop();
}

//send a data value to the cloud server for the sensor with the specified id.
void ATTDevice::Send(String value, int id)
{
	if(_mqttclient->connected() == false)
	{
		Serial.println(F("Lost broker connection,restarting")); 
		MqttConnect();
	}

	char* message_buff;
	{																					//put in a sub block so 'pubstring' can be freed asap.
		int length = value.length() + 3;
		message_buff = new char[length];
		sprintf(message_buff, "0|%s", value.c_str());
		message_buff[length-1] = 0;
	}
	
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

void ATTDevice::GetHTTPResult()
{
	// If there's incoming data from the net connection, send it out the serial port
	// This is for debugging purposes only
	if(_client.available()){
		while (_client.available()) {
			char c = _client.read();
			Serial.print(c);
		}
		Serial.println();
	}
}

