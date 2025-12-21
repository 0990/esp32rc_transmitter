#include "common.h"

connectionState_e connectionState = disconnected;
// Current state of channels, CRSF format
uint32_t ChannelData[CRSF_NUM_CHANNELS];