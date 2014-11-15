/*

Not yet supported

	allthingstalk_arduino_gateway_lib.h - SmartLiving.io Arduino library 
	provides a way to create devices & assets + send & receives asset values to/from the cloud.
	
	Author: Jan Bogaerts
	first version: october 2014
*/

#ifndef ATTDevice_h
#define ATTDevice_h

#include "Arduino.h"
#include <Ethernet.h>
#include <PubSubClient.h>
#include <string.h>
#include <Dhcp.h>

//this class represents the ATT cloud platform.
class ATTGateway
{
	public:
		//create the object
		ATTGateway(String clientId, String clientKey);
		
		/*connect with the http server (call first)
		-mac: the mac address of the arduino (4 bytes)
		-httpServer: the name of the http server to use, kept in memory until after calling 'Subscribe' 
		returns: true when subscribe was succesfulll, otherwise false.*/
		bool Connect(byte mac[], char httpServer[]);
		
		//create a device
		void AddDevice(String deviceId, String name, String description);
		
		//check if the device already exists or not
		bool DeviceExists(String deviceId);
		
		//create or update the specified sensor. (call after connecting)
		//note: after this call, the name will be in lower case, so that it can be used to compare with the topic of incomming messages.
		void AddAsset(String deviceId, char id, String name, String description, bool isActuator, String type);

		/*Stop http processing & make certain that we can receive data from the mqtt server. */
		void Subscribe(PubSubClient& mqttclient);
		
		//send a data value to the cloud server for the sensor with the specified name.
		void Send(String deviceId, char sensorId, String value);
	
		//check for any new mqtt messages.
		void Process();
	private:	
		char* _serverName;				//stores the name of the http server that we should use.
		String _clientId;				//the client id provided by the user.	
		String _clientKey;				//the client key provided by the user.
		EthernetClient _client;			//raw http communication. Possible to save some memory here: pass the client as a param in connect, put the object local in the setup function.
		PubSubClient* _mqttclient;		//provides mqtt support
		
		//subscribe to the mqtt topic so we can receive data from the server.
		void MqttSubscribe(String deviceId);
		
		//tries to create a connection with the mqtt broker. also used to try and reconnect.
		void MqttConnect();
		
		//checks the result of the http request by comparing the characters at pos 9,10 & 11
		//ex: CheckHTTPResult('2','0','0')  will check if the http server returned 200 OK
		bool CheckHTTPResult(char a, char b, char c);
};

#endif