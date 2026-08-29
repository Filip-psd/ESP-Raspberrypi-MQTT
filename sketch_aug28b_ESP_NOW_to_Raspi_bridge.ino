#include <esp_now.h>
#include <cstring>
#include <WiFi.h>
#include <PubSubClient.h>

uint8_t MAC_SENSOR_BOARD[] = {0x20, 0x9B, 0xA9, 0x61, 0xE4, 0x10};
uint8_t MAC_PHOTO_BOARD[]  = {0x8C, 0x94, 0xDF, 0x4D, 0x77, 0xF0}; 

const char* ssid = "TP-Link_D36D";
const char* password = "30056282";
const char* mqttServer = "192.168.0.104";

WiFiClient espClient; 
PubSubClient client(espClient);
long lastMsg = 0;

struct struct_message {
  float distance;
};

struct struct_message2 {
  int light;
};

struct_message dataFromSensor1;
struct_message2 dataFromSensor2;
bool sensor1Received = false;
bool sensor2Received = false;

void WiFi_setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() !=WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }
  Serial.println("WiFi connected");
  Serial.print("Channel: ");
  Serial.println(WiFi.channel()); 
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/output"); 
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* message, unsigned int length) { 
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageDistance;
  
for (int i = 0; i < length; i++) {
  Serial.print((char)message[i]); 
  messageDistance += (char)message[i];
  }
}

void OnDataParsed (const esp_now_recv_info_t *recv_info, const uint8_t *data, int len){
  if (std::memcmp(recv_info->src_addr, MAC_SENSOR_BOARD, 6) == 0 && len == sizeof(dataFromSensor1)) {
    std::memcpy(&dataFromSensor1, data, sizeof(dataFromSensor1));
    sensor1Received = true;
    Serial.println("vl53I0x data received!");
  } else if (std::memcmp(recv_info->src_addr, MAC_PHOTO_BOARD, 6) == 0 && len == sizeof(dataFromSensor2)) {
    std::memcpy(&dataFromSensor2, data, sizeof(dataFromSensor2));
    sensor2Received = true;
    Serial.println("Photoresistor data received!");
  } else {
    Serial.println("Data received from an unrecognized sender - ignored.");
    return;
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  WiFi_setup();

  while (! Serial) { 
  delay(1);
  }
  WiFi.mode(WIFI_STA);
  if(esp_now_init()!=ESP_OK){
  Serial.println("Initializing failed - error");
  return; 
  }
  esp_now_register_recv_cb(OnDataParsed);
}

void loop() {

   if (!client.connected()) {
  reconnect();
  }

  client.loop(); 
  
  long now = millis(); 
  if (now - lastMsg > 1000) { 
    lastMsg = now;

    char distanceString[10];
    char lightString[10];
    dtostrf(dataFromSensor1.distance, 1, 3, distanceString);
    itoa (dataFromSensor2.light, lightString, 10);
    Serial.print("Distance: ");
    Serial.println(distanceString);
    client.publish("esp32/distance", distanceString); 
    Serial.print("Luminosity: ");
    Serial.println(lightString);
    client.publish("esp32/photosensor", lightString); 
  }
}