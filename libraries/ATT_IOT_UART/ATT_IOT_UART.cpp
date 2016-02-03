/*
    ATT_IOT.cpp - SmartLiving.io Arduino library 
*/



#include "ATT_IOT_UART.h"
#include "SerialCommands.h"

#define RETRYDELAY 5000                 //the nr of milliseconds that we pause before retrying to create the connection
#define ETHERNETDELAY 1000      //the nr of milliseconds that we pause to give the ethernet board time to start
#define MQTTPORT 1883

#ifdef DEBUG
char UARTINITTEXT[] = "initialization of UART";
char HTTPSERVTEXT[] = "connection HTTP Server";
char MQTTSERVTEXT[] = "connection MQTT Server";
char FAILED[] = " failed";
char SUCCESTXT[] = " established";
#endif

//create the object
ATTDevice::ATTDevice(Stream* stream)
{
    _stream = stream;
}

void ATTDevice::writeCommand(const char* command, String& param1, String& param2, String& param3)
{
    _stream->print(command);
    _stream->print(";");
    sendParam(param1);
    _stream->print(";");
    sendParam(param2);
    _stream->print(";");
    sendParam(param3);
    _stream->println();
}

void ATTDevice::writeCommand(const char* command, String& param1, String& param2)
{
    _stream->print(command);
    _stream->print(";");
    sendParam(param1);
    _stream->print(";");
    sendParam(param2);
    _stream->println();
}

void ATTDevice::writeCommand(const char* command, String& param1)
{
    _stream->print(command);
    _stream->print(";");
    sendParam(param1);
    _stream->println();
}

void ATTDevice::writeCommand(const char* command, String& param1, String& param2, String& param3, String& param4, String& param5)
{
    _stream->print(command);
    _stream->print(";");
    sendParam(param1);
    _stream->print(";");
    sendParam(param2);
    _stream->print(";");
    sendParam(param3);
    _stream->print(";");
    sendParam(param4);
    _stream->print(";");
    sendParam(param5);
    _stream->println();
}

void ATTDevice::sendParam(String& param)
{
    for(unsigned int i = 0; i < param.length(); i++)
    {
        char toSend = param.charAt(i);
        //if(toSend == ';')
        //    _stream->print("\;");
        //else
            _stream->print(toSend);
    }
}


// waits for string, if str is found returns ok, if other string is found returns false, if timeout returns false
bool ATTDevice::expectString(const char* str, unsigned short timeout, bool report)
{
    unsigned long start = millis();
	bool pointsDrawn = false;
    while (timeout == 0 || millis() < start + timeout)          //if timeout = 0, we wait indefinetly
    {
        if (readLn() > 0)
        {
            if (strstr(this->inputBuffer, str) != NULL)         //the serial modem can return debug statements or the expected string, allow for both.
				return true;
			else if (strstr(this->inputBuffer, STR_RESULT_NOK) != NULL)         //the serial modem can return debug statements or the expected string, allow for both.
				return false;
			if(report){
				if(pointsDrawn){										//for some pretty printing: if ... were drawn, start a new line for logging
					pointsDrawn = false;
					Serial.println("");
				}
				Serial.println(this->inputBuffer);					//only show on screen if it's not an 'ok' response.			
			}
        }
		#ifdef DEBUG    
		else{
			Serial.print(".");
			pointsDrawn = true;
		}
        #endif
    }
    return false;
}

bool ATTDevice::expectAny(unsigned short timeout)
{
	unsigned long start = millis();
    while (timeout == 0 || millis() < start + timeout)          //if timeout = 0, we wait indefinetly
    {
        #ifdef DEBUG    
        Serial.print(".");
        #endif

        if (readLn() > 0){
			delay(500);
			while(Serial.available() > 0){
				readLn();
				delay(500);
			}
			return true;
		}
    }
    return false;
}

unsigned short ATTDevice::readLn(char* buffer, unsigned short size, unsigned short start)
{
    int len = _stream->readBytesUntil('\n', buffer + start, size);
    if(len > 0)
        this->inputBuffer[start + len - 1] = 0; // bytes until \n always end with \r, so get rid of it (-1)
    else
        this->inputBuffer[start] = 0;
    return len;
}

//connect with the http server
bool ATTDevice::Init(String deviceId, String clientId, String clientKey)
{
    #ifdef DEBUG
	Serial.println();
    Serial.println(F("setting device credentials"));
    #endif
		
    writeCommand(CMD_INIT, deviceId, clientId, clientKey);
    bool res = expectString(CMD_INIT_OK, 2000);		//for init, we use a shorter wait period, so that the main application can retry quickly (resend the AT, so that the remote wifi can sync).
    #ifdef DEBUG	
    if(res == false){
        Serial.print(UARTINITTEXT);
        Serial.println(FAILED);
    }
    #endif
    return res;
}

