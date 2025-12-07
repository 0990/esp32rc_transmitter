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
CRSFRouter crsfRouter;
CRSFParser crsfParser;
TXOTAConnector otaConnector;
TXModuleEndpoint crsfTransmitter;


StubbornReceiver DataDlReceiver;
StubbornSender DataUlSender;
uint8_t CRSFinBuffer[CRSF_MAX_PACKET_LEN+1];

device_affinity_t ui_devices[] = {
  {&Handset_device, 1}
};


void clearOTAQueue()
{
    otaConnector.resetOutputQueue();
}

static void telemetryVbat(uint32_t vbat)
{
    CRSF_MK_FRAME_T(crsf_sensor_battery_t) crsfbatt = { 0 };
    // Values are MSB first (BigEndian)
    crsfbatt.p.voltage = htobe16((uint16_t)vbat);
    crsfRouter.SetHeaderAndCrc((crsf_header_t *)&crsfbatt, CRSF_FRAMETYPE_BATTERY_SENSOR, CRSF_FRAME_SIZE(sizeof(crsf_sensor_battery_t)));
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, &crsfbatt.h);
}

//定期有效的LinkStatis才能触发遥控器显示回传，目前是伪造信息
static void telemetryLinkStatis()
{
    uint8_t linkStatisticsFrame[CRSF_FRAME_NOT_COUNTED_BYTES + CRSF_FRAME_SIZE(sizeof(crsfLinkStatistics_t))];
    crsfRouter.makeLinkStatisticsPacket(linkStatisticsFrame);
    // the linkStats originates from the OTA connector so we don't send it back there.
   // crsfRouter.deliverMessage(&otaConnector, (crsf_header_t *)linkStatisticsFrame);
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, (crsf_header_t *)linkStatisticsFrame);
}

static void checkSendLinkStatsToHandset(uint32_t now)
{
  static uint32_t lastReported_Ms  = 0;
  if((now - lastReported_Ms) > RC_LINKSTATS_PERIOD_MS)
  {
    uint8_t linkStatisticsFrame[CRSF_FRAME_NOT_COUNTED_BYTES + CRSF_FRAME_SIZE(sizeof(crsfLinkStatistics_t))];

    crsfRouter.makeLinkStatisticsPacket(linkStatisticsFrame);
    // the linkStats originates from the OTA connector so we don't send it back there.
   // crsfRouter.deliverMessage(&otaConnector, (crsf_header_t *)linkStatisticsFrame);
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, (crsf_header_t *)linkStatisticsFrame);
    lastReported_Ms = now;
  }
}

// --- 接收回调 ---
void OnEspNowDataRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(TelemetryPacket)) {
    Serial.printf("[WARN] unexpected size: %d bytes\n", len);
    return;
  }

  TelemetryPacket pkt;
  memcpy(&pkt, data, len);

  uint16_t calc = crc16((uint8_t*)&pkt, sizeof(pkt) - sizeof(pkt.crc));

  if (pkt.header != 0xAB) {
    Serial.printf("[ERR]header mismatch:%d,expected:0xAA",pkt.header);
    return;
  }

  if (calc != pkt.crc) {
    Serial.println("[ERR] CRC mismatch");
    return;
  }

  telemetryLinkStatis();
  telemetryVbat(pkt.vbat);
}


void setup() {
    // Register the devices with the framework
  devicesRegister(ui_devices, ARRAY_SIZE(ui_devices));
  // Initialise the devices
  devicesInit();
  DBGLN("Initialised devices");

  crsfTransmitter.begin();
  crsfRouter.addEndpoint(&crsfTransmitter);
  crsfRouter.addConnector(&otaConnector);
  devicesStart();

  DataDlReceiver.SetDataToReceive(CRSFinBuffer, sizeof(CRSFinBuffer));

  EspNow_InitTransmitter(OnEspNowDataRecv);
}


RcPacket g_txPacket;

void loop() {
  const uint32_t now = millis();
  devicesUpdate(now);

  if (DataDlReceiver.HasFinishedData())
  {
      // Send all other tlm to CRSF router
      crsfRouter.processMessage(&otaConnector, (crsf_header_t *)CRSFinBuffer);
      DataDlReceiver.Unlock();
  }

  static uint32_t lastSendMs  = 0;
  // 50Hz 发射
  if (now - lastSendMs >= RC_TX_PERIOD_MS) {
    lastSendMs = now; 
    uint16_t channels[RC_MAX_CHANNELS];
    for (int i =0;i<RC_MAX_CHANNELS;i++){
      channels[i]=CRSF_to_US(uint16_t(ChannelData[i]));
    }
    RcPacket_Fill(g_txPacket, channels, RC_MAX_CHANNELS);
    EspNow_Send(reinterpret_cast<uint8_t*>(&g_txPacket),
                          sizeof(g_txPacket));
  }

  // static uint32_t lastSendTelemetry  = 0;
  // // 50Hz 发射
  // if (now - lastSendTelemetry >= RC_TELEMETRY_PERIOD_MS) {
  //   reportVbat();
  //   lastSendTelemetry = now; 
  // }

  // checkSendLinkStatsToHandset(now);
}

