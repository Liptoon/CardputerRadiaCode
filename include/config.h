#pragma once

#include <cstdint>

// ── BLE ──
#define RADIACODE_SERVICE_UUID  "e63215e5-7003-49d8-96b0-b024798fb901"
#define RADIACODE_WRITE_UUID    "e63215e6-7003-49d8-96b0-b024798fb901"
#define RADIACODE_NOTIFY_UUID   "e63215e7-7003-49d8-96b0-b024798fb901"

// Leave empty for auto-discovery by service UUID
#define RADIACODE_MAC ""

// ── Timing ──
#define POLL_INTERVAL_MS        1000
#define BLE_TIMEOUT_MS          3000
#define RECONNECT_INTERVAL_MS   5000
#define BLE_CHUNK_SIZE          18

// ── SD card (M5Cardputer pins) ──
#define SD_SPI_SCK  40
#define SD_SPI_MISO 39
#define SD_SPI_MOSI 14
#define SD_SPI_CS   12
#define SD_LOG_INTERVAL_MS      10000

// ── Geiger click ──
#define GEIGER_MIN_CPS          0.5f
#define GEIGER_DEFAULT_VOLUME   80

// ── Display ──
#define SCREEN_W                240
#define SCREEN_H                135
#define STATUS_BAR_H            18

// ── Alarm thresholds (μSv/h) ──
#define ALARM_DR_L1             1.0f
#define ALARM_DR_L2             10.0f
