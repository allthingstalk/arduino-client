#include <Ethernet2.h>
#include <EthernetClient.h>


#include <PubSubClient.h>

#include <NW_WatchDog.h>
#include <ATT_IOT.h>
#include <SPI.h>                //required to have support for signed/unsigned long type.

/*
  This demo shows how you can make your embedded device more robust by introducing a 
  a network watchdog that periodically sends a ping to itself via the broker.
  If the ping fails, you can recreate the mqtt connection.
*/

char deviceId[] = "deviceId";
char clientId[] = "clientId";
char clientKey[] = "clientKey";

ATTDevice Device(deviceId, clientId, clientKey);            //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "api.AllThingsTalk.io";                   // HTTP API Server host
char* mqttServer = "broker.AllThingsTalk.io";                   

//required for the device
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);
NW_WatchDog WatchDog(pubSub, deviceId, clientId, 10000);

int ledPin = 8;

void setup()
{
  Serial.begin(9600);                                       // init serial link for debugging
  
  byte mac[] = {0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x3E};        // Adapt to your Arduino MAC Address  
  if (Ethernet.begin(mac) == 0)                             // Initialize the Ethernet connection:
  { 
    Serial.println(F("DHCP failed,end"));
    while(true);                                            //we failed to connect, halt execution here. 
  }
  delay(1000);                                              //give the Ethernet shield a second to initialize:
  
  while(!Device.Connect(&ethClient, httpServer))            // connect the device with the IOT platform.
    Serial.println("retrying");
  Device.AddAsset(ledPin, "led", "light emitting diode", true, "boolean");
  WatchDog.Setup(Device);                                   //create the required assets for the watchdog   
   while(!Device.Subscribe(pubSub))                          // make certain that we can receive message from the iot platform (activate mqtt)
    Serial.println("retrying"); 
  WatchDog.Ping();                                          //send the first ping to initiate the process.
}

void loop()
{
  
  Device.Process(); 
  if(!WatchDog.CheckPing()){                                //if the ping failed, recreate the connection.  
    Serial.println("recreating broker connection");
    while(!Device.Subscribe(pubSub))                        // make certain that we can receive message from the iot platform (activate mqtt)
        Serial.println("retrying");
	WatchDog.Ping();										//resstart the watchdog
  }
		
}


// Callback function: handles messages that were sent from the iot platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{ 
  String msgString; 
  {                                                     //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
    char message_buff[length + 1];                      //need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
    strncpy(message_buff, (char*)payload, length);      //copy over the data
    message_buff[length] = '\0';                        //make certain that it ends with a null         
          
    msgString = String(message_buff);
    msgString.toLowerCase();                            //to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
  }
  int* idOut = NULL;
  {                                                     //put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
   
    Serial.print("Payload: ");                          //show some debugging.
    Serial.println(msgString);
    Serial.print("topic: ");
    Serial.println(topic);

    int pinNr = Device.GetPinNr(topic, strlen(topic));
    
      if(!WatchDog.IsWatchDog(pinNr, msgString)){
        if(idOut != NULL)                                     //also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
          Device.Send(msgString, *idOut); 
      }
    }  
    
}
