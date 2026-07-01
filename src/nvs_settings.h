#pragma once

#include <cstdint>
#include <cstddef>

struct SettingsData {
    uint8_t brightness_idx = 2;
    bool    geiger_enabled = true;
    bool    sd_logging = false;
};

SettingsData settingsLoad();
void         settingsSave(const SettingsData& data);
void         settingsDump();  // debug
