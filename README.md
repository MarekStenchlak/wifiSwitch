# wifiSwitch

## MQTT wifi wall switch using ESP8266-01

### Description
Switch can be controlled via MQTT or touch button.

Board can be ordered on OSH park:
  https://oshpark.com/shared_projects/9JxnbNwu

### Schematics
Schematics for this project was designed using Eagle.

Eagle project can be found in repository.

![Eagle Scheme](/images/wifiSwitch_scheme.png?raw=true "Schematics")

### Board

![Eagle Board](/images/wifiSwitch_board.png?raw=true "Board")

### BOM (Bill Of Materials)
*TODO*

### 3D model
3D model was designed in Sketch-Up and exported to STL files.

Model consists of 3 parts: box, frame and button.

3D model files and exported STL files can be found in repository.

Exploded view looks like this:

![3D exploded view](/images/èxploded.png?raw=true "3D exploded")

## Setup

### Wiring
*TODO*

### MQTT broker
To use this switch you need to have an MQTT broker like [Mosquitto](https://mosquitto.org/) installed on a server or RaspeberryPi in your local wifi network.

To install Mosquitto, please follow this guide: [Setup mosquitto MQTT broker](https://www.digitalocean.com/community/tutorials/how-to-install-and-secure-the-mosquitto-mqtt-messaging-broker-on-ubuntu-16-04)

or this guie for RaspberryPi: [Setup mosquitto MQTT broker on RPi]:(http://www.instructables.com/id/Installing-MQTT-BrokerMosquitto-on-Raspberry-Pi/)

The bast practise is to set static IP address to the MQTT broker server or raspberryPi running mosquitto.

### WifiSwitch settings
This switch is using [WifiManager]:(https://github.com/tzapu/WiFiManager) project to set-up switch parameters.

When the switch is connected to mains, it tries to connect to configured Wifi network and MQTT broker. If it is not able to connect or is not configured, it enters the configuration mode.

#### Configuration mode
  When in configuration mode, the wsitch creates its own wifi network.
  This network`s SSID consists of string "ESP_HOME" followed by its MAC address 
  
    e.g: ESP_HOME A1:B2:3A:4E:2D:E2
    
  This network has no Password.
  
  After you connect to this network (e.g using smartphone) enter IP address **192.168.4.1**
  
  Then configure:
    wifi SSID
    wifi Password
    MQTT broker IP (or DNS name)
    MQTT broker port
    MQTT username
    MQTT password
    MQTT topic
    
