#include <Wire.h>
#include <SPI.h>
#include "Adafruit_VL53L0X.h"
#include <esp_now.h>
#include <WiFi.h>

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

uint8_t macAddress[] = {0x8C, 0x94, 0xDF, 0x60, 0xDC, 0x18};

struct struct_message {
  float distance;
};

struct struct_message package;

void sensorRead (){
  VL53L0X_RangingMeasurementData_t measure; //library-specific object
    
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); 
  if (measure.RangeStatus != 4) {  // RangeStatus == 4 indicates a phase failure - library specific
    package.distance = measure.RangeMilliMeter; //assigning the value to the distance float in the package
  } 
  delay(100);
}

esp_now_peer_info_t responderInfo; 

void parsedData (const wifi_tx_info_t *tx_info, esp_now_send_status_t status){ //callback function
  Serial.print("Package status ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "sending successful" : "sending failed");
  Serial.printf("Distance:", package.distance);
}

int getWiFiChannel(const char *ssid) {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (String(ssid) == WiFi.SSID(i)) {
      return WiFi.channel(i);
    }
  }
  return 1;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Wire.begin();

  Serial.print("Sender channel: ");
  Serial.println(WiFi.channel());
  int ch = getWiFiChannel("TP-Link_D36D"); //change current for network name
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  Serial.printf("Locked to channel %d\n", ch);

  if (!lox.begin()) {
    Serial.println("Failed to boot VL53L0X - check wiring/I2C address");
    while (1) { delay(1000); }   // halt rather than crash-loop on bad readings
  }
  Serial.println("VL53L0X init OK");


 if(esp_now_init() != ESP_OK){
    Serial.println("failed to connect");
    return;
  }
  esp_now_register_send_cb(parsedData); //registering callback
  memcpy(responderInfo.peer_addr, macAddress, 6); //assigning mac address
  responderInfo.channel = 0;
  responderInfo.encrypt = false;

  if(esp_now_add_peer(&responderInfo)!=ESP_OK){ 
    Serial.println("failed to add responder"); 
    return;
  }
}


void loop() {
  sensorRead();
  esp_err_t result = esp_now_send(macAddress, (uint8_t *) &package, sizeof(package)); 
  if(result==ESP_OK){
    Serial.println("Parsing complete");
  } else{
    Serial.println("Parsing incomplete - error");
    delay(2000);
  }
}
