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
  VL53L0X_RangingMeasurementData_t measure;
    
  Serial.print("Reading a measurement... ");
  lox.rangingTest(&measure, false); 
  if (measure.RangeStatus != 4) {  
    package.distance = measure.RangeMilliMeter; 
  } 
  delay(100);
}

esp_now_peer_info_t responderInfo; 

void parsedData (const wifi_tx_info_t *tx_info, esp_now_send_status_t status){
  Serial.print("Package status ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "sending successful" : "sending failed");
  Serial.printf("Distance:", package.distance);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Wire.begin();

  if (!lox.begin()) {
    Serial.println("Failed to boot VL53L0X - check wiring/I2C address");
    while (1) { delay(1000); }   // halt rather than crash-loop on bad readings
  }
  Serial.println("VL53L0X init OK");


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
  sensorRead();
  esp_err_t result = esp_now_send(macAddress, (uint8_t *) &package, sizeof(package)); 
  if(result==ESP_OK){
    Serial.println("Parsing complete");
  } else{
    Serial.println("Parsing incomplete - error");
    delay(2000);
  }
}
