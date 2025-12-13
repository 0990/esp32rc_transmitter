#include <Arduino.h>
#include "packet.h"
#include "CRSFHandset.h"
#include "CRSFParameters.h"
#include "devHandset.h"
#include "CRSFParser.h"
#include "CRSFRouter.h"
#include "TXOTAConnector.h"
#include "TXModuleEndpoint.h"
#include "stubborn_receiver.h"
#include "stubborn_sender.h"
#include "helpers.h"
#include "espnow_link.h"
#include "common.h"
#include "rc_config.h"
#include "LQCALC.h"

CRSFRouter crsfRouter;
TXModuleEndpoint crsfTransmitter;


static uint32_t LastTLMpacketRecv_Ms = 0;
static uint32_t LinkStatsLastReported_Ms = 0;
LQCALC<100> LqTQly;


device_affinity_t ui_devices[] = {
  {&Handset_device, 1}
};

static void telemetryBattery(packet_battery_t packet)
{
    CRSF_MK_FRAME_T(crsf_sensor_battery_t) crsfbatt = { 0 };
    // Values are MSB first (BigEndian)
    crsfbatt.p.voltage = htobe16(packet.voltage);
    crsfbatt.p.current = htobe16(packet.current);
    crsfbatt.p.capacity = htobe32(packet.capacity);
    crsfbatt.p.remaining = packet.remaining;
    crsfRouter.SetHeaderAndCrc((crsf_header_t *)&crsfbatt, CRSF_FRAMETYPE_BATTERY_SENSOR, CRSF_FRAME_SIZE(sizeof(crsf_sensor_battery_t)));
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, &crsfbatt.h);
}


// --- 接收回调 ---
void OnEspNowDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len < static_cast<int>(sizeof(packet_header_t) + sizeof(uint8_t))) {
    Serial.printf("[WARN] telemetry too small: %d bytes\n", len);
    return;
  }

  const uint8_t crc = getPacketCrc8().calc(data, static_cast<uint16_t>(len - 1), 0);
  if (crc != data[len - 1]){
    Serial.println("[ERR] battery CRC mismatch");
    return;
  }

  const uint8_t header = data[0];
  if ((header & 0xF0) != 0xB0) {
     Serial.println("[ERR] Not a telemetry packet (only accept 0xBx headers");
    return;
  }

  LastTLMpacketRecv_Ms = millis();
  LqTQly.add();

  switch (header) {
  case PACKET_HEAD_LINK_STATISTICS: {
    using LinkStatsFrame =  PACKET_FRAME_T(packet_linkstatistics_t);
    if (len != static_cast<int>(sizeof(LinkStatsFrame))) {
      Serial.printf("[WARN] link stats size mismatch: %d bytes\n", len);
      return;
    }
    const auto *pkt = reinterpret_cast<const LinkStatsFrame *>(data);

    linkStats.uplink_RSSI_1 = pkt->p.uplink_RSSI_1;
    linkStats.uplink_RSSI_2 = pkt->p.uplink_RSSI_2;
    linkStats.uplink_Link_quality = pkt->p.uplink_Link_quality;
    linkStats.uplink_SNR = pkt->p.uplink_SNR;
    break;
  }

  case PACKET_HEAD_BATTERY: {
    using BatteryFrame =  PACKET_FRAME_T(packet_battery_t);
    if (len != static_cast<int>(sizeof(BatteryFrame))) {
      Serial.printf("[WARN] battery size mismatch: %d bytes\n", len);
      return;
    }

    const auto *pkt = reinterpret_cast<const BatteryFrame *>(data);
    telemetryBattery(pkt->p);
    break;
  }

  default:
    // Unknown 0xBx telemetry type
    break;
  }
}

static void checkUpdateConnectStatus(uint32_t now){
  const uint32_t lastTlmMillis = LastTLMpacketRecv_Ms;
  if (lastTlmMillis && ((now - lastTlmMillis) <= CONNECTION_LOST_TIMEOUT_MS))
  {
    if (connectionState != connected)
    {
      setConnectionState(connected);
      DBGLN("got downlink conn");
    }
  }
  // If past RX_LOSS_CNT, or in awaitingModelId state for longer than DisconnectTimeoutMs, go to disconnected
  else if (connectionState == connected)
  {
    setConnectionState(disconnected);
    linkStats.uplink_Link_quality = 0;
    LinkStatsLastReported_Ms = 0; // Notify immediately
  }
}

static void checkSendTx2HandSetLinkStats(uint32_t now)
{
  if (now - LinkStatsLastReported_Ms >= TX_TO_HANDSET_LINKSTATS_PERIOD_MS)
  {
    LinkStatsLastReported_Ms = now;
    uint8_t linkStatisticsFrame[CRSF_FRAME_NOT_COUNTED_BYTES + CRSF_FRAME_SIZE(sizeof(crsfLinkStatistics_t))];
    crsfRouter.makeLinkStatisticsPacket(linkStatisticsFrame);
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, (crsf_header_t *)linkStatisticsFrame);
  }
}

static void checkSendTx2RxChannelData(uint32_t now){
  static uint32_t lastMs  = 0;
  // 50Hz 发射
  if (now - lastMs >= TX_TO_RX_RCDATA_PERIOD_MS) {
    lastMs = now;

    PACKET_FRAME_T(packet_channel_t) packet = { 0 };
    for (int i =0;i<RC_MAX_CHANNELS;i++){
      packet.p.channels[i]=CRSF_to_US(uint16_t(ChannelData[i]));
    }
    setHeaderAndCrc(&packet,PACKET_HEAD_CHANNEL);
    EspNow_Send(reinterpret_cast<uint8_t*>(&packet),
                          sizeof(packet));
  }
}

static void checkLqTQlyInc(uint32_t now){
  static uint32_t lastMs  = 0;
  if (now - lastMs >= RX_TO_TX_LINKSTATS_PERIOD_MS) {
    lastMs = now; 
    linkStats.downlink_Link_quality = LqTQly.getLQ();
    LqTQly.inc();
  }
}


void setup() {
    // Register the devices with the framework
  devicesRegister(ui_devices, ARRAY_SIZE(ui_devices));
  // Initialise the devices
  devicesInit();
  DBGLN("Initialised devices");

  crsfTransmitter.begin();
  crsfRouter.addEndpoint(&crsfTransmitter);
  devicesStart();
  EspNow_InitTransmitter(OnEspNowDataRecv);
}


void loop() {
  const uint32_t now = millis();
  devicesUpdate(now);
  checkUpdateConnectStatus(now);
  checkSendTx2RxChannelData(now);
  checkSendTx2HandSetLinkStats(now);
  checkLqTQlyInc(now);
}
