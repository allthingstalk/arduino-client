/*
   Copyright 2014-2016 AllThingsTalk

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#include "NW_WatchDog.h"

#define WATCHDOG_ID -1

#define DEBUG
#define FAST_MQTT

#include <string.h>

NW_WatchDog::NW_WatchDog(PubSubClient& mqttclient, const char* deviceId, const char* clientId, unsigned int frequency)
{
	_mqttclient = &mqttclient;	
	_clientId = clientId;
	_deviceId = deviceId;
	_topicLen = strlen(_deviceId) + strlen(_clientId) + 36;
	_frequency = frequency;
}

//void NW_WatchDog::Setup(ATTDevice& device)
//{
//	device.AddAsset(WATCHDOG_ID, "network watchdog", "used to verify the connectivity with the broker", true, "integer");
//}


bool NW_WatchDog::IsWatchDog(int pinNr, String& value)
{
	if(pinNr == WATCHDOG_ID){
		_lastReceived = atol(value.c_str());
		Serial.print("received ping: "); Serial.println(_lastReceived);
		return true;
	}
	return false;
}


bool NW_WatchDog::CheckPing()
{
	if(_nextPingAt <= millis()){
		if(_lastReceived != _pingCounter){				//oeps, something went wrong, did not receive our last ping back in time, so close the connection (it will be reopened by the function in the base class
			Serial.println("ping not arrived: resetting connection");
			_mqttclient->disconnect();
			_lastReceived = 0;								//reset the counters, 
			_pingCounter = 0;
			return false;
		}
		else{
			_pingCounter++;
			Ping();
			return true;
		}
	}
	return true;
}

void NW_WatchDog::Ping()
{
	_nextPingAt = millis() + _frequency; 
	
	Serial.println("pinging");
	String message_buff(_pingCounter);
	char* Mqttstring_buff;
	{
		//only build this string when we need it cause it takes up a lot of mem
		Mqttstring_buff = new char[_topicLen];
		sprintf(Mqttstring_buff, "client/%s/in/device/%s/asset/-1/command", _clientId, _deviceId);      
		Mqttstring_buff[_topicLen-1] = 0;
	}
	
	#ifdef DEBUG																					//don't need to write all of this if not debugging.
	Serial.print(F("ping to ")); Serial.print(Mqttstring_buff); Serial.print(" : "); 
	Serial.println(message_buff);																
	#endif
	
	_mqttclient->publish(Mqttstring_buff, message_buff.c_str());
	Serial.println("ping sent");
	#ifndef FAST_MQTT											//some boards like the old arduino ethernet need a little time after sending mqtt data, other boards don't.
	delay(100);													//give some time to the ethernet shield so it can process everything.       
	#endif
	delete(Mqttstring_buff);
}