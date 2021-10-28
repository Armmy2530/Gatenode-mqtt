#define WEBSERVER_H

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid         = "floor_1";
const char* password     = "83005097";
const char* mqttServer   = "192.168.2.56";
const int  mqttPort     = 1883;
const char* mqttUser     = "armmy2530";
const char* mqttPassword = "arm254889";
unsigned long int currentTime = millis();
unsigned long int previousTime = 0;
String currentMode = "online";
#define PUB_GPIO2_CONNECTION "tele/gatenode/LWT"
#define PUB_GPIO2_VERSION "tele/gatenode/version"
#define PUB_GPIO2_MODE "stat/gatenode/mode"
#define PUB_GPIO2_ACTIVE "stat/gatenode/active"
#define SUB_GPIO2_ACTION "cmnd/gatenode"
#define Version "0.5.3"
#define GPIO2_LED 3

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WiFiManager wifiManager;
AsyncWebServer server(80);

bool shouldSaveConfig1 = false;
void saveConfigCallback1 () {
  shouldSaveConfig1 = true;
  Serial.println("Should save  All Data config");
}

void initWifiStation() {
  WiFiManagerParameter custom_text ( " <p>  </p> " );
  WiFiManagerParameter custom_text0 ( " <label> ป้อน MQTT IP </label> " );
  WiFiManagerParameter custom_text1 ( " <label> ป้อน MQTT USERNAME </label> " );
  WiFiManagerParameter custom_text2 ( " <label> ป้อน MQTT PASSWORD </label> " );
  WiFiManagerParameter custom_mqtt_ip("LINE", "", mqttServer, 20);
  WiFiManagerParameter custom_mqtt_username("latitude", "", mqttUser, 20);
  WiFiManagerParameter custom_mqtt_password("longitude", "", mqttPassword, 30);
  wifiManager.setSaveConfigCallback(saveConfigCallback1);
  wifiManager.addParameter (& custom_text);
  wifiManager.addParameter (& custom_text0);
  wifiManager.addParameter(&custom_mqtt_ip);
  wifiManager.addParameter (& custom_text1);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter (& custom_text2);
  wifiManager.addParameter(&custom_mqtt_password);
  delay(1000);
  Serial.print("\nConnecting to WiFi");
  if (!wifiManager.autoConnect("Gatenode__config AP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
  mqttServer = custom_mqtt_ip.getValue();
  mqttUser = custom_mqtt_username.getValue();
  mqttPassword = custom_mqtt_password.getValue();
  delay(1000);
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(PubSubCallback);
  while (!mqttClient.connected()) {
    delay(100);
    Serial.println("Connecting to MQTT (" + String(mqttServer) + ") ...");
    if (mqttClient.connect("Gatenode_mk3", mqttUser, mqttPassword,PUB_GPIO2_CONNECTION,0,true,"Offline")) {
      Serial.println("MQTT client connected");
      mqttClient.publish(PUB_GPIO2_CONNECTION, "Online",true);
    } 
    else {
      Serial.print("\nFailed with state ");
      Serial.println(mqttClient.state());
        if (WiFi.status() != WL_CONNECTED) {
          initWifiStation();
        }
      }
    delay(2000);
  }
  // Declare Pub/Sub topics
  AsyncElegantOTA.begin(&server); 
  server.begin();
  Serial.println("HTTP server started");
  mqttClient.subscribe(SUB_GPIO2_ACTION);
  mqttClient.publish(PUB_GPIO2_MODE, "deactive");
}
void checkconnection(){
  while (!mqttClient.connected()) {
    delay(100);
    Serial.println("Connecting to MQTT (" + String(mqttServer) + ") ...");
    if (mqttClient.connect("Gatenode_mk3", mqttUser, mqttPassword,PUB_GPIO2_CONNECTION,0,true,"Offline")) {
      Serial.println("MQTT client connected");
      mqttClient.publish(PUB_GPIO2_CONNECTION, "Online",true);
    } 
    else {
      Serial.print("\nFailed with state ");
      Serial.println(mqttClient.state());
        if (WiFi.status() != WL_CONNECTED) {
          initWifiStation();
        }
      }
    delay(2000);
  }
}

void PubSubCallback(char* topic, byte* payload, unsigned int length) {
  String strTopicGpio2Action = SUB_GPIO2_ACTION;
  String strON = "active";
  String strOFF = "deactive";
  String strOnline = "online";
  String strOffline = "offline";
  String strPayload = "";
  Serial.print("Topic:");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    strPayload += (char)payload[i];
  }
  Serial.println();
  Serial.println("-----------------------");

  if (strTopicGpio2Action == topic) {
    if (strON == strPayload && currentMode == strOnline) {
      mqttClient.publish(PUB_GPIO2_MODE, "active");
      digitalWrite(GPIO2_LED, HIGH);
      delay(500);
      digitalWrite(GPIO2_LED, LOW);
      mqttClient.publish(PUB_GPIO2_MODE, "deactive");
      Serial.println("gate opening");
    } 
    else if (strON == strPayload && currentMode == strOffline) {
      Serial.println("Current mode is offline");
    } 
    else if (strOFF == strPayload) {
      digitalWrite(GPIO2_LED, LOW);
    }
    else if (strOnline == strPayload)
    {
      currentMode = strOnline;
      Serial.println("Current mode has set to " + currentMode);
      mqttClient.publish(PUB_GPIO2_MODE, "online");
    }
    else if (strOffline == strPayload)
    {
      currentMode = strOffline;
      Serial.println("Current mode has set to " + currentMode);
      mqttClient.publish(PUB_GPIO2_MODE, "offline");
    }
    else if (strPayload == "wifireset"){
      Serial.println("reset config");
      wifiManager.resetSettings();
      delay(100);
      ESP.reset();
    }
    else if (strPayload == "restart"){
      Serial.println("restart");
      mqttClient.publish(PUB_GPIO2_CONNECTION, "Offline");
      delay(100);
      ESP.reset();
    }
  }
}


void setup() {
  Serial.begin(115200);
  pinMode(GPIO2_LED, OUTPUT);
  digitalWrite(GPIO2_LED, LOW);
  Serial.println("version: "+ String(Version));
  initWifiStation();
  mqttClient.publish(PUB_GPIO2_VERSION, Version);
}

void loop() {
  mqttClient.loop();
  checkconnection();
  AsyncElegantOTA.loop();
  currentTime = millis();
}