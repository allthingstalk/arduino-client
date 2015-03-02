arduino-client
==============

A library for arduinos that provides access to the ATT IOT platform.

Check the wiki pages for [how-to's and API documentation](https://github.com/allthingstalk/arduino-client/wiki).

### Installation
  1. Download the source code
  2. copy the content of the 'libraries' directory in the zip file to your arduino libraries folder.
  
### Instructions

  1. Setup the Arduino hardware
    - USB2Serial
    - Grove kit shield
    - Potentiometer to A0
    - Led light to D8
  2. Create the device in the IOT platform.
  3. Open the 'iot_demo' template sketch.
  3. fill in the missing strings: replace deviceId, clientId, clientKey, mac. Optionally change/add the sensor & actuator names, pins, descriptions, types. 
  4. Upload the sketch

### Extra info

- For extra actuators, make certain to extend the callback code at the end of the sketch. The pins for the actuators & sensors that you have to specify here are the same pin values that you use in the sketch to initialize the sensors/actuators.  
- The mac address can be found on your arduino board. 
- DeviceId, clientId & clientKey can be found on the IOT platform. 
