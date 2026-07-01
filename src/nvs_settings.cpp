#include "nvs_settings.h"
#include "config.h"
#include <Preferences.h>
#include <SD.h>
#include <cstring>

static const char* TAG = "Settings";

// ── SD file helpers ──

static bool _sdWrite(const SettingsData& d) {
    File f = SD.open(SD_SETTINGS_PATH, FILE_WRITE);
    if (!f) return false;
    f.printf("brightness_idx=%u\n", d.brightness_idx);
    f.printf("geiger_enabled=%u\n", (unsigned)d.geiger_enabled);
    f.printf("sd_logging=%u\n", (unsigned)d.sd_logging);
    f.printf("alarm_enabled=%u\n", (unsigned)d.alarm_enabled);
    f.close();
    return true;
}

static SettingsData _sdRead() {
    SettingsData d;
    File f = SD.open(SD_SETTINGS_PATH, FILE_READ);
    if (!f) return d;

    char line[64];
    while (f.available()) {
        int len = f.readBytesUntil('\n', line, sizeof(line) - 1);
        line[len] = 0;
        // Strip trailing \r
        char* nl = strchr(line, '\r');
        if (nl) *nl = 0;

        char* sep = strchr(line, '=');
        if (!sep) continue;
        *sep = 0;
        const char* key = line;
        const char* val = sep + 1;

        unsigned uv = (unsigned)atoi(val);
        if      (strcmp(key, "brightness_idx") == 0) d.brightness_idx = uv;
        else if (strcmp(key, "geiger_enabled") == 0) d.geiger_enabled = uv;
        else if (strcmp(key, "sd_logging")     == 0) d.sd_logging     = uv;
        else if (strcmp(key, "alarm_enabled")  == 0) d.alarm_enabled  = uv;
    }
    f.close();
    return d;
}

// ── Public API ──

SettingsData settingsLoad() {
    SettingsData d;
    Preferences pref;

    // 1) Try NVS
    if (pref.begin(NVS_NAMESPACE, true)) {
        size_t sz = pref.getBytesLength("data");
        if (sz == sizeof(SettingsData)) {
            pref.getBytes("data", &d, sizeof(d));
            pref.end();
            return d;
        }
        pref.end();
    }

    // 2) Fallback to SD
    if (SD.cardType() != CARD_NONE) {
        d = _sdRead();
        // Sync to NVS for next time
        settingsSave(d);
        return d;
    }

    // 3) Hardcoded defaults
    return d;
}

void settingsSave(const SettingsData& data) {
    Preferences pref;
    if (pref.begin(NVS_NAMESPACE, false)) {
        pref.putBytes("data", &data, sizeof(data));
        pref.end();
    }

    // Best-effort SD write
    if (SD.cardType() != CARD_NONE) {
        _sdWrite(data);
    }
}

void settingsDump() {
    SettingsData d = settingsLoad();
    Serial.printf("Settings: brIdx=%u geiger=%u sdLog=%u\n",
                  d.brightness_idx, (unsigned)d.geiger_enabled, (unsigned)d.sd_logging);
}