/*Start up the wifi network*/
bool ATTDevice::StartWifi(bool forceReset)
{
    #ifdef DEBUG
    Serial.println("starting wifi");
    #endif
	bool res = false;
	if(!forceReset)								
	{
		_stream->println(CMD_AT);                   //first send the at command to synchronize: we have something to wait for an 'ok' ->could be that the wifi chip is still processing a prev command and returns 'ok', in which case the new 'init' is lost.
		res = expectString(CMD_AT_OK, 1000, true);	//if response, wifi is already set ok, otherwise wifi switched to router, we show the correct message to the user (otherwise the wifi module has to do 2 things: it's most likely already started the wifi server, but we want to send commands to it, which doesn't work		
		if(res == false)									//try the command 1 more time, could be that user did reset, in which case the wifi module might wake up a little too late to see that AT command
			res = expectString(CMD_AT_OK, 1000, true);
	}
	else{
		_stream->println(CMD_AT_RESET_WIFI);                   //first send the at command to synchronize: we have something to wait for an 'ok' ->could be that the wifi chip is still processing a prev command and returns 'ok', in which case the new 'init' is lost.
		res = expectString(CMD_AT_RESET_WIFI, 2000, true);	
	}
	if(res == false){
		Serial.println();
		if(!forceReset)
			Serial.println("The wifi module could not connect to the wifi router. The module will now switch to router mode");
		else
			Serial.println("The credentials for the wifi module has been reset. The module will now switch to router mode");
		Serial.println("Please use another device to connect to 'ESP8266 wifi'");
		Serial.println("Browse to 192.168.4.1 and set the SSID and password");
		res = expectString(CMD_AT_OK, 0, false);
	}
	return res;
}

//connect with the http server
bool ATTDevice::Connect(char httpServer[])
{
    String param = String(httpServer);
    writeCommand(CMD_CONNECT, param);
    bool res = expectString(CMD_CONNECT_OK, 0);
    #ifdef DEBUG
    if(res == false){
        Serial.print(HTTPSERVTEXT);
        Serial.println(FAILED);
    }
    #endif
    return res;
}

//create or update the specified asset.
bool ATTDevice::AddAsset(int id, String name, String description, bool isActuator, String type)
{
    #ifdef DEBUG
    Serial.println(F("adding asset"));
    #endif

    String isAct;
    if(isActuator)
		isAct = "true";
	else
		isAct = "false";
	String idStr = String(id);
	writeCommand(CMD_ADDASSET, idStr, name, description, isAct, type);
	bool res = expectString(CMD_ADDASSET_OK);
	#ifdef DEBUG
	if(res == false)
		Serial.println("Failed to add asset");
	else
		Serial.println("asset added");
	#endif
	return res;
}

//connect with the broker
bool ATTDevice::Subscribe(char broker[], mqttCallback callback)
{
    _callback = callback;
    bool res = false;
    String param = String(broker);
	writeCommand(CMD_SUBSCRIBE, param);
	res = expectString(CMD_SUBSCRIBE_OK);
	#ifdef DEBUG
	if(res == false){
		Serial.print(MQTTSERVTEXT);
		Serial.println(FAILED);
	}
	#endif
	return res;
}

//check for any new mqtt messages.
void ATTDevice::Process()
{
    _stream->println(CMD_RECEIVE);
    unsigned long start = millis();
    while (millis() < start + DEFAULT_TIMEOUT)
    {
        if (readLn() > 0)
        {
            if (strstr(this->inputBuffer, STR_RESULT_OK) != NULL) return;           //we received 'ok', so nothing to process
            else{
                #ifdef DEBUG    
                Serial.print("received: ");
                Serial.println(this->inputBuffer);
                #endif
                // Split the command in two values
                char* separator = strchr(this->inputBuffer, ';');
                *separator = 0;
                int pin = atoi(this->inputBuffer);
                ++separator;
                String value = String(separator);
                _callback(pin, value);
                return;
            }
        }
  }
}

//send a data value to the cloud server for the sensor with the specified id.
bool ATTDevice::Send(String value, int id)
{
	String idStr = String(id);
    writeCommand(CMD_SEND, value, idStr);
    bool res = expectString(CMD_SEND_OK);
    #ifdef DEBUG
    if(res == false)
        Serial.println("Failed to send value");
    else
        Serial.println("value sent");
    #endif
    return res;
}


