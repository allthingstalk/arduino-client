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
char FAILED_RETRY[] = " failed,retry";
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
    for(int i = 0; i < param.length(); i++)
    {
        char toSend = param.charAt(i);
        if(toSend == ';')
            _stream->print("\;");
        else
            _stream->print(toSend);
    }
}


// waits for string, if str is found returns ok, if other string is found returns false, if timeout returns false
bool ATTDevice::expectString(const char* str, unsigned short timeout)
{
    unsigned long start = millis();
    while (timeout == 0 || millis() < start + timeout)          //if timeout = 0, we wait indefinetly
    {
        #ifdef DEBUG    
        Serial.print(".");
        #endif

        if (readLn() > 0)
        {
			Serial.println();	
            // TODO make more strict?
            if (strstr(this->inputBuffer, str) != NULL)         //the serial modem can return debug statements or the expected string, allow for both.
				return true;
			#ifdef DEBUG        
            Serial.println(this->inputBuffer);					//only show on screen if it's not an 'ok' response.			
            #endif
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
    Serial.println(F("initializing serial connection with wifi module"));
    #endif
	
	_stream->println(CMD_AT);                   //first send the at command to synchronize: we have something to wait for an 'ok' ->could be that the wifi chip is still processing a prev command and returns 'ok', in which case the new 'init' is lost.
    bool res = expectString(CMD_AT_OK);
	if(res == false){
		#ifdef DEBUG
		Serial.print(UARTINITTEXT);
		Serial.println(FAILED_RETRY);
		#endif
		return res;
	}
	
    writeCommand(CMD_INIT, deviceId, clientId, clientKey);
    res = expectString(CMD_INIT_OK);
    #ifdef DEBUG
    if(res == false){
        Serial.print(UARTINITTEXT);
        Serial.println(FAILED_RETRY);
    }
    #endif
    return res;
}

/*Start up the wifi network*/
void ATTDevice::StartWifi(String ssid, String pwd)
{
    #ifdef DEBUG
    Serial.println("starting wifi");
    #endif
    writeCommand(CMD_WIFI, ssid, pwd);
    bool res = expectString(CMD_WIFI_OK, 0);    //we wait indefinitely
    #ifdef DEBUG
    if(res == false)
        Serial.println("failed to start wifi, retrying...");
    else
        Serial.println("wifi started");
    #endif
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
        Serial.println(FAILED_RETRY);
    }
    else
    {
        Serial.print(HTTPSERVTEXT);
        Serial.println(SUCCESTXT);
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
void ATTDevice::Subscribe(char broker[], mqttCallback callback)
{
    _callback = callback;
    #ifdef DEBUG
    Serial.println(F("Stopping HTTP, starting mqtt"));
    #endif
    bool res = false;
    while(!res){
        String param = String(broker);
        writeCommand(CMD_SUBSCRIBE, param);
        res = expectString(CMD_SUBSCRIBE_OK);
        #ifdef DEBUG
        if(res == false){
            Serial.print(MQTTSERVTEXT);
            Serial.println(FAILED_RETRY);
        }
        else{
            Serial.print(MQTTSERVTEXT);
            Serial.println(SUCCESTXT);
        }
        #endif
    }
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


