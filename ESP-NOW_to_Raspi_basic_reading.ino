#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
float distance = 0;

const int led_pin = 23;

const char* ssid = "TP-Link_D36D";
const char* password = "30056282";
const char* mqttServer = "192.168.0.104";

WiFiClient espClient; 
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0; 

void WiFi_setup() {
  WiFi.begin(ssid, password);
  while (WiFi.status() !=WL_CONNECTED){
    delay(500);
    Serial.println(".");
  }
  Serial.println("WiFi connected");
}

void callback(char* topic, byte* message, unsigned int length) { //these three values are handed by the library
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageDistance;
  
for (int i = 0; i < length; i++) {
  Serial.print((char)message[i]); //converting payload to a string
  messageDistance += (char)message[i];
  }

if (String(topic) == "esp32/output") { //this line -  is the text "esp32/output" the topic name
    Serial.print("Changing output to ");
    if(messageDistance == "on"){
      Serial.println("on");
      digitalWrite(led_pin, HIGH);  
    }
    else if(messageDistance == "off"){
      Serial.println("off");
      digitalWrite(led_pin, LOW);
    }
  }
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

void setup() {
  randomSeed(analogRead(0));

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);

  Serial.begin(115200);
  WiFi_setup();
  pinMode(led_pin, OUTPUT);

  while (! Serial) { 
  delay(1);
  }

  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    digitalWrite(led_pin, HIGH);
    delay(200);
    digitalWrite(led_pin, LOW);
  } 
}

void loop() {

  if (!client.connected()) {
  reconnect();
  }

  client.loop(); 
  
  long now = millis(); 
  if (now - lastMsg > 5000) { 
    lastMsg = now;
  
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);
  char distanceString[10]; 
    dtostrf(measure.RangeMilliMeter, 1, 3, distanceString); 
    Serial.print("Distance: ");
    Serial.println(distanceString);
    client.publish("esp32/distance", distanceString); 
  } 
  delay(100); 
}
