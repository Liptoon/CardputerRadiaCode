#pragma once

#include "radiacode_protocol.h"

enum class ViewMode : uint8_t {
    Main,
    Spectrum,
    Menu,
};

enum class BLEStatus : uint8_t {
    Disconnected,
    Scanning,
    Connecting,
    Connected,
    Error,
};

struct DataState {
    // real-time
    float    dose_rate = 0;       // μSv/h
    float    count_rate = 0;      // CPS
    float    dose = 0;            // μSv accumulated
    float    temperature = 0;     // °C
    float    battery = 0;         // 0–100 %
    uint32_t battery_mv = 0;

    // spectrum
    SpectrumData spectrum;
    bool         spectrum_valid = false;

    // status
    BLEStatus ble_status = BLEStatus::Disconnected;
    ViewMode  view = ViewMode::Main;
    bool      geiger_enabled = true;
    bool      sd_logging = false;
    bool      alarm_active = false;
    bool      sd_ready = false;
    uint8_t   brightness_idx = 2;

    // timestamps
    uint32_t last_data_ms = 0;
    uint32_t last_log_ms = 0;
};
