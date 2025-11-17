#pragma once
#include <Arduino.h>

// ====== PPM / RC 通道配置 ======
constexpr uint8_t  CPPM_PIN            = 7;    // 按你现在接法
constexpr uint8_t  RC_MAX_CHANNELS     = 8;
constexpr uint8_t  ESPNOW_CHANNEL     = 6; //使用的espnow通道

// PPM 脉宽阈值（单位 us）
constexpr uint16_t CPPM_SYNC_MIN_US    = 3000; // 大于这个视为同步间隔
constexpr uint16_t CPPM_PULSE_MIN_US   = 900;
constexpr uint16_t CPPM_PULSE_MAX_US   = 2100;
constexpr uint16_t CPPM_DEFAULT_US     = 1500;

// 发送频率
constexpr uint32_t RC_TX_PERIOD_MS     = 20;   // 50 Hz
constexpr uint32_t RC_PRINT_PERIOD_MS  = 5000; // 5 秒打印一次调试信息
