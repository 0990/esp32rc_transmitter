#pragma once
#include <Arduino.h>

// ====== PPM / RC 通道配置 ======
constexpr uint8_t  CPPM_PIN        = 7;    // 当前 PPM 输入脚
constexpr uint8_t  RC_MAX_CHANNELS = 8;
constexpr uint8_t  ESPNOW_CHANNEL  = 6;    // 使用的 ESP-NOW WiFi channel

// 选择 RC 输入源：PPM/CPPM 或 CRSF 单线半双工
enum RcInputType : uint8_t { RC_INPUT_CPPM = 0, RC_INPUT_CRSF = 1 };
constexpr RcInputType RC_INPUT_TYPE = RC_INPUT_CRSF;

// PPM 脉宽阈值（单位 us）
constexpr uint16_t CPPM_SYNC_MIN_US  = 3000; // 大于这个视为同步间隔
constexpr uint16_t CPPM_PULSE_MIN_US = 900;
constexpr uint16_t CPPM_PULSE_MAX_US = 2100;
constexpr uint16_t CPPM_DEFAULT_US   = 1500;

// CRSF 单线 UART 配置（半双工）
constexpr uint8_t  CRSF_UART_NUM            = 1;       // 使用 UART1
constexpr int      CRSF_PIN                 = 16;      // TX/RX 共用引脚
constexpr uint32_t CRSF_BAUDRATE            = 420000;  // CRSF 默认波特率
constexpr uint32_t CRSF_FAILSAFE_TIMEOUT_MS = 300;     // 超时则回落默认值
constexpr bool     CRSF_ENABLE_TELEMETRY    = true;    // 回传电压等信息
constexpr uint32_t CRSF_TELEMETRY_PERIOD_MS = 200;     // 遥测发送周期

// 若需要回传电压，请把飞控/电池分压接到此 ADC；设为 -1 可关闭
constexpr int   CRSF_VBAT_PIN        = -1;
constexpr float CRSF_VBAT_DIVIDER    = 2.0f;   // 分压比（R上+R下）/ R下
constexpr float CRSF_ADC_REF_V       = 3.3f;   // ADC 参考电压
constexpr int   CRSF_ADC_MAX_COUNTS  = 4095;   // 12bit ADC
constexpr float CRSF_MIN_REPORT_VBAT = 0.1f;   // 防止噪声回传

// 发送频率
constexpr uint32_t RC_TX_PERIOD_MS    = 20;   // 50 Hz
constexpr uint32_t RC_PRINT_PERIOD_MS = 5000; // 5 秒打印一次调试信息

constexpr uint32_t RC_TELEMETRY_PERIOD_MS    = 500;   
constexpr uint32_t RC_LINKSTATS_PERIOD_MS    = 100;

#define TARGET_TX 1
