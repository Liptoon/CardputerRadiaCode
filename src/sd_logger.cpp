#include "sd_logger.h"
#include "config.h"
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>

static bool _sdReady = false;

bool sdReady() { return _sdReady; }

bool sdInit() {
    SPI.begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_CS);
    if (!SD.begin(SD_SPI_CS, SPI, 25000000)) {
        _sdReady = false;
        return false;
    }
    _sdReady = true;

    // Write CSV header
    File f = SD.open("/RC_data.csv", FILE_APPEND);
    if (!f) {
        f = SD.open("/RC_data.csv", FILE_WRITE);
        if (f) {
            f.println("timestamp_ms,dose_rate_uSv_h,count_rate_cps,dose_uSv,temp_C,battery_pct");
        }
    }
    if (f) f.close();
    return true;
}

void sdLogData(const DataState& st, bool enabled) {
    if (!_sdReady || !enabled) return;

    static uint32_t _lastLogMs = 0;
    uint32_t now = millis();
    if (now - _lastLogMs < SD_LOG_INTERVAL_MS) return;
    _lastLogMs = now;

    char line[128];
    snprintf(line, sizeof(line), "%u,%.3f,%.1f,%.3f,%.1f,%.0f",
             now, st.dose_rate, st.count_rate, st.dose, st.temperature, st.battery);

    File f = SD.open("/RC_data.csv", FILE_APPEND);
    if (f) {
        f.println(line);
        f.close();
    }
}
