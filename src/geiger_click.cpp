#include "geiger_click.h"
#include "config.h"
#include <M5Cardputer.h>

static uint32_t _nextClickMs = 0;
static int      _volume = GEIGER_DEFAULT_VOLUME;
static bool     _initialized = false;

void geigerInit() {
    _initialized = true;
    _nextClickMs = millis() + 100;
    M5.Speaker.begin();
    M5.Speaker.setVolume(_volume);
}

void geigerUpdate(float cps, bool enabled) {
    if (!_initialized) return;

    uint32_t now = millis();

    if (!enabled || cps < GEIGER_MIN_CPS) {
        return;
    }

    if (now >= _nextClickMs) {
        // Generate tick sound: short click at ~2kHz
        M5.Speaker.tone(2000, 15);

        // Schedule next click based on CPS
        // Average interval = 1000ms / CPS, add randomness
        float avgInterval = 1000.0f / cps;
        // Randomize: 0.5x – 1.5x avg interval
        float interval = avgInterval * (0.5f + ((float)random(100) / 100.0f));
        _nextClickMs = now + (uint32_t)interval;
    }
}
