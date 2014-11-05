/*
	iot_att.cpp - SmartLiving.io Arduino library 
*/

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.


#include "iot_att_min.h"

#define RETRYDELAY 5000					//the nr of milliseconds that we pause before retrying to create the connection
#define ETHERNETDELAY 1000				//the nr of milliseconds that we pause to give the ethernet board time to start
#define MQTTPORT 1883

#ifdef DEBUG
char MQTTSERVTEXT[] = "connection MQTT Server";
char FAILED_RETRY[] = " failed,retry";
char SUCCESTXT[] = " established";
#endif

//create the object
ATTDevice::ATTDevice(String deviceId, String clientId)
{
	_deviceId = deviceId;
	_clientId = clientId;
}

//connect with the http server and broker
void ATTDevice::Subscribe(byte* mac, PubSubClient& mqttclient)
{
	if (Ethernet.begin(mac) == 0)                         // Initialize the Ethernet connection: for pub-sub client
       Serial.println(F("DHCP failed,end"));
	delay(1000);							                // give the Ethernet shield a second to initialize:
  
	_mqttclient = &mqttclient;	
	MqttConnect();
}

//tries to create a connection with the mqtt broker. also used to try and reconnect.
void ATTDevice::MqttConnect()
{
	char mqttId[23]; // Or something long enough to hold the longest file name you will ever use.
	int length = _clientId.length();
	length = length > 22 ? 22 : length;
    _clientId.toCharArray(mqttId, length);
	mqttId[22] = 0;
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
void ATTDevice::Send(String value, char id)
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
	Serial.print(F("Publish to ")); Serial.print(id); Serial.print(": "); 
	Serial.println(message_buff);
	#endif
																	
	char* Mqttstring_buff;
	{
		int length = _clientId.length() + 26;
		Mqttstring_buff = new char[length];
		sprintf(Mqttstring_buff, "client/%s/out/asset/%s%c/state", _clientId.c_str(),  id);      
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
	String MqttString = "client/" + _clientId + "/in/device/" + _deviceId + "/+/command";   //arduinos are only intersted in actuator command, no management commands
	char Mqttstring_buff[MqttString.length()+1];
    MqttString.toCharArray(Mqttstring_buff, MqttString.length()+1);
    _mqttclient->subscribe(Mqttstring_buff);

	#ifdef DEBUG
    Serial.print(F("MQTT Client subscribed"));
	#endif
}


