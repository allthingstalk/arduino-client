# About #
This application is the server part that runs on the ESP8266 module itself in order to provide an AT like command set for the ESP8266 WIFI chip.  
This allows client applications to communicate with the WIFI module through the a serial interface (serial communication).

# Installation #
To upload the application to the grove version of the ESP8266 module, first create a grove cable that hooks up to the grove module on one side and to an arduino programmer on the other side.
See the wiki page of the grove module for more info: [http://www.seeedstudio.com/wiki/Grove_-_UART_WiFi](http://www.seeedstudio.com/wiki/Grove_-_UART_WiFi)
You will also need to download the arduino IDE extentions for the ESP8266 in order to upload the sketch to the module. You can find more info about this here:
- [https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide)
- [http://iot-playground.com/2-uncategorised/38-esp8266-and-arduino-ide-blink-example](http://iot-playground.com/2-uncategorised/38-esp8266-and-arduino-ide-blink-example)

# AT commands #
## Format ##
Format = command [param {‘;’ param} \n
## supported commands ##
The system currently supports the following AT commands:

### AT ###
Check if connection ok.
Params: none
Returns: ‘HELLO’
###ATI###
Initialize
Params: 
•	deviceId: string, the id of the device.
•	Clientid: string, the id of the client
•	clientKey: string, the client key
Returns: ‘okAT’
###ATW###
Start wifi
Params: 
•	ssid: string, name of the router
•	pwd: string, pwd for the router
Returns: ‘okATW’
###ATC###
Connect to the http server
Params: 
•	http server: string, dns name of the http api server
Returns: ‘okATC’
###ATB###
Subscribe to the broker
Params: 
•	mqtt server: string, dns name of the broker
Returns: ‘okATB’
###ATA###
Add asset
Params:  same as call to ‘addAsset’ for regular lib
Returns: ‘okATA’
###ATS###
Send data over mqtt
Params:  same as call to ‘Send’ for regular lib
Returns: ‘okATS’
###ATR###
Receive data
Params: none
Returns:
When no values: ‘ok’
otherwise:
•	pin nr: string
•	payload: string value

