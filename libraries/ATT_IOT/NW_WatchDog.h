/*
AllThingsTalk - AllThingsTalk.io Arduino library 
Released into the public domain.

15/3/2016	Jan Bogaerts 	release 1
07/11/2016	Jan Bogaerts 	rebrand to AllThingsTalk
*/

#ifndef ATT_NW_WatchDog_h
#define ATT_NW_WatchDog_h

//#include "Arduino.h"
#include <ATT_IOT.h>
#include <string.h>
#include <PubSubClient.h>

/*
	Adds a network watchdog feature to the att device. 
	When the device made a successfull connection with the cloud, it will automatically add an actuator to the device, with id -1, which is used to verify network connectivity.
	Warning: uses asset id -1  for the watchdog actuator.
*/
class NW_WatchDog
{
	public:
		//create the object
		NW_WatchDog(PubSubClient& mqttclient, const char* deviceId, const char* clientId, unsigned int frequency = 300000);
		

		void Setup(ATTDevice& device);
		
		//send a ping to the broker
		void Ping();
	
		//check if we need to resend a ping and if we received the previous ping in time
		//returns true if the previous ping was in time, when it wasn't, false is returned.
		//upon false, you can try to recreate the connection with the broker.
		bool CheckPing();
		
		//checks if we received a ping back from the broker.If not, the broker connection will be closed (and reopened upon the next call to 'process'.
		//returns true if the pin was for the network monitor actuator. Otherwise, it returns false.
		bool IsWatchDog(int pinNr, String& value);
	
	private:	
		unsigned long _nextPingAt = 0;				//time when the next ping should be sent.
		unsigned int _pingCounter = 0;				//next ping to send out
		unsigned long _lastReceived = 0;			//ping that we last received, set to same as first ping, otherwise the first call will fail
		PubSubClient* _mqttclient;
		const char* _deviceId;
		const char* _clientId;
		unsigned char _topicLen;					//so we only have to calculate it 1 time
		unsigned int _frequency;
		
};

#endif