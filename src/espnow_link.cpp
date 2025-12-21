#include "espnow_link.h"

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "tx_rx_config.h"

namespace {
  void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // 有需要可以打开调试
    // Serial.print("ESP-NOW send status: ");
    // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
    (void)mac_addr;
    (void)status;
  }
}

void EspNow_InitTransmitter(esp_now_recv_cb_t OnDataRecv) {
  WiFi.mode(WIFI_STA);

  // 启用低速远距协议（LR）
  #if ESP_IDF_VERSION_MAJOR >= 4
    esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
  #else
    esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCOL_LR);
  #endif

  // 固定频道
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    ESP.restart();
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, ESPNOW_BROADCAST_ADDRESS, sizeof(ESPNOW_BROADCAST_ADDRESS));
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ESP-NOW add peer failed!");
    ESP.restart();
  }

  Serial.println("ESP-NOW Transmitter Ready.");
}

bool EspNow_Send(const uint8_t *data, size_t len) {
  esp_err_t err = esp_now_send(ESPNOW_BROADCAST_ADDRESS, data, len);
  return err == ESP_OK;
}

