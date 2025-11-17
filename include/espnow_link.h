#pragma once
#include <Arduino.h>

void EspNow_InitTransmitter();
bool EspNow_Send(const uint8_t *data, size_t len);
