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
#include "rx_tx_common.h"
CRSFRouter crsfRouter;
CRSFParser crsfParser;
TXOTAConnector otaConnector;
TXModuleEndpoint crsfTransmitter;


StubbornReceiver DataDlReceiver;
StubbornSender DataUlSender;
uint8_t CRSFinBuffer[CRSF_MAX_PACKET_LEN+1];


static uint32_t LastTLMpacketRecv_Ms = 0;
static uint32_t LinkStatsLastReported_Ms = 0;
LQCALC<100> LqTQly;
RcPacket g_txPacket;


device_affinity_t ui_devices[] = {
  {&Handset_device, 1}
};

static void telemetryVbat(uint32_t vbat)
{
    CRSF_MK_FRAME_T(crsf_sensor_battery_t) crsfbatt = { 0 };
    // Values are MSB first (BigEndian)
    crsfbatt.p.voltage = htobe16((uint16_t)vbat);
    crsfRouter.SetHeaderAndCrc((crsf_header_t *)&crsfbatt, CRSF_FRAMETYPE_BATTERY_SENSOR, CRSF_FRAME_SIZE(sizeof(crsf_sensor_battery_t)));
    crsfRouter.deliverMessageTo(CRSF_ADDRESS_RADIO_TRANSMITTER, &crsfbatt.h);
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

  LastTLMpacketRecv_Ms = millis();
  LqTQly.add();
  
  telemetryVbat(pkt.vbat);
}

static void UpdateConnectDisconnectStatus(){
  const uint32_t lastTlmMillis = LastTLMpacketRecv_Ms;
  const uint32_t now = millis();
  if (lastTlmMillis && ((now - lastTlmMillis) <= msConnectionLostTimeout))
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
    uint16_t channels[RC_MAX_CHANNELS];
    for (int i =0;i<RC_MAX_CHANNELS;i++){
      channels[i]=CRSF_to_US(uint16_t(ChannelData[i]));
    }
    RcPacket_Fill(g_txPacket, channels, RC_MAX_CHANNELS);
    EspNow_Send(reinterpret_cast<uint8_t*>(&g_txPacket),
                          sizeof(g_txPacket));
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
  crsfRouter.addConnector(&otaConnector);
  devicesStart();

  DataDlReceiver.SetDataToReceive(CRSFinBuffer, sizeof(CRSFinBuffer));

  EspNow_InitTransmitter(OnEspNowDataRecv);
}


void loop() {
  const uint32_t now = millis();
  devicesUpdate(now);
  checkSendTx2RxChannelData(now);
  checkSendTx2HandSetLinkStats(now);
}

