#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <PubSubClient.h>         //https://github.com/knolleary/pubsubclient
#include "OneButton.h"            //https://github.com/mathertel/OneButton

#define BUTTON_PIN       0
#define RELAY_PIN       2
#define DISCOVERY_TOPIC "discovery"
#define STATE_SUFFIX "/state"
#define LONG_PRESS_RESET_VALUE 5 // time to reset ESP8266 to Configuration state in seconds

//flag for saving data
bool shouldSaveConfig = false;

//define your default values here, if there are different values in config.json, they are overwritten.
//char mqtt_server[40];
char mqtt_server[50];
char mqtt_port[6];
char mqtt_user[40];
char mqtt_pass[40];
char mqtt_topic[45];

volatile boolean relayStatus = true; // The logic is inverted, switch is opened on "1" .
volatile boolean isPublishStatus = false;

WiFiManager wifiManager;

WiFiClient espClient;
PubSubClient client(espClient);

OneButton button(BUTTON_PIN, true);

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

// MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  
  if (strcmp(topic, mqtt_topic) == 0){
    // Switch ON the relay if "1" was received as the first character on the topic
    if ((char)payload[0] == '1') {
      // inverse logic because of inverter in the circuit
      relayStatus = false;
    }  
    // Switch OFF the relay if "0" was received as the first character on the topic
    if ((char)payload[0] == '0') {
      // inverse logic because of inverter in the circuit
      relayStatus = true;
    }
    // Toggle the relay if "2" was received as the first character on the topic
    if ((char)payload[0] == '2') {
      relayStatus = !relayStatus;
    }
    // MQTT command to reset the ESP
    // Reset the ESP if "x" was received as the first character on the topic
    if ((char)payload[0] == 'x')
    {
      Serial.println("Reseting ESP...");
      //client.disconnect();
      runOnDemandSetup();
    }
    // debug output
    Serial.println(relayStatus);
    
    digitalWrite(RELAY_PIN, relayStatus);
    publishStatus(mqtt_topic);
  }
  
  if (strcmp(topic, DISCOVERY_TOPIC) == 0)
  {
    if ((char)payload[0] == '1') {
      publishStatus(mqtt_topic);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  //Serial.begin(115200);
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  Serial.println();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, relayStatus);

  // set Touch pin to input  > interupt
  pinMode(BUTTON_PIN, INPUT);
  
  button.attachClick(oneClickFunction);
  button.attachLongPressStart(longPressStart);
  button.attachLongPressStop(longPressStop);
  button.attachDuringLongPress(longPress);
  button.setPressTicks(LONG_PRESS_RESET_VALUE*1000);
  //clean FS for testing 
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(mqtt_user, json["mqtt_user"]);
          strcpy(mqtt_pass, json["mqtt_pass"]);
          strcpy(mqtt_topic, json["mqtt_topic"]);
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, 20);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, 20);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic", mqtt_topic, 25);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
//  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_mqtt_topic);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimum quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(180);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration

  String wifiName = "ESP_HOME ";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  wifiName += macToStr(mac);

  char WN[40];
  wifiName.toCharArray(WN, 40);
  
  if (!wifiManager.autoConnect(WN)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    
   // wifiManager.resetSettings();
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
 

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);
}

void publishStatus(char* topicToPublish)
{
  char buf[40];
  char payload[4];
  strcpy(buf,topicToPublish);
  strcat(buf,STATE_SUFFIX);
  if(relayStatus == true)
    strcpy(payload,"0");
  else
    strcpy(payload,"1");
  client.publish(buf, payload);
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //digitalWrite(RELAY_PIN, false);
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    //set client id
  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  uint8_t mac[6];
  button.tick();
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  char id[35];
  clientName.toCharArray(id, 35);
    if (client.connect(id, mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
      client.subscribe(DISCOVERY_TOPIC);
      Serial.print("Subscribed to topics: ");
      Serial.println(mqtt_topic);
      Serial.println(DISCOVERY_TOPIC);
      publishStatus(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void runOnDemandSetup()
{
  if (client.connected())
  {
    client.disconnect();
  }
  SPIFFS.format();
  wifiManager.resetSettings();
  ESP.restart();
  //setup();
}

// Button handling 
void oneClickFunction() {
  // callback for button click, sets the flag to toggle switch
  isPublishStatus = true;
}

void longPressStart() {
  relayStatus = !relayStatus;
  publishStatus(mqtt_topic);
  runOnDemandSetup();
} // longPressStart

void longPress() {
  
} // longPress

void longPressStop() {
    
} // longPressStop

// main loop
void loop() {
  // put your main code here, to run repeatedly:
  
  if (!client.connected()) {
    reconnect();
  }
  // if flag from button click is set to true
  if (isPublishStatus)
  {
    relayStatus = !relayStatus;
    digitalWrite(RELAY_PIN, relayStatus);
    publishStatus(mqtt_topic);
    Serial.println("Change from button !");  
    isPublishStatus = false;
  }
  button.tick();
  client.loop();
}
