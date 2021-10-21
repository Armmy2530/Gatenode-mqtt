#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DNSServer.h>
#include <WiFiManager.h>

const char* ssid         = "";
const char* password     = "";
const char* mqttServer   = "192.168.2.56";
const int  mqttPort     = 1883;
const char* mqttUser     = "armmy2530";
const char* mqttPassword = "arm254889";

#define PUB_GPIO2_STATUS "cmnd/gatenode/status"
#define SUB_GPIO2_ACTION "cmnd/gatenode/cmnd"
#define GPIO2_LED 3

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
WiFiManager wifiManager;

bool shouldSaveConfig1 = false;
String serial_raw = "";
void saveConfigCallback1 () {
  shouldSaveConfig1 = true;
  Serial.println("Should save  All Data config");
}

void initWifiStation() {
  WiFiManagerParameter custom_text ( " <p>  </p> " );
  WiFiManagerParameter custom_text0 ( " <label> ป้อน MQTT IP </label> " );
  WiFiManagerParameter custom_text1 ( " <label> ป้อน MQTT USERNAME </label> " );
  WiFiManagerParameter custom_text2 ( " <label> ป้อน MQTT PASSWORD </label> " );
  WiFiManagerParameter custom_mqtt_ip("LINE", "line_token", mqttServer, 15);
  WiFiManagerParameter custom_mqtt_username("latitude", "latitude", mqttUser, 15);
  WiFiManagerParameter custom_mqtt_password("longitude", "longitude", mqttPassword, 30);
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
  Serial.println(String("\nConnected to the WiFi network (") + ssid + ")" );
  Serial.println("Mqtt ip: "+ String(mqttServer));
  Serial.println("Mqtt username: "+ String(mqttUser));
  Serial.println("Mqtt password: "+ String(mqttPassword));
}

void initMQTTClient() {

  // Connecting to MQTT server
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(PubSubCallback);
  while (!mqttClient.connected()) {
    Serial.println(String("Connecting to MQTT (") + mqttServer + ")...");
    if (mqttClient.connect("ESP8266Client", mqttUser, mqttPassword)) {
      Serial.println("MQTT client connected");
    } else {
      Serial.print("\nFailed with state ");
      Serial.println(mqttClient.state());

      if (WiFi.status() != WL_CONNECTED) {
        initWifiStation();
      }
      delay(2000);
    }
  }

  // Declare Pub/Sub topics
  mqttClient.publish(PUB_GPIO2_STATUS, "off");
  mqttClient.subscribe(SUB_GPIO2_ACTION);
}

void PubSubCallback(char* topic, byte* payload, unsigned int length) {

  String strTopicGpio2Action = SUB_GPIO2_ACTION;
  String strPayload = "";
  String strON = "on";
  String strOFF = "off";

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
    if (strON == strPayload) {
      mqttClient.publish(PUB_GPIO2_STATUS, "on");
      digitalWrite(GPIO2_LED, LOW);
      delay(500);
      digitalWrite(GPIO2_LED, HIGH);
      Serial.println("gate opening");
      mqttClient.publish(PUB_GPIO2_STATUS, "off");
    } else if (strOFF == strPayload) {
      digitalWrite(GPIO2_LED, HIGH);
    }
  }
}

void setup() {

  Serial.begin(115200);
  // GPIO2 is set OUTPUT
  pinMode(GPIO2_LED, OUTPUT);
  digitalWrite(GPIO2_LED, HIGH);
  initWifiStation();
  initMQTTClient();

}

void loop() {
  mqttClient.loop();
  serial_raw = Serial.readStringUntil('\n');
  if(serial_raw == "reset ap"){
    wifiManager.resetSettings();
  }
}
