arduino-client
==============

A library for arduinos that provides access to the ATT IOT platform.

Check the wiki pages for [how-to's and API documentation](https://github.com/allthingstalk/arduino-client/wiki).

### Installation
  1. Download the source code
  2. copy the directories in the 'library' directory to your arduino library directory.
  3. copy the directories in the 'dependencies' directory to your arduino library directory. (These are external arduino libraries that are either used by the IOT libraries or on of the demo templates).
  4. Use the 'demo' templates as a starting point.
  
### Instructions

It is best to start from one of the 'example' templates so that you only need to replace some values and possibly add/remove a couple of lines in order to support different sensors and actuators.

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Add 'allthingstalk_arduino_standard_lib' library to your Arduino Environment. [Try this guide](http://arduino.cc/en/Guide/Libraries)
  3. Create the device in the IOT platform.
  4. fill in the missing strings: replace deviceId, clientId, clientKey, mac. Optionally change/add the sensor & actuator names, pins, descriptions, types. For extra actuators, make certain to extend the callback code at the end of the sketch. The pins for the actuators & sensors that you have to specify here are the same pin values that you use in the sketch to initialize the sensors/actuators.  The mac address can be found on your arduino board. DeviceId, clientId & clientKey can be found on the IOT platform. 
  4. Upload the sketch
  
