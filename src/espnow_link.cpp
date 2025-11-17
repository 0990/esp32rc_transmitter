#include "espnow_link.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "rc_config.h"

namespace {
  // 全局只在本编译单元可见
  uint8_t kBroadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

  void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // 有需要可以打开调试
    // Serial.print("ESP-NOW send status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
  }
}

void EspNow_InitTransmitter() {
  WiFi.mode(WIFI_STA);

  // 启用低速远距协议（LR）
  #if ESP_IDF_VERSION_MAJOR >= 4
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  #else
    esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR);
  #endif

  // 固定频道
  esp_wifi_set_channel(6, WIFI_SECOND_CHAN_NONE); // 如果你想跟 rc_config.h 对齐，可以也读常量

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    ESP.restart();
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, kBroadcastAddress, ESPNOW_CHANNEL);
  peerInfo.channel = 6;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ESP-NOW add peer failed!");
    ESP.restart();
  }

  Serial.println("ESP-NOW Transmitter Ready.");
}

bool EspNow_Send(const uint8_t *data, size_t len) {
  esp_err_t err = esp_now_send(kBroadcastAddress, data, len);
  return err == ESP_OK;
}
