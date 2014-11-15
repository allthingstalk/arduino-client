/*
AllThingsTalk - SmartLiving.io Arduino library 
Released into the public domain.

Original author: Peter Leemans (2014)
2/5/2014	Jan Bogaerts	Convert to library
*/

#ifndef ATTDevice_h
#define ATTDevice_h

#include "Arduino.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <string.h>
#include <Dhcp.h>

//this class represents the ATT cloud platform.
class ATTDevice
{
	public:
		//create the object
		ATTDevice(String deviceId, String clientId);
			
		//set up ethernet & make certain that we can receive data from the mqtt server.
		void Subscribe(byte* mac, PubSubClient &mqttclient, char* clientKey);
		
		//send a data value to the cloud server for the sensor with the specified id.
		void Send(String value, char* id);
	
		//check for any new mqtt messages.
		void Process();
	private:	
		String _deviceId;				//the device id provided by the user.
		String _clientId;				//the client id provided by the user.	
		char* _clientKey;				//the pwd used to connect to the broker.
		PubSubClient* _mqttclient;		//provides mqtt support
		
		//subscribe to the mqtt topic so we can receive data from the server.
		void MqttSubscribe();
		
		//tries to create a connection with the mqtt broker. also used to try and reconnect.
		void MqttConnect();
		
};

#endif