#include "display.h"
#include "config.h"
#include <M5Cardputer.h>

// ── Lazy canvas (created on first use, avoids breaking M5.Display) ──

static M5Canvas* _canvas = nullptr;

static M5Canvas* getCanvas() {
    if (!_canvas) {
        _canvas = new M5Canvas(&M5.Display);
        _canvas->createSprite(SCREEN_W, SCREEN_H);
        _canvas->setTextWrap(false);
    }
    return _canvas;
}

static constexpr auto BG     = TFT_BLACK;
static constexpr auto FG     = TFT_WHITE;
static constexpr auto ACCENT = TFT_GREEN;
static constexpr auto WARN   = TFT_YELLOW;
static constexpr auto ALARM  = TFT_RED;
static constexpr auto DIM    = 0x4208;
static constexpr auto SEL_BG = 0x3186;
static constexpr int SBH = STATUS_BAR_H;

static int _cardputerBattPct = -1;
static uint32_t _lastBattRead = 0;

static int readCardputerBattery() {
    uint32_t now = millis();
    if (now - _lastBattRead < 5000) return _cardputerBattPct;
    _lastBattRead = now;
    int level = M5.Power.getBatteryLevel();
    if (level >= 0) _cardputerBattPct = level;
    return _cardputerBattPct;
}

void displayInit() {
    M5.Display.setRotation(1);
    M5.Display.fillScreen(BG);
    M5.Display.setTextWrap(false);
}

void displayUpdate(const DataState& st) {
    auto* c = getCanvas();
    c->fillSprite(BG);
    switch (st.view) {
        case ViewMode::Main:     drawMainView(st);     break;
        case ViewMode::Spectrum: drawSpectrumView(st); break;
        case ViewMode::Menu:     drawMenuView(st);     break;
    }
    c->pushSprite(0, 0);
}

void drawStatusBar(const DataState& st) {
    auto* c = getCanvas();
    c->fillRect(0, 0, SCREEN_W, SBH, 0x2104);
    c->setTextSize(1);

    c->setCursor(2, 3);
    c->setTextColor(ACCENT);
    c->print("RC");

    c->setCursor(44, 3);
    bool bleOn = st.ble_status == BLEStatus::Connected;
    c->setTextColor(bleOn ? ACCENT : DIM);
    c->print(bleOn ? "BLE" : "---");

    int cb = readCardputerBattery();
    if (cb >= 0) {
        c->setTextColor(DIM);
        c->setCursor(162, 4);
        c->print("C");
        drawBatteryIcon(170, 3, cb);
    }
    if (st.battery > 0) {
        c->setTextColor(DIM);
        c->setCursor(194, 4);
        c->print("R");
        drawBatteryIcon(202, 3, st.battery);
    }

    if (st.alarm_active)
        c->fillCircle(SCREEN_W / 2, SBH / 2, 3, ALARM);

    const char* lbl = "";
    switch (st.view) {
        case ViewMode::Main:     lbl = "DOS"; break;
        case ViewMode::Spectrum: lbl = "SPC"; break;
        case ViewMode::Menu:     lbl = "MENU"; break;
    }
    c->setTextColor(DIM);
    c->setCursor(72, 3);
    c->print(lbl);

    if (st.geiger_enabled) {
        c->setTextColor(WARN);
        c->setCursor(110, 3);
        c->print("G");
    }
    if (st.sd_logging) {
        c->setTextColor(st.sd_ready ? ACCENT : ALARM);
        c->setCursor(122, 3);
        c->print(st.sd_ready ? "SD" : "ERR");
    }
}

void drawNavBar(const DataState& st) {
    auto* c = getCanvas();
    (void)st;
    const char* text = "[Enter] next  [G] geiger  [A] alarm  [L] log";
    c->setTextSize(1);
    int tw = c->textWidth(text);
    c->setTextColor(DIM, BG);
    c->setCursor((SCREEN_W - tw) / 2, SCREEN_H - 10);
    c->print(text);
}

void drawBatteryIcon(int x, int y, float pct) {
    // Works with both M5.Display and M5Canvas
    auto* c = getCanvas();
    int w = 18, h = 10;
    c->drawRect(x, y, w, h, FG);
    c->fillRect(x + w, y + 2, 2, h - 4, FG);
    int fw = (w - 2) * constrain(pct, 0.0f, 100.0f) / 100.0f;
    uint16_t col = (pct > 20.0f) ? ACCENT : ALARM;
    c->fillRect(x + 1, y + 1, fw, h - 2, col);
}

