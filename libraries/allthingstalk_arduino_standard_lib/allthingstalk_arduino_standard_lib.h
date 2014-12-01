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
		ATTDevice(String deviceId, String clientId, String clientKey);
		
		/*connect with the http server (call first)
		-mac: the mac address of the arduino (4 bytes)
		-httpServer: the name of the http server to use, kept in memory until after calling 'Subscribe' 
		returns: true when subscribe was succesfulll, otherwise false.*/
		bool Connect(byte mac[], char httpServer[]);
		
		//create or update the specified asset. (call after connecting)
		//note: after this call, the name will be in lower case, so that it can be used to compare with the topic of incomming messages.
		void AddAsset(int id, String name, String description, bool isActuator, String type);

		/*Stop http processing & make certain that we can receive data from the mqtt server. */
		void Subscribe(PubSubClient& mqttclient);
		
		//send a data value to the cloud server for the sensor with the specified id.
		void Send(String value, int id);
	
		//check for any new mqtt messages.
		void Process();
	private:	
		char* _serverName;				//stores the name of the http server that we should use.
		String _deviceId;				//the device id provided by the user.
		String _clientId;				//the client id provided by the user.	
		String _clientKey;				//the client key provided by the user.
		EthernetClient _client;			//raw http communication. Possible to save some memory here: pass the client as a param in connect, put the object local in the setup function.
		PubSubClient* _mqttclient;		//provides mqtt support
		
		//subscribe to the mqtt topic so we can receive data from the server.
		void MqttSubscribe();
		
		//tries to create a connection with the mqtt broker. also used to try and reconnect.
		void MqttConnect();
		
		//read all the data from the ethernet card and display on the debug screen.
		void GetHTTPResult();
};

#endif