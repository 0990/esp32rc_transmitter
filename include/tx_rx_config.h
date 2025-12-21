#pragma once
#include <Arduino.h>

// ====== PPM / RC 通道配置 ======
constexpr uint8_t  RC_MAX_CHANNELS = 8;
constexpr uint8_t  ESPNOW_CHANNEL  = 6;    // 使用的 ESP-NOW WiFi channel


const int CONNECTION_LOST_TIMEOUT_MS = 1000;//连接超时
constexpr uint32_t TX_TO_HANDSET_LINKSTATS_PERIOD_MS = 200; 
constexpr uint32_t TX_TO_RX_RCDATA_PERIOD_MS = 20; 
constexpr uint32_t RX_TO_TX_LINKSTATS_PERIOD_MS = 100; 
constexpr uint32_t RX_TO_TX_BATTERY_PERIOD_MS = 1000; 

const uint8_t ESPNOW_BROADCAST_ADDRESS[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

