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
    ble.begin();
}

void loop() {
    ble.update();
    char key = inputGetKey();

    // ── While not connected to RadiaCode → scan / select UI ──
    if (!ble.connected()) {
        static uint32_t lastRedraw = 0;
        uint32_t now = millis();

        if (now - lastRedraw > 200) {
            lastRedraw = now;

            if (ble.isScanning()) {
                drawScanningScreen(now / 200);
            } else {
                drawDeviceList(ble.devices(), ble.selectedIndex(),
                               ble.scanSecondsRemaining());
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
                    // drawStatusMsg handled by displayUpdate after connect
                    break;
            }
        }

        delay(10);
        return;
    }

    // ── Data mode (connected to RadiaCode) ──
    if (ble.pollDataState(data)) {
        geigerUpdate(data.count_rate, data.geiger_enabled);
        sdLogData(data, data.sd_logging);
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
                break;
            case 'l': case 'L':
                data.sd_logging = !data.sd_logging;
                break;
            case 'b': case 'B':
                {
                    static uint8_t lvl[] = {32, 96, 160, 255};
                    static uint8_t idx = 2;
                    idx = (idx + 1) & 3;
                    M5.Display.setBrightness(lvl[idx]);
                }
                break;
            case 'r': case 'R':
                data.dose = 0; data.dose_rate = 0; data.count_rate = 0;
                break;
        }
    }

    displayUpdate(data);
    delay(10);
}
