#pragma once
#include <Arduino.h>
#include <esp_now.h> 

void EspNow_InitTransmitter(esp_now_recv_cb_t OnDataRecv);
bool EspNow_Send(const uint8_t *data, size_t len);
