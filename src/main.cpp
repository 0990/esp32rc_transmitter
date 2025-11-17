#include <Arduino.h>

#include "rc_config.h"
#include "packet.h"
#include "cppm_reader.h"
#include "espnow_link.h"

RcPacket g_txPacket;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("RC Transmitter starting...");

  Cppm_Init();
  EspNow_InitTransmitter();
}

void loop() {
  const uint32_t now = millis();
  static uint32_t lastSendMs  = 0;
  static uint32_t lastPrintMs = 0;

  // 50Hz 发送
  if (now - lastSendMs >= RC_TX_PERIOD_MS) {
    uint16_t channels[RC_MAX_CHANNELS];
    Cppm_GetChannels(channels, RC_MAX_CHANNELS);

    RcPacket_Fill(g_txPacket, channels, RC_MAX_CHANNELS);
    bool ok = EspNow_Send(reinterpret_cast<uint8_t*>(&g_txPacket),
                          sizeof(g_txPacket));

    // 可选：简单调试
    // if (!ok) Serial.println("ESP-NOW send failed");

    lastSendMs = now;
  }

  // 定期打印调试信息
  if (now - lastPrintMs >= RC_PRINT_PERIOD_MS) {
    Cppm_PrintDebug();
    lastPrintMs = now;
  }
}