# ESP32 RC Transmitter (ESP-NOW)

本工程实现一个“发射端模块”：从遥控器模块仓读取 RC 通道数据（CRSF/PPM，自动识别），通过 ESP-NOW 发送到接收端；同时接收端回传的链路统计/电池等遥测会被转成 CRSF Telemetry 回送给遥控器。

接收端工程：`https://github.com/0990/esp32rc_receiver`

---

## 功能概览

- 上行（TX -> RX）：按 `TX_TO_RX_RCDATA_PERIOD_MS` 周期发送通道数据（默认 50Hz）。
- 下行（RX -> TX）：接收链路统计/电池等遥测，并通过 CRSF 回传给遥控器。
- ESP-NOW：固定信道 + WiFi LR（远距离协议）。

---

## 配置位置

- ESP-NOW 信道、发送周期、连接超时等：`include/tx_rx_config.h`
- 数据包定义与 CRC8：`include/tx_rx_packet.h`
- 遥控器信号引脚（默认单线半双工，GPIO 8）：`include/target/Unified_ESP32_TX.h`

说明：当 `GPIO_PIN_RCSIGNAL_RX == GPIO_PIN_RCSIGNAL_TX` 时会启用自动识别（CRSF/PPM），逻辑见 `lib/Handset/devHandset.cpp`。

---

## PlatformIO 使用

本项目的环境在 `platformio.ini` 中：

- `Unified_ESP32S3_TX`
- `Unified_ESP32S3XIAO_TX`

VSCode（PlatformIO）：

1. 左下角选择环境（env）
2. Build / Upload / Monitor

命令行：

- 编译：`pio run -e Unified_ESP32S3_TX`
- 上传：`pio run -e Unified_ESP32S3_TX -t upload`
- 串口监视器：`pio device monitor -e Unified_ESP32S3_TX`

串口监视器波特率以 `platformio.ini` 的 `monitor_speed` 为准（当前 Unified 环境默认 `420000`）。

---

## 数据包（ESP-NOW）

定义见 `include/tx_rx_packet.h`：

- `0xAA`：通道数据（`RC_MAX_CHANNELS`，默认 8 路，uint16_t，单位 us）
- `0xB0`：链路统计（RSSI/LQ/SNR）
- `0xB1`：电池信息（电压/电流/容量/剩余）

每个包末尾包含 CRC8（poly `0xD5`）。

---

## 目录结构（关键文件）

- `src/main.cpp`：主循环（通道上行 + 遥测下行 + CRSF 回传）
- `src/espnow_link.cpp`：ESP-NOW 初始化与收发
- `include/tx_rx_config.h`：频率/信道/超时等参数
- `include/target/Unified_ESP32_TX.h`：模块仓信号引脚定义
- `lib/*`：CRSF/Handset 相关实现（从遥控器获取通道、向遥控器回传遥测）

