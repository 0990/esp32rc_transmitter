#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> 

#define ESPNOW_CHANNEL 6
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setUpEspNow(){
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    ESP.restart();
  }
  Serial.println("ESP-NOW Receiver Ready.");
}

void setupEspNowTransmitter() {
  WiFi.mode(WIFI_STA);
    // 启用低速远距协议
  #if ESP_IDF_VERSION_MAJOR >= 4
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  #else
    esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR);
  #endif

  // 固定频道
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_now_init();

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}


