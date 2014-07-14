/*
	iot_att.cpp - SmartLiving.io Arduino library 

	Morse.cpp - Library for flashing Morse code.
	Created by David A. Mellis, November 2, 2007.
	Released into the public domain.
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.


#include "iot_att.h"
#include <Time.h>

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
void ATTDevice::AddAsset(String id, String name, String description, bool isActuator, String type)
{
    // form a JSON-formatted string:
    String jsonString = "{\"name\":\"" + name + "\",\"description\":\"" + description + "\",\"is\":\"";
	if(isActuator) 
		jsonString += "actuator";
	else 
		jsonString += "sensor";
    jsonString += "\",\"profile\": { \"type\":\"" + type + "\" }, \"deviceId\":\"" + _deviceId + "\" }";
    
    Serial.println(jsonString);  									//show it on the screen

    // Make a HTTP request:
    _client.println("PUT /api/asset/" + _deviceId + id + " HTTP/1.1");
    _client.print(F("Host: "));
    _client.println(_serverName);
    _client.println(F("Content-Type: application/json"));
    _client.print(F("Auth-ClientKey: "));_client.println(_clientKey);
    _client.print(F("Auth-ClientId: "));_client.println(_clientId); 
	_client.print("Content-Length: ");
	_client.println(jsonString.length());
    _client.println();
    _client.println(jsonString);
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
	int length = sizeof(_deviceId) > 22 ? 22 : sizeof(_deviceId);
    _deviceId.toCharArray(mqttId, length);
	mqttId[22] = 0;
	Serial.println(mqttId);
	while (!_mqttclient->connect(mqttId)) 
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
void ATTDevice::Send(String value, String sensorName)
{
	if(_mqttclient->connected() == false)
	{
		Serial.println(F("Lost broker connection,restarting")); 
		MqttConnect();
	}
	unsigned long timeNow = (unsigned long)now();

	String pubString = String(timeNow) + "|" + value;
	int length = pubString.length() + 1;
	char message_buff[length];
	pubString.toCharArray(message_buff, length);
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("time = ")); Serial.println(timeNow);
	Serial.print(F("Publish to ")); Serial.print(sensorName); Serial.print(" : "); 
	#endif
	Serial.println(pubString);																	//this value is still useful and generated anyway, so no extra cost.
	
	String Mqttstring = "f/" + _clientId + "/d/" + _deviceId + "/a/" + sensorName;
	length = Mqttstring.length() + 1;
	char Mqttstring_buff[length];
	Mqttstring.toCharArray(Mqttstring_buff, length);      
	_mqttclient->publish(Mqttstring_buff, message_buff);
	delay(100);													//give some time to the ethernet shield so it can process everything.       
}


//subscribe to the mqtt topic so we can receive data from the server.
void ATTDevice::MqttSubscribe()
{
	String MqttString = "m/" + _clientId + "/d/" + _deviceId + "/#";
	char Mqttstring_buff[MqttString.length()+1];
    MqttString.toCharArray(Mqttstring_buff, MqttString.length()+1);
    _mqttclient->subscribe(Mqttstring_buff);
	
	Mqttstring_buff[0] = 's';				//change from m/ClientId/DeviceId/#  to s/ClientId/DeviceId/# 
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