void drawMainView(const DataState& st) {
    drawStatusBar(st);
    auto* c = getCanvas();

    char buf[16];
    if      (st.dose_rate < 10.0f)  snprintf(buf, sizeof(buf), "%.2f", st.dose_rate);
    else if (st.dose_rate < 100.0f) snprintf(buf, sizeof(buf), "%.1f", st.dose_rate);
    else                             snprintf(buf, sizeof(buf), "%.0f", st.dose_rate);

    c->setTextSize(4);
    int vw = strlen(buf) * 12 * 4;
    int vx = (SCREEN_W - vw) / 2;
    if (vx < 0) vx = 0;
    c->setTextColor(ACCENT, BG);
    c->setCursor(vx, SBH + 6);
    c->print(buf);

    c->setTextSize(1);
    c->setTextColor(FG, BG);
    int ux = vx + vw - (5 * 6);
    if (ux < 0) ux = vx;
    c->setCursor(ux, SBH + 50);
    c->print("uSv/h");

    int r2 = SBH + 66;
    c->setTextSize(2);
    c->setTextColor(FG, BG);
    c->setCursor(4, r2);
    snprintf(buf, sizeof(buf), "%.1f CPS", st.count_rate);
    c->print(buf);

    c->setCursor(4, r2 + 20);
    snprintf(buf, sizeof(buf), "%.2f uSv", st.dose);
    c->print(buf);

    c->setTextColor(DIM);
    c->setCursor(SCREEN_W - 80, r2);
    c->printf("%.0f", st.temperature);
    c->drawCircle(SCREEN_W - 80 + c->getCursorX(), r2 + 2, 1, DIM);
    c->print("C");

    c->setCursor(SCREEN_W - 80, r2 + 20);
    c->printf("%.0f%%", st.battery);

    drawNavBar(st);
}

void drawSpectrumView(const DataState& st) {
    drawStatusBar(st);
    auto* c = getCanvas();

    if (!st.spectrum_valid) {
        c->setTextColor(WARN, BG);
        c->setCursor(10, SCREEN_H / 2);
        c->print("No spectrum data");
        drawNavBar(st);
        return;
    }
    const auto& sp = st.spectrum;
    int gx = 4, gy = SBH + 4;
    int gw = SCREEN_W - 8, gh = SCREEN_H - gy - 14;
    uint32_t maxV = 1;
    for (int i = 0; i < sp.valid_channels; i++)
        if (sp.counts[i] > maxV) maxV = sp.counts[i];
    int bars = gw;
    int cpb = (sp.valid_channels + bars - 1) / bars;
    if (cpb < 1) cpb = 1;
    for (int b = 0; b < bars && b * cpb < sp.valid_channels; b++) {
        uint32_t sum = 0; int cnt = 0;
        for (int c = 0; c < cpb; c++) {
            int idx = b * cpb + c;
            if (idx < sp.valid_channels) { sum += sp.counts[idx]; cnt++; }
        }
        uint32_t avg = cnt ? sum / cnt : 0;
        int bh = (avg * gh) / maxV;
        if (bh < 1 && avg) bh = 1;
        uint16_t col = TFT_GREEN;
        if (b > bars * 3 / 4) col = TFT_RED;
        else if (b > bars / 2) col = TFT_YELLOW;
        c->drawFastVLine(gx + b, gy + gh - bh, bh, col);
    }
    char info[64];
    snprintf(info, sizeof(info), "Live:%us  Ch:%u", sp.live_time, sp.valid_channels);
    c->setTextSize(1);
    c->setTextColor(DIM);
    int iw = c->textWidth(info);
    c->setCursor((SCREEN_W - iw) / 2, SCREEN_H - 20);
    c->print(info);

    drawNavBar(st);
}

