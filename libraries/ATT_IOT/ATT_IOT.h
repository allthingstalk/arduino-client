/*
AllThingsTalk - SmartLiving.io Arduino library 
Released into the public domain.

Original author: Peter Leemans (2014)
2/5/2014	Jan Bogaerts	Convert to library
2/3/2015	Jan Bogaerts 	release 2
*/

#ifndef ATTDevice_h
#define ATTDevice_h

//#include "Arduino.h"
#include <Client.h>
#include <PubSubClient.h>
#include <string.h>

//this class represents the ATT cloud platform.
class ATTDevice
{
	public:
		//create the object
		ATTDevice(String deviceId, String clientId, String clientKey);
		
		/*connect with the http server (call first)
		-Client: the client object to use for communicating with the cloud HTTP server (this is usually an EthernetClient, WifiClient or similar)
		-httpServer: the name of the http server to use, kept in memory until after calling 'Subscribe' 
		returns: true when subscribe was successful, otherwise false.*/
		bool Connect(Client* httpClient, char httpServer[]);
		
		//create or update the specified asset. (call after connecting)
		//note: after this call, the name will be in lower case, so that it can be used to compare with the topic of incomming messages.
		void AddAsset(int id, String name, String description, bool isActuator, String type);

		/*Stop http processing & make certain that we can receive data from the mqtt server. 
		returns true when successful, false otherwise*/
		bool Subscribe(PubSubClient& mqttclient);
		
		/*Stop http processing & make certain that we can receive data from the mqtt server, given the specified username and pwd.
		  This Subscribe function can be used to connect to a fog gateway
		returns true when successful, false otherwise*/
		bool Subscribe(PubSubClient& mqttclient, const char* username, const char* pwd);
		
		//send a data value to the cloud server for the sensor with the specified id.
		void Send(String value, int id);
		
		//closes any open connections (http & mqtt) and resets the device. After this call, you 
		//can call connect and/or subscribe again. Credentials remain stored.
		//Note: all clients (httpclient & pubsubClient) are the caller's responsibility to clean up
		void Close();
	
		//check for any new mqtt messages.
		void Process();
		
		//returns the pin nr found in the topic
		int GetPinNr(char* topic, int topicLength);
	private:	
		String _serverName;				//stores the name of the http server that we should use.
		String _clientKey;				//the client key provided by the user.
		Client* _client;				//raw http communication. Possible to save some memory here: pass the client as a param in connect, put the object local in the setup function.
		
		const char* _mqttUserName;		//we store a local copy of the the mqtt username and pwd, so we can auto reconnect if the connection was lost.
		const char* _mqttpwd;	
		
		//subscribe to the mqtt topic so we can receive data from the server.
		void MqttSubscribe();
		
		//read all the data from the Ethernet card and display on the debug screen.
		void GetHTTPResult();
		
		//builds the content that has to be sent to the cloud using mqtt (either a csv value or a json string)
		char* BuildContent(String value);
		
		//closes the http connection, if any.
		void CloseHTTP();
		
		PubSubClient* _mqttclient;		//provides mqtt support (placed as protected, so we can build inheriters that can access the mqtt client directly (ex: network watchdog)
		
		//tries to create a connection with the mqtt broker. also used to try and reconnect.
		bool MqttConnect();				//so inheriters can reconnect with the mqtt server if they detect a network loss.
		String _deviceId;				//the device id provided by the user.
		String _clientId;				//the client id provided by the user.	
};

#endif