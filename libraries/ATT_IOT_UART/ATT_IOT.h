/*
AllThingsTalk - SmartLiving.io Arduino library 
Released into the public domain.

Original author: Peter Leemans (2014)
2/5/2014	Jan Bogaerts	Convert to library
2/3/2015	Jan Bogaerts 	release 2
*/

#ifndef ATTDevice_h
#define ATTDevice_h

#include "Arduino.h"
#include <string.h>

//this class represents the ATT cloud platform.
class ATTDevice
{
	public:
		//create the object
		ATTDevice(Stream* stream);
		
		/*Initialize the library
		deviceId: the id of the device, as found on the website
		clientId: the client id, found on the website
		clientkey: secret key for your device, found on website.
		*/
		void Init(String deviceId, String clientId, String clientKey);
		
		/*connect with the http server (call first)
		-Client: the client object to use for communicating with the cloud HTTP server (this is usually an EthernetClient, WifiClient or similar)
		-httpServer: the name of the http server to use, kept in memory until after calling 'Subscribe' 
		returns: true when subscribe was successful, otherwise false.*/
		bool Connect(char httpServer[]);
		
		//create or update the specified asset. (call after connecting)
		//note: after this call, the name will be in lower case, so that it can be used to compare with the topic of incomming messages.
		void AddAsset(int id, String name, String description, bool isActuator, String type);

		/*Stop http processing & make certain that we can receive data from the mqtt server. */
		void Subscribe(PubSubClient& mqttclient);
		
		//send a data value to the cloud server for the sensor with the specified id.
		void Send(String value, int id);
	
		//check for any new mqtt messages.
		void Process();
		
		//returns the pin nr found in the topic
		int GetPinNr(char* topic, int topicLength);
	private:	
		Stream* _stream;
		
		void writeCommand(const char* command, String& param1, String& param2, String& param3);
		bool waitForOk();
};

#endif