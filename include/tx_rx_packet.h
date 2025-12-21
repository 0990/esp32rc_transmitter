#pragma once
#include <Arduino.h>
#include "tx_rx_config.h"

#define PACKED __attribute__((packed))

#define PACKET_HEAD_CHANNEL 0xAA
#define PACKET_HEAD_LINK_STATISTICS 0xB0
#define PACKET_HEAD_BATTERY 0xB1

typedef struct packet_header_s{
  uint8_t header;
}PACKED packet_header_t;

#define PACKET_FRAME_T(payload) struct payload##_frame_s {packet_header_t h; payload p; uint8_t crc; } PACKED

typedef struct packet_channel_s{
  uint16_t channels[RC_MAX_CHANNELS];
}PACKED packet_channel_t;


typedef struct packet_linkstatistics_s{
  uint8_t uplink_RSSI_1;
  uint8_t uplink_RSSI_2;
  uint8_t uplink_Link_quality;
  int8_t uplink_SNR;
}PACKED packet_linkstatistics_t;

typedef struct packet_battery_s{
    uint16_t voltage;  // mv * 100 BigEndian
    uint16_t current;  // ma * 100
    uint32_t capacity; // mah
    uint8_t remaining; // %
}PACKED packet_battery_t;

// --- CRC8 (poly configurable, default matches CRSF: 0xD5) ---
class PacketCrc8
{
public:
  explicit PacketCrc8(uint8_t poly) { init(poly); }

  inline uint8_t calc(uint8_t data) const {
    return crc8tab[data];
  }

  inline uint8_t calc(const uint8_t *data, uint16_t len, uint8_t crc = 0) const {
    while (len--) {
      crc = crc8tab[crc ^ *data++];
    }
    return crc;
  }

private:
  void init(uint8_t poly)
  {
    uint8_t crc;
    for (uint16_t i = 0; i < 256; i++) {
      crc = i;
      for (uint8_t j = 0; j < 8; j++) {
        crc = (crc << 1) ^ ((crc & 0x80) ? poly : 0);
      }
      crc8tab[i] = crc & 0xFF;
    }
  }

  uint8_t crc8tab[256]{};
};

inline PacketCrc8 &getPacketCrc8()
{
  static PacketCrc8 crc8(0xD5);
  return crc8;
}

template <typename FrameT>
inline void setHeaderAndCrc(FrameT *frame, uint8_t header)
{
  constexpr uint16_t frameSize = sizeof(FrameT);
  auto *bytes = reinterpret_cast<uint8_t *>(frame);

  reinterpret_cast<packet_header_t *>(frame)->header = header;
  const uint8_t crc = getPacketCrc8().calc(bytes, frameSize - 1, 0);
  bytes[frameSize - 1] = crc;
}
