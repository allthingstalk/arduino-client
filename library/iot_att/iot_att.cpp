/*
AllThingsTalk - SmartLiving.io Arduino library 
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
char FAILED_RETRY[] = " failed,retrying";
char SUCCESTXT[] = " established";
#endif
char _mac[18];

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
	String macStr = String(mac[0], HEX);
	for(int i = 1; i < 6; i++)					//copy the mac address to a char buffer so we can use it later on to connect to mqtt.
		macStr += "-" + String(mac[i], HEX);
	macStr.toCharArray(_mac, 18);
	delay(ETHERNETDELAY);							// give the Ethernet shield a second to initialize:
	
	#ifdef DEBUG
	Serial.println(_mac);
	Serial.println(F("connecting"));
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
	return true;									//we have created a connection succesfully.
}

//create or update the specified asset.
void ATTDevice::AddAsset(String name, String description, bool isActuator, String type)
{
    // form a JSON-formatted string:
    String jsonString = "{\"name\":\"" + name + "\",\"description\":\"" + description + "\",\"is\":\"";
	if(isActuator)
		jsonString += "actuator";
	else
		jsonString += "sensor";
    jsonString += "\",\"profile\": { \"type\":\"" + type + "\" }, \"deviceId\":\"" + _deviceId + "\" }";
    
    // Make a HTTP request:
    _client.println(F("POST /api/asset?idFromName=true HTTP/1.1"));
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
    Serial.println(jsonString);  //show it on the screen
    delay(ETHERNETDELAY);
	GetHTTPResult();			//get the response fromm the server and show it.
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
	
	//delay(RETRYDELAY); 						//give the ethernet card a little time to stop properly before working with mqtt.
	while (!_mqttclient->connect(_mac)) 
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
void ATTDevice::Send(String value, String sensorId)
{
	unsigned long timeNow = (unsigned long)now();

	String pubString = String(timeNow) + "|" + value;
	int length = pubString.length() + 1;
	char message_buff[length];
	pubString.toCharArray(message_buff, length);
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("time = ")); Serial.println(timeNow);
	Serial.print(F("Publish to ")); Serial.print(sensorId); Serial.print(" : "); 
	#endif
	Serial.println(pubString);																	//this value is still useful and generated anyway, so no extra cost.
	
	String Mqttstring = "/f/" + _clientId + "/s/" + _deviceId + "/" + sensorId;
	length = Mqttstring.length() + 1;
	char Mqttstring_buff[length];
	Mqttstring.toCharArray(Mqttstring_buff, length);      
	_mqttclient->publish(Mqttstring_buff, message_buff);
	delay(100);													//give some time to the ethernet shield so it can process everything.       
}

//subscribe to the mqtt topic so we can receive data from the server.
void ATTDevice::MqttSubscribe()
{
	String MqttString = "/m/"+_clientId+"/#";
	char Mqttstring_buff[MqttString.length()+1];
    MqttString.toCharArray(Mqttstring_buff, MqttString.length()+1);
    _mqttclient->subscribe(Mqttstring_buff);
	
	Mqttstring_buff[1]  = 's';				//change from /m/ClientId/#  to /s/ClientId/# 
    _mqttclient->subscribe(Mqttstring_buff);
	#ifdef DEBUG
    Serial.println(F("MQTT Subscribed..."));
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

