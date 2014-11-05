#include <Ethernet.h>					//for the pub/sub client
#include <PubSubClient.h>				//sends data to the IOT platform


#include <SPI.h>                        //required to have support for signed/unsigned long type.
#include <XBee.h>
#include <iot_att_gateway.h>			//routines for working with the iot platform

/*
  AllThingsTalk Makers Arduino-gateway template
  This is a template sketch which handels XBEE communication for the Plant Sensor project.
  It presumes that all xbee-devices that connect to this gateway will always have the same assets.

  Usage: 
  - fill in your client id, client key & mac address
  - define the sensors & actuators that will be available for each device connected to the xbee-controller.
  - in NodeIdResponse: create the device & all the assets when a new xbee has connected to the xbee-controller.
  - in SampleResponse: upload the data for all the sensors whenever a new sample packet comes in from the xbee.
  - in callback: send the value to the xbee-device when an actuator value is sent from the IOT platform to the gateway.
*/

byte mac[] = {  0x90, 0xA2, 0xDA, 0x0D, 0xE1, 0x4B }; 	    // Adapt to your Arduino MAC Address  

//IOT stuff
char clientId[] = "Your client id comes here"; 
char clientKey[] = "Your client key comes here";

ATTGateway Gateway(clientId, clientKey);                        //create the object that provides the connection to the cloud to manager the device.
char httpServer[] = "beta.smartliving.io";                      // HTTP API Server host
char* mqttServer = "broker.smartliving.io";                     // MQTT(ATT1) Server 
void callback(char* topic, byte* payload, unsigned int length);
EthernetClient ethClient;
PubSubClient pubSub(mqttServer, 1883, callback, ethClient);

String Sensor1 = "Moisture sensor";                       
char Sensor1_Id = '1';
String Sensor2 = "Temp Sensor";                         
char Sensor2_Id = '2';
String Sensor3 = "Light Sensor";
char Sensor3_Id = '3';


// XBEE Stuff
uint8_t payload[50];

XBee xbee = XBee();
ZBRxIoSampleResponse ioSample = ZBRxIoSampleResponse();
XBeeResponse response = XBeeResponse();
ZBRxResponse rx = ZBRxResponse();                                                // create reusable response objects for responses we expect to handle 
RemoteAtCommandResponse remoteAtResponse = RemoteAtCommandResponse();            // Create a Remote AT response object


void setup()
{
  Serial.begin(9600);                                            // init serial link for debugging
  
  // start the Ethernet connection:
  if(Gateway.Connect(mac, httpServer))                           // connect the device with the IOT platform.
    Gateway.Subscribe(pubSub);                                   // make certain that we can receive message from the iot platform (activate mqtt)
  else 
    while(true);                                                 //can't set up the gateway on the cloud, can't continue, so put the app in an ethernal loop so it doesn't do anything else anymore.								
  
  delay(5000);
  Serial.println("Starting Up XBEE communication");  
  xbee.begin(Serial);
}

//extract the device id from an xbee data packet
String GetDeviceId(XBeeResponse &response)
{
  uint32_t msb,lsb;                                             // xbee most and least address bits (64 bit address)
  msb = rx.getRemoteAddress64().getMsb();
  lsb = rx.getRemoteAddress64().getLsb();
  return String(msb, HEX) + String(lsb, HEX);
}

//called when a new device is connected to the xbee controller
void NodeIdResponse()
{
  Serial.println(F("device connected"));
  xbee.getResponse().getZBRxResponse(rx);
  String devId = GetDeviceId(rx);
  if(Gateway.DeviceExists(devId) == false)
  {
    Gateway.AddDevice(devId, "The name of the device", "The description of the device");        //replace values here
    //adjust the following list according to your needs.
    Gateway.AddAsset(devId, Sensor1_Id, Sensor1, "description of the asset", false, "int");     //replace values here
    Gateway.AddAsset(devId, Sensor2_Id, Sensor2, "description of the asset", false, "int");     //replace values here
    Gateway.AddAsset(devId, Sensor3_Id, Sensor3, "description of the asset", false, "int");     //replace values here
  }
}
//display the xbee source of the incomming data
void PrintDataSource(XBeeResponse &response)
{
  Serial.print(F(" MSB Address = "));Serial.println(rx.getRemoteAddress64().getMsb(),HEX);
  Serial.print(F(" LSB Address = "));Serial.println(rx.getRemoteAddress64().getLsb(),HEX);
}

void SampleResponse()
{
  xbee.getResponse().getZBRxIoSampleResponse(ioSample);
  Serial.println(F("Received I/O Sample from: ")); 
  PrintDataSource(ioSample);
  // read analog inputs
  for (int i = 0; i <= 4; i++) {
    if (ioSample.isAnalogEnabled(i)) {
      Serial.print(F("Analog (AI"));
      Serial.print(i, DEC);
      Serial.print(F(") is "));
      Serial.println(ioSample.getAnalog(i), DEC);
    }
  }
  // read digital inputs
  for (int i = 0; i <= 12; i++) {
    if (ioSample.isDigitalEnabled(i)) {
      Serial.print(F("Digital (DI"));
      Serial.print(i, DEC);
      Serial.print(F(") is "));
      Serial.println(ioSample.isDigitalOn(i), DEC);          
    }
  }
  // Send Plantsensor values to ATt Platform
  String devId = GetDeviceId(rx);
  Gateway.Send(devId, Sensor1_Id, String(ioSample.getAnalog(1)));         //adjust to your assets
  Gateway.Send(devId, Sensor2_Id, String(ioSample.getAnalog(2)));
  Gateway.Send(devId, Sensor3_Id, String(ioSample.getAnalog(3)));
}

// XBEE RECEIVING FUNCTION
// receives XBEE RX responses
// Source xbee address stored in msb,lsb global vars
void XBEE_RX() 
{
  xbee.readPacket();                                                //attempt to read a packet    
  
  XBeeResponse &response = xbee.getResponse();
  if (response.isAvailable()) {
    Serial.println("new xbee data");
    if (response.getApiId() == ZB_IO_NODE_IDENTIFIER_RESPONSE)      // 0x95
      NodeIdResponse();  
    else if (response.getApiId() == ZB_IO_SAMPLE_RESPONSE)         // 0x92
      SampleResponse();
  }
}

void loop()
{
  XBEE_RX();						// Check if XBEE received data
  Gateway.Process();  				//process incomming iot data
}


// Callback function: handles messages that were sent from the ALLTHINGSTALK IoT platform to this device.
void callback(char* topic, byte* payload, unsigned int length) 
{  
  char message_buff[length + 1];						//need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
	strncpy(message_buff, (char*)payload, length);		//copy over the data
	message_buff[length + 1] = '\0';							    //make certain that it ends with a null			
	  
  String msgString = String(message_buff);
  msgString.toLowerCase();							//to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
	
  Serial.println("Payload: " + msgString);			                //show some debugging.
  Serial.println("topic: " + topic);
	

}

