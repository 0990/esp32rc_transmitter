#include "cppm_reader.h"

namespace {
  volatile uint16_t g_channels[RC_MAX_CHANNELS];
  volatile uint8_t  g_channelIndex = 0;
  volatile uint32_t g_lastMicros   = 0;

  // 可选：统计脉宽区间用于调试
  constexpr int HIST_BUCKETS = 10;
  volatile uint32_t g_hist[HIST_BUCKETS] = {0};

  int bucketForPulse(uint16_t us) {
    // 简单按 [900,2100] 均分
    if (us < CPPM_PULSE_MIN_US || us > CPPM_PULSE_MAX_US) return -1;
    uint16_t span = CPPM_PULSE_MAX_US - CPPM_PULSE_MIN_US;
    uint16_t rel  = us - CPPM_PULSE_MIN_US;
    int idx = rel * HIST_BUCKETS / (span + 1);
    if (idx < 0) idx = 0;
    if (idx >= HIST_BUCKETS) idx = HIST_BUCKETS - 1;
    return idx;
  }

  void IRAM_ATTR onCppmChange() {
    uint32_t now = micros();
    uint32_t delta = now - g_lastMicros;
    g_lastMicros = now;

    if (delta > CPPM_SYNC_MIN_US) {
      // 同步间隔：重新开始
      g_channelIndex = 0;
      return;
    }

    if (delta < CPPM_PULSE_MIN_US || delta > CPPM_PULSE_MAX_US) {
      return;
    }

    if (g_channelIndex < RC_MAX_CHANNELS) {
      g_channels[g_channelIndex] = (uint16_t)delta;
      int b = bucketForPulse(delta);
      if (b >= 0) {
        g_hist[b]++;
      }
      g_channelIndex++;
    }
  }
}

void Cppm_Init() {
  pinMode(CPPM_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(CPPM_PIN), onCppmChange, RISING);

  // 默认值
  for (int i = 0; i < RC_MAX_CHANNELS; ++i) {
    g_channels[i] = CPPM_DEFAULT_US;
  }
}

void Cppm_GetChannels(uint16_t *out, size_t maxChannels) {
  noInterrupts();
  for (size_t i = 0; i < maxChannels && i < RC_MAX_CHANNELS; ++i) {
    out[i] = g_channels[i];
  }
  interrupts();
}

void Cppm_PrintDebug() {
  Serial.println(F("PPM channels (us):"));
  uint16_t tmp[RC_MAX_CHANNELS];
  Cppm_GetChannels(tmp, RC_MAX_CHANNELS);
  for (int i = 0; i < RC_MAX_CHANNELS; ++i) {
    Serial.print("CH");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(tmp[i]);
    Serial.print("  ");
  }
  Serial.println();

  Serial.println(F("Pulse histogram:"));
  for (int i = 0; i < HIST_BUCKETS; ++i) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(g_hist[i]);
  }
  Serial.println();
}
