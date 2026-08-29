#include <WiFi.h>
#include <esp_now.h>

#define PHOTOPIN 34

esp_now_peer_info_t responderInfo;

uint8_t macAddress[] = {0x8C, 0x94, 0xDF, 0x60, 0xDC, 0x18};

struct struct_message2 {
  int light;
};

struct struct_message2 package2;

void photoRead () {
  package2.light = digitalRead(PHOTOPIN);
  delay(100);
}

void parsedData (const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  Serial.print("Package2 status ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "sending successful" : "sending failed");
}

void setup() {
    Serial.begin(115200);
    pinMode(PHOTOPIN, INPUT);
    WiFi.mode(WIFI_STA);

  if(esp_now_init() != ESP_OK){
  Serial.println("failed to connect");

  return;
  }
  esp_now_register_send_cb(parsedData);
  memcpy(responderInfo.peer_addr, macAddress, 6);
  responderInfo.channel = 0;
  responderInfo.encrypt = false;

  if(esp_now_add_peer(&responderInfo)!=ESP_OK){ 
    Serial.println("failed to add responder"); 
    return;
  }
}

void loop() {
    photoRead();
    esp_err_t result = esp_now_send(macAddress, (uint8_t *) &package2, sizeof(package2)); 
  if(result==ESP_OK){
    Serial.println("Parsing complete");
  } else{
    Serial.println("Parsing incomplete - error");
    delay(2000);
  }
}