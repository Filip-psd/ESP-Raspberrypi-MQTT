#include <WiFi.h>
#include <esp_now.h>

#define PHOTOPIN 34

esp_now_peer_info_t responderInfo; //peer info object holding well... info about the peer

uint8_t macAddress[] = {0x8C, 0x94, 0xDF, 0x60, 0xDC, 0x18}; //mac address of the ESP bridge board the info is to be sent to

struct struct_message2 { //struct holding reding
  int light;
};

struct struct_message2 package2; //struct object 

void photoRead () {
  package2.light = digitalRead(PHOTOPIN); //where the magic happens - reading the photopin and inputting the read value into the struct object "package2"
  delay(100);
}

void parsedData (const wifi_tx_info_t *tx_info, esp_now_send_status_t status){ //callback function activated upon sending of data - print package status
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
  esp_now_register_send_cb(parsedData); //registering the callback
  memcpy(responderInfo.peer_addr, macAddress, 6); //populating the responderInfo function's peer_addr with the macAddress, size of 6 bytes 
  responderInfo.channel = 0;
  responderInfo.encrypt = false;

  if(esp_now_add_peer(&responderInfo)!=ESP_OK){ 
    Serial.println("failed to add responder"); 
    return;
  }
}

void loop() {
    photoRead();
    esp_err_t result = esp_now_send(macAddress, (uint8_t *) &package2, sizeof(package2)); //sending data, outputting ESP_OK if goes fine
  if(result==ESP_OK){
    Serial.println("Parsing complete");
  } else{
    Serial.println("Parsing incomplete - error");
    delay(2000);
  }
}
