#pragma once
#include <Arduino.h>
#include "rc_config.h"

#pragma pack(push, 1)
struct RcPacket{
  uint8_t header=0xAA;
  uint16_t channels[RC_MAX_CHANNELS];
  uint16_t crc;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TelemetryPacket{
  uint8_t header=0xAB;
  uint32_t vbat;
  uint16_t crc;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CommonHeader{
  uint8_t header;
};
#pragma pack(pop)

// --- CRC16 (Modbus) ---
inline uint16_t crc16(const uint8_t *data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}

// 计算整个 RcPacket 的 CRC（不包含自身 crc 字段）
inline uint16_t RcPacket_CalcCrc(const RcPacket &pkt) {
  const uint8_t *ptr = reinterpret_cast<const uint8_t*>(&pkt);
  // sizeof(RcPacket) - sizeof(pkt.crc) -> 只算到 crc 前
  return crc16(ptr, sizeof(RcPacket) - sizeof(pkt.crc));
}

// 用当前通道值填充数据包并刷新 CRC
inline void RcPacket_Fill(RcPacket &pkt, const uint16_t *channels, size_t numChannels) {
  pkt.header = 0xAA;
  for (size_t i = 0; i < RC_MAX_CHANNELS; ++i) {
    pkt.channels[i] = (i < numChannels) ? channels[i] : CPPM_DEFAULT_US;
  }
  pkt.crc = RcPacket_CalcCrc(pkt);
}