void drawMenuView(const DataState& st) {
    drawStatusBar(st);
    auto* c = getCanvas();

    c->setTextSize(1);
    c->setCursor(4, SBH + 8);
    c->setTextColor(FG, BG);
    c->print("= Menu =");
    int y = SBH + 24;
    c->setTextColor(st.geiger_enabled ? ACCENT : DIM);
    c->setCursor(4, y); y += 12;
    c->printf("G: Geiger click  [%s]", st.geiger_enabled ? "ON" : "OFF");
    c->setTextColor(st.sd_logging ? (st.sd_ready ? ACCENT : ALARM) : DIM);
    c->setCursor(4, y); y += 12;
    const char* sdSt = st.sd_logging ? (st.sd_ready ? "ON" : "ERR") : "OFF";
    c->printf("L: SD logging    [%s]", sdSt);
    c->setTextColor(st.alarm_enabled ? ACCENT : DIM);
    c->setCursor(4, y); y += 12;
    c->printf("A: Alarm sound   [%s]", st.alarm_enabled ? "ON" : "OFF");
    c->setTextColor(FG);
    c->setCursor(4, y); y += 12;
    c->print("B: Brightness");
    c->setCursor(4, y); y += 12;
    c->print("R: Reset dose");
    drawNavBar(st);
}

// ── Scanning animation (canvas-based, no flicker) ──

static const char* _spinnerFrames = "|/-\\";

void drawScanningScreen(int animFrame) {
    M5.Display.setBrightness(255);
    auto* c = getCanvas();
    c->fillSprite(BG);
    c->setTextSize(1);
    c->setTextColor(ACCENT);
    c->setCursor(4, 4);
    c->print("Scanning for RadiaCode...");

    int cx = SCREEN_W / 2, cy = SCREEN_H / 2;
    c->setTextSize(4);
    c->setTextColor(ACCENT);
    c->setCursor(cx - 12, cy - 16);
    c->print(_spinnerFrames[animFrame & 3]);

    c->setTextSize(1);
    c->setTextColor(DIM);
    c->setCursor(4, SCREEN_H - 10);
    c->print("Looking for devices...");

    c->pushSprite(0, 0);
}

// ── Device selection list (uses canvas, no flicker) ──

void drawDeviceList(const std::vector<BleDevice>& devices, int sel, int scanSecs) {
    auto* c = getCanvas();
    c->fillSprite(BG);
    c->setTextSize(1);
    c->setTextColor(ACCENT);
    c->setCursor(4, 4);
    c->print("Select RadiaCode device:");
    c->drawFastHLine(0, 16, SCREEN_W, DIM);

    if (devices.empty()) {
        c->setTextColor(WARN);
        c->setCursor(4, 28);
        c->printf("Scan %ds left", scanSecs);
        c->setTextColor(DIM);
        c->setCursor(4, 50);
        c->print("Make sure RadiaCode is");
        c->setCursor(4, 62);
        c->print("powered on and in range.");
        c->pushSprite(0, 0);
        return;
    }

    if (scanSecs > 0) {
        c->setTextColor(DIM);
        c->setCursor(SCREEN_W - 28, 4);
        c->printf("%ds", scanSecs);
    }

    int y = 22, ih = 18, maxV = (SCREEN_H - y - 10) / ih;
    int sc = (sel >= maxV) ? sel - maxV + 1 : 0;
    for (int i = 0; i < (int)devices.size(); i++) {
        if (i < sc) continue;
        if (i - sc >= maxV) break;
        int py = y + (i - sc) * ih;
        if (i == sel) c->fillRect(0, py, SCREEN_W, ih, SEL_BG);
        c->setTextColor(i == sel ? TFT_BLACK : FG, i == sel ? SEL_BG : BG);
        c->setCursor(4, py + 1);
        c->print(devices[i].name.c_str());
        c->setTextColor(i == sel ? 0x4208 : DIM, i == sel ? SEL_BG : BG);
        c->setCursor(SCREEN_W - 40, py + 1);
        c->printf("%d dBm", devices[i].rssi);
    }
    c->setTextColor(DIM);
    c->setCursor(4, SCREEN_H - 10);
    if (!devices.empty()) c->print("[Enter] connect  [Tab/Spc] next");
    c->pushSprite(0, 0);
}

// ── Status message (direct M5.Display) ──

void drawStatusMsg(const char* msg) {
    M5.Display.fillScreen(BG);
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(FG, BG);
    M5.Display.setCursor(4, SCREEN_H / 2 - 4);
    M5.Display.print(msg);
}
