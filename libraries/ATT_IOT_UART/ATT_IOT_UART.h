/*
AllThingsTalk - SmartLiving.io Arduino library 
Released into the public domain.

Original author: Peter Leemans (2014)
2/5/2014	Jan Bogaerts	Convert to library
2/3/2015	Jan Bogaerts 	release 2
*/

#ifndef ATT_IOT_UART
#define ATT_IOT_UART

#include "Arduino.h"
#include <string.h>

#define DEFAULT_INPUT_BUFFER_SIZE 400
#define DEFAULT_TIMEOUT 40000

#define DEBUG					//turns on debugging in the IOT library. comment out this line to save memory.

typedef void (*mqttCallback)(int pin, String& value);

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
		bool Init(String deviceId, String clientId, String clientKey);
		
		/*Start up the wifi network
		blocks until connection has been made
		if forceReset == true, then any previous router settings will be removed and you will
		have to use another device again to provide the correct settings.
		*/
		bool StartWifi(bool forceReset = false);
		
		/*connect with the http server (call first)
		-Client: the client object to use for communicating with the cloud HTTP server (this is usually an EthernetClient, WifiClient or similar)
		-httpServer: the name of the http server to use, kept in memory until after calling 'Subscribe' 
		returns: true when subscribe was successful, otherwise false.*/
		bool Connect(char httpServer[]);
		
		//create or update the specified asset. (call after connecting)
		//note: after this call, the name will be in lower case, so that it can be used to compare with the topic of incomming messages.
		bool AddAsset(int id, String name, String description, bool isActuator, String type);

		/*Stop http processing & make certain that we can receive data from the mqtt server. 
		blocks until connection has been made*/
		bool Subscribe(char broker[], mqttCallback callback);
		
		//send a data value to the cloud server for the sensor with the specified id.
		//returns true when successful
		bool Send(String value, int id);
	
		//check for any new mqtt messages.
		void Process();

	private:	
		Stream* _stream;
		mqttCallback _callback;
		char inputBuffer[DEFAULT_INPUT_BUFFER_SIZE + 1];
		
		void writeCommand(const char* command, String& param1);
		void writeCommand(const char* command, String& param1, String& param2);
		void writeCommand(const char* command, String& param1, String& param2, String& param3);
		void writeCommand(const char* command, String& param1, String& param2, String& param3, String& param4, String& param5);
		
		void sendParam(String& param);
		//if timeout == 0: then wait indefinitely
		bool expectString(const char* str, unsigned short timeout = DEFAULT_TIMEOUT, bool report = true);
		bool expectAny(unsigned short timeout = DEFAULT_TIMEOUT);
		unsigned short readLn(char* buffer, unsigned short size, unsigned short start = 0);
		unsigned short readLn() { return readLn(this->inputBuffer, DEFAULT_INPUT_BUFFER_SIZE); }
};

#endif
