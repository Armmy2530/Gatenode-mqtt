#define WEBSERVER_H

#include <FS.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <ArduinoJson.h>

const char *ssid = "floor_1";
const char *password = "83005097";
unsigned long int currentTime = millis();
unsigned long int previousTime = 0;
String currentMode = "online";

#define PUB_GPIO2_CONNECTION "tele/gatenode/LWT"
#define PUB_GPIO2_VERSION "tele/gatenode/version"
#define PUB_GPIO2_MODE "stat/gatenode/mode"
#define PUB_GPIO2_ACTIVE "stat/gatenode/active"
#define SUB_GPIO2_ACTION "cmnd/gatenode"
#define Version "0.5.4"
#define GPIO2_RELAY 3
#define mqttServer "192.168.2.56"
#define mqttPort 1883
#define mqttUser "armmy2530"
#define mqttPassword "arm254889"

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WiFiManager wifiManager;
AsyncWebServer server(80);

bool shouldSaveConfig = false;

void saveConfigCallback()
{
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void initWifiStation()
{
  //wifimanager
  WiFiManagerParameter custom_text(" <p>  </p> ");
  WiFiManagerParameter custom_text0(" <label> ป้อน MQTT IP </label> ");
  WiFiManagerParameter custom_text1(" <label> ป้อน MQTT USERNAME </label> ");
  WiFiManagerParameter custom_text2(" <label> ป้อน MQTT PASSWORD </label> ");
  WiFiManagerParameter custom_mqtt_ip("mqttserver", "", mqttServer, 20);
  WiFiManagerParameter custom_mqtt_username("mqttuser", "", mqttUser, 20);
  WiFiManagerParameter custom_mqtt_password("mqttpass", "", mqttPassword, 30);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_text0);
  wifiManager.addParameter(&custom_mqtt_ip);
  wifiManager.addParameter(&custom_text1);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_text2);
  wifiManager.addParameter(&custom_mqtt_password);
  delay(1000);
  Serial.print("\nConnecting to WiFi");
  if (!wifiManager.autoConnect("Gatenode__config AP"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  strcpy(mqttServer, custom_mqtt_ip.getValue());
  strcpy(mqttUser, custom_mqtt_username.getValue());
  strcpy(mqttPassword, custom_mqtt_password.getValue());
  Serial.println("Mqtt server: " + String(mqttServer));
  Serial.println("Mqtt server: " + String(mqttUser));
  Serial.println("Mqtt server: " + String(mqttPassword));

  //saveconfig
  if (shouldSaveConfig)
  {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
    json["mqttServer"] = mqttServer;
    json["mqttUser"] = mqttUser;
    json["mqttPassword"] = mqttPassword;
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile)
    {
      Serial.println("failed to open config file for writing");
    }
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  //Mqtt connection
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(PubSubCallback);
  if (mqttClient.connect("Gatenode_mk3", mqttUser, mqttPassword, PUB_GPIO2_CONNECTION, 0, true, "Offline"))
  {
    Serial.println("MQTT client connected");
    mqttClient.publish(PUB_GPIO2_CONNECTION, "Online", true);
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
    if (WiFi.status() != WL_CONNECTED)
    {
      initWifiStation();
    }
  }
  delay(2000);

  //AsyncOTA
  AsyncElegantOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server started");

  // Declare Pub/Sub topics
  mqttClient.subscribe(SUB_GPIO2_ACTION);
  mqttClient.publish(PUB_GPIO2_ACTIVE, "deactive");
}
void checkconnection()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("Gatenode_mk3", mqttUser, mqttPassword, PUB_GPIO2_CONNECTION, 0, true, "Offline"))
    {
      Serial.println("MQTT client connected");
      mqttClient.setCallback(PubSubCallback);
      mqttClient.subscribe(SUB_GPIO2_ACTION);
      mqttClient.publish(PUB_GPIO2_ACTIVE, "deactive");
      mqttClient.publish(PUB_GPIO2_CONNECTION, "Online", true);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      if (WiFi.status() != WL_CONNECTED)
      {
        initWifiStation();
      }
    }
    delay(2000);
  }
}

void PubSubCallback(char *topic, byte *payload, unsigned int length)
{
  String strTopicGpio2Action = SUB_GPIO2_ACTION;
  String strON = "active";
  String strOFF = "deactive";
  String strOnline = "online";
  String strOffline = "offline";
  String strPayload = "";
  Serial.print("Topic:");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    strPayload += (char)payload[i];
  }
  Serial.println();

  if (strTopicGpio2Action == topic)
  {
    if (strON == strPayload && currentMode == strOnline)
    {
      mqttClient.publish(PUB_GPIO2_ACTIVE, "active");
      digitalWrite(GPIO2_RELAY, LOW);
      delay(500);
      digitalWrite(GPIO2_RELAY, HIGH);
      mqttClient.publish(PUB_GPIO2_ACTIVE, "deactive");
      Serial.println("gate opening");
    }
    else if (strON == strPayload && currentMode == strOffline)
    {
      Serial.println("Current mode is offline");
    }
    else if (strOFF == strPayload)
    {
      digitalWrite(GPIO2_RELAY, HIGH);
    }
    else if (strOnline == strPayload)
    {
      currentMode = strOnline;
      Serial.println("Current mode has set to " + currentMode);
      mqttClient.publish(PUB_GPIO2_MODE, "online", true);
    }
    else if (strOffline == strPayload)
    {
      currentMode = strOffline;
      Serial.println("Current mode has set to " + currentMode);
      mqttClient.publish(PUB_GPIO2_MODE, "offline", true);
    }
    else if (strPayload == "wifireset")
    {
      Serial.println("reset config");
      wifiManager.resetSettings();
      delay(100);
      ESP.reset();
    }
    else if (strPayload == "restart")
    {
      Serial.println("restart");
      mqttClient.publish(PUB_GPIO2_CONNECTION, "Offline");
      SPIFFS.format();
      delay(100);
      ESP.reset();
    }
  }
  Serial.println("-----------------------");
}

void SPIFFS_setup()
{
  Serial.println("mounting FS...");
  if (SPIFFS.begin())
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json"))
    {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile)
      {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject &json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success())
        {
          Serial.println("\nparsed json");
          strcpy(mqttServer, json["mqttServer"]);
          strcpy(mqttUser, json["mqttUser"]);
          strcpy(mqttPassword, json["mqttPassword"]);
        }
        else
        {
          Serial.println("failed to load json config");
        }
      }
    }
  }
  else
  {
    Serial.println("failed to mount FS");
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(GPIO2_RELAY, OUTPUT);
  digitalWrite(GPIO2_RELAY, HIGH);
  Serial.println("version: " + String(Version));
  SPIFFS_setup();
  initWifiStation();
  mqttClient.publish(PUB_GPIO2_VERSION, Version);
}

void loop()
{
  mqttClient.loop();
  AsyncElegantOTA.loop();
  checkconnection();
  currentTime = millis();
}