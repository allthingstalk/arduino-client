arduino-client
==============

libraries for arduinos that provides access to the ATT IOT platform.
There are 3 different flavours of the IOT library in order to perform the best balance between memory usage and automation.

Check the wiki pages for [how-to's and API documentation](https://github.com/allthingstalk/arduino-client/wiki).

### flavours
  1. Minimal: uses the least amount of memory, but it requires you to create devices and assets manually from the IOT platform.
  2. regular: requires more memory then the minimal version, but generates the assets automatically from the script. The device needs to be created manually in the IOT platform.
  3. gateway: requires most memory. Can function as a gateway for other devices that are wirelessly connected through xbee. The included template scketch shows how evices & assets can automatically be created whenever a new xbee connects to the controller.

### Installation
  1. Download the source code
  2. copy the directories in the 'library' directory to your arduino library directory.
  3. copy the directories in the 'dependencies' directory to your arduino library directory. (These are external arduino libraries that are either used by the IOT libraries or on of the demo templates).
  4. Use the 'demo' templates as a starting point.
  
### Instructions

For all flavours, the basic instructions are always the same.  It is best to start from one of the 'demo' templates so that you only need to replace some values and possibly add/remove a couple of lines in order to support different sensors and actuators.

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Add 'iot_att' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Create the required items in the IOT platform. This is different for each flavour:
    - minimal: create the device and all the assets for the device. Don't forget to specify the data type of the asset in the profile. Ex: {"type": "int"}  Other possible types are: double, bool, string, datetime, timespan
	- regular: create the device.
  4. fill in the missing strings. This depends on the library flavour that you are using.
	- minimal: replace DeviceId & clientId & mac.  Also include the assetId's for every sensor/actuator that you want to use in your sketch.  All these values can be found on the IOT platform, on the pages of the device/assets that you just created in the previous step. The mac address can be found on your arduino board.
	- regular: replace deviceId, clientId, clientKey, mac. Optionally change/add the sensor & actuator names, ids, descriptions, types. For extra actuators, make certain to extend the callback code at the end of the sketch. The id's for the actuators & sensors that you have to specify here, should be unique within your sketch. For instance: 1, 2, 3,...  The mac address can be found on your arduino board. DeviceId, clientId & clientKey can be found on the IOT platform. 
	- gateway: replace the clientId, clientKey & mac. The mac address can be found on your arduino board. ClientId & clientKey are available on the IOT platform.
  4. Upload the sketch
  
