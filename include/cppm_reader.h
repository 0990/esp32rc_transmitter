#pragma once
#include <Arduino.h>
#include "rc_config.h"

// 初始化 PPM 输入捕获
void Cppm_Init();

// 获取当前解析出来的通道值（单位 us）
void Cppm_GetChannels(uint16_t *out, size_t maxChannels);

// 打印一些调试信息（直方图等，你可以按需实现）
void Cppm_PrintDebug();
