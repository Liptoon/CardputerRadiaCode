#include <Arduino.h>
#include <M5Cardputer.h>

#include "screen.h"
#include "splash.h"
#include "config.h"
#include "version.h"

#include "radiacode_ble.h"
#include "radiacode_data.h"
#include "display.h"
#include "geiger_click.h"
#include "sd_logger.h"
#include "input_handler.h"
#include "nvs_settings.h"

RadiaCodeBLE ble;
DataState    data;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    screenInit();
    showSplash();
    delay(2000);

    displayInit();
    geigerInit();
    sdInit();
    data.sd_ready = sdReady();

    // Restore persistent settings (NVS → SD → defaults)
    SettingsData s = settingsLoad();
    data.geiger_enabled  = s.geiger_enabled;
    data.sd_logging      = s.sd_logging;
    data.alarm_enabled   = s.alarm_enabled;
    data.brightness_idx  = s.brightness_idx;
    static const uint8_t lvl[] = {32, 96, 160, 255};
    M5.Display.setBrightness(lvl[data.brightness_idx & 3]);

    ble.begin();

    // Boot chime — rising three-note sequence
    delay(100);
    M5.Speaker.tone(523, 120);
    delay(130);
    M5.Speaker.tone(659, 120);
    delay(130);
    M5.Speaker.tone(784, 180);
}

void loop() {
    ble.update();
    char key = inputGetKey();

    // ── While not connected to RadiaCode → scan / select UI ──
    if (!ble.connected()) {
        static uint32_t lastRedraw = 0;
        static bool    wasScanning = true;
        static bool    _connecting = false;
        uint32_t now = millis();

        if (wasScanning && !ble.isScanning()) {
            lastRedraw = 0;
        }
        if (!wasScanning && ble.isScanning()) {
            _connecting = false;
        }
        wasScanning = ble.isScanning();

        if (now - lastRedraw > 200) {
            lastRedraw = now;

            if (ble.isScanning()) {
                drawScanningScreen(now / 200);
            } else {
                drawDeviceList(ble.devices(), ble.selectedIndex(),
                               ble.scanSecondsRemaining(), _connecting);
            }
        }

        // Device selection happens in PH_SELECT
        if (!ble.isScanning() && key && ble.devices().size() > 0) {
            switch (key) {
                case '\t': case ' ': case 'd': case '.':
                    ble.selectNext(); break;
                case '\b': case 'a': case ',':
                    ble.selectPrev(); break;
                case '\n':
                    ble.selectDevice(ble.selectedIndex());
                    _connecting = true;
                    break;
            }
        }

        delay(10);
        return;
    }

    // ── Data mode (connected to RadiaCode) ──
    if (ble.pollDataState(data)) {
        sdLogData(data, data.sd_logging);
        data.sd_ready = sdReady();
    }

    // Geiger click (every loop, not just on BLE poll)
    geigerUpdate(data.count_rate, data.geiger_enabled);

    // Alarm beep — two-tone pattern, ~3s cycle while active
    {
        static uint32_t _alarmCycleStart = 0;
        if (data.alarm_active && data.alarm_enabled) {
            uint32_t cycle = (millis() - _alarmCycleStart) % 3000;
            if (cycle < 400)
                M5.Speaker.tone(880, 400);
            else if (cycle >= 600 && cycle < 1000)
                M5.Speaker.tone(1320, 400);
        } else {
            _alarmCycleStart = millis();
        }
    }

    if (key) {
        switch (key) {
            case '\n':
                {
                    uint8_t v = (uint8_t)data.view + 1;
                    if (v > 2) v = 0;
                    data.view = (ViewMode)v;
                }
                break;
            case '\b':
                if (data.view != ViewMode::Main) data.view = ViewMode::Main;
                break;
            case '\t':
                {
                    uint8_t v = (uint8_t)data.view + 1;
                    if (v > 2) v = 0;
                    data.view = (ViewMode)v;
                }
                break;
            case 'g': case 'G': case ' ':
                data.geiger_enabled = !data.geiger_enabled;
                goto _save;
            case 'l': case 'L':
                data.sd_logging = !data.sd_logging;
                goto _save;
            case 'a': case 'A':
                data.alarm_enabled = !data.alarm_enabled;
                goto _save;
            case 'b': case 'B':
                {
                    static const uint8_t lvl[] = {32, 96, 160, 255};
                    data.brightness_idx = (data.brightness_idx + 1) & 3;
                    M5.Display.setBrightness(lvl[data.brightness_idx]);
                }
                goto _save;
            case 'r': case 'R':
                data.dose = 0; data.dose_rate = 0; data.count_rate = 0;
                break;
        }
    }
    goto _skip_save;
_save:
    {
        SettingsData s;
        s.brightness_idx = data.brightness_idx;
        s.geiger_enabled = data.geiger_enabled;
        s.sd_logging     = data.sd_logging;
        s.alarm_enabled  = data.alarm_enabled;
        settingsSave(s);
    }
_skip_save:

    displayUpdate(data);
    delay(10);
}
