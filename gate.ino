
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid         = "floor_1";
const char* password     = "83005097";
const char* mqttServer   = "192.168.2.56";
const int   mqttPort     = 1883;
const char* mqttUser     = "armmy2530";
const char* mqttPassword = "arm254889";

#define PUB_GPIO2_STATUS "cmnd/gatenode/status"
#define SUB_GPIO2_ACTION "cmnd/gatenode/cmnd"

#define GPIO2_LED 3

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void loop() {
  mqttClient.loop();
}

void initWifiStation() {

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(String("\nConnected to the WiFi network (") + ssid + ")" );
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

void setup() {

  Serial.begin(115200);
 
  // GPIO2 is set OUTPUT
  pinMode(GPIO2_LED, OUTPUT);
  digitalWrite(GPIO2_LED, HIGH);
  initWifiStation();
  initMQTTClient();

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
