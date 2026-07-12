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
static constexpr int BORDER = 2;
static constexpr int MX = BORDER + 2;             // content margin x
static constexpr int MY = BORDER + 2;             // content margin y
static constexpr int MW = SCREEN_W - 2 * MX;      // content width
static constexpr int MH = SCREEN_H - 2 * MY;      // content height
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
    c->drawRect(BORDER, BORDER, SCREEN_W-2*BORDER, SCREEN_H-2*BORDER, DIM);
    switch (st.view) {
        case ViewMode::Main:     drawMainView(st);     break;
        case ViewMode::Spectrum: drawSpectrumView(st); break;
        case ViewMode::Menu:     drawMenuView(st);     break;
    }
    c->pushSprite(0, 0);
}

void drawStatusBar(const DataState& st) {
    auto* c = getCanvas();
    c->fillRect(MX, MY, MW, SBH, 0x2104);
    c->setTextSize(1);

    c->setCursor(MX + 2, MY + 3);
    c->setTextColor(ACCENT);
    c->print("RC");

    // Time from RadiaCode device
    if (st.device_time > 0) {
        int h = (st.device_time % 86400) / 3600;
        int m = (st.device_time % 3600) / 60;
        char tb[8];
        snprintf(tb, sizeof(tb), "%02d:%02d", h, m);
        c->setTextColor(DIM);
        c->setCursor(MX + 20, MY + 3);
        c->print(tb);
    }

    c->setCursor(MX + 56, MY + 3);
    bool bleOn = st.ble_status == BLEStatus::Connected;
    c->setTextColor(bleOn ? ACCENT : DIM);
    c->print(bleOn ? "BLE" : "---");

    const char* lbl = "";
    switch (st.view) {
        case ViewMode::Main:     lbl = "DOS"; break;
        case ViewMode::Spectrum: lbl = "SPC"; break;
        case ViewMode::Menu:     lbl = "MENU"; break;
    }
    c->setTextColor(DIM);
    c->setCursor(MX + 84, MY + 3);
    c->print(lbl);

    if (st.geiger_enabled) {
        c->setTextColor(WARN);
        c->setCursor(MX + 108, MY + 3);
        c->print("G");
    }
    if (st.sd_logging) {
        c->setTextColor(st.sd_ready ? ACCENT : ALARM);
        c->setCursor(MX + 120, MY + 3);
        c->print(st.sd_ready ? "SD" : "ERR");
    }
    if (st.alarm_enabled) {
        c->setTextColor(ALARM);
        c->setCursor(MX + 136, MY + 3);
        c->print("A");
    }

    if (st.alarm_active)
        c->fillCircle(MX + MW / 2, MY + SBH / 2, 3, ALARM);

    int cb = readCardputerBattery();
    if (cb >= 0) {
        c->setTextColor(DIM);
        c->setCursor(MX + MW - 76, MY + 4);
        c->print("C");
        drawBatteryIcon(MX + MW - 66, MY + 3, cb);
    }
    if (st.battery > 0) {
        c->setTextColor(DIM);
        c->setCursor(MX + MW - 38, MY + 4);
        c->print("R");
        drawBatteryIcon(MX + MW - 28, MY + 3, st.battery);
    }
}

void drawNavBar(const DataState& st) {
    auto* c = getCanvas();
    (void)st;
    const char* text = "[Enter]next [G]geiger [A]alarm [L]log";
    c->setTextSize(1);
    int tw = c->textWidth(text);
    c->setTextColor(DIM, BG);
    c->setCursor(MX + (MW - tw) / 2, MY + MH - 10);
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
    int vx = MX + (MW - vw) / 2;
    if (vx < MX) vx = MX;
    c->setTextColor(ACCENT, BG);
    c->setCursor(vx, MY + SBH + 6);
    c->print(buf);

    c->setTextSize(1);
    c->setTextColor(FG, BG);
    int ux = vx + vw - (5 * 6);
    if (ux < 0) ux = vx;
    c->setCursor(ux, MY + SBH + 50);
    c->print("uSv/h");

    int r2 = MY + SBH + 62;
    c->setTextSize(2);
    c->setTextColor(FG, BG);
    c->setCursor(MX + 4, r2);
    snprintf(buf, sizeof(buf), "%.1f CPS", st.count_rate);
    c->print(buf);

    c->setCursor(MX + 4, r2 + 20);
    snprintf(buf, sizeof(buf), "%.2f uSv", st.dose);
    c->print(buf);

    c->setTextColor(DIM);
    c->setCursor(MX + MW - 80, r2);
    c->printf("%.0f", st.temperature);
    c->drawCircle(MX + MW - 80 + c->getCursorX(), r2 + 2, 1, DIM);
    c->print("C");

    c->setCursor(MX + MW - 80, r2 + 16);
    c->printf("%.0f%%", st.battery);

    drawNavBar(st);
}

void drawSpectrumView(const DataState& st) {
    drawStatusBar(st);
    auto* c = getCanvas();

    if (!st.spectrum_valid) {
        c->setTextColor(WARN, BG);
        c->setCursor(MX + 10, MY + MH / 2);
        c->print("No spectrum data");
        drawNavBar(st);
        return;
    }
    const auto& sp = st.spectrum;
    int gx = MX + 4, gy = MY + SBH + 4;
    int gw = MW - 8, gh = MY + MH - gy - 22;
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
    c->setCursor(MX + (MW - iw) / 2, MY + MH - 20);
    c->print(info);

    drawNavBar(st);
}

void drawMenuView(const DataState& st) {
    drawStatusBar(st);
    auto* c = getCanvas();

    c->setTextSize(1);
    c->setCursor(MX + 4, MY + SBH + 8);
    c->setTextColor(FG, BG);
    c->print("= Menu =");
    int y = MY + SBH + 24;
    c->setTextColor(st.geiger_enabled ? ACCENT : DIM);
    c->setCursor(MX + 4, y); y += 12;
    c->printf("G: Geiger click  [%s]", st.geiger_enabled ? "ON" : "OFF");
    c->setTextColor(st.sd_logging ? (st.sd_ready ? ACCENT : ALARM) : DIM);
    c->setCursor(MX + 4, y); y += 12;
    const char* sdSt = st.sd_logging ? (st.sd_ready ? "ON" : "ERR") : "OFF";
    c->printf("L: SD logging    [%s]", sdSt);
    c->setTextColor(st.alarm_enabled ? ACCENT : DIM);
    c->setCursor(MX + 4, y); y += 12;
    c->printf("A: Alarm sound   [%s]", st.alarm_enabled ? "ON" : "OFF");
    c->setTextColor(FG);
    c->setCursor(MX + 4, y); y += 12;
    c->print("B: Brightness");
    c->setCursor(MX + 4, y); y += 12;
    c->print("R: Reset dose");
    drawNavBar(st);
}

// ── Scanning animation (canvas-based, no flicker) ──

static const char* _spinnerFrames = "|/-\\";

void drawScanningScreen(int animFrame) {
    M5.Display.setBrightness(255);
    auto* c = getCanvas();
    c->fillSprite(BG);
    c->drawRect(BORDER, BORDER, SCREEN_W-2*BORDER, SCREEN_H-2*BORDER, DIM);

    c->setTextSize(1);
    c->setTextColor(ACCENT);
    c->setCursor(MX + 4, MY + 4);
    c->print("Scanning for RadiaCode...");

    int cx = MX + MW / 2, cy = MY + MH / 2;
    c->setTextSize(4);
    c->setTextColor(ACCENT);
    c->setCursor(cx - 12, cy - 16);
    c->print(_spinnerFrames[animFrame & 3]);

    c->pushSprite(0, 0);
}

// ── Device selection list (uses canvas, no flicker) ──

void drawDeviceList(const std::vector<BleDevice>& devices, int sel, int scanSecs) {
    auto* c = getCanvas();
    c->fillSprite(BG);
    c->drawRect(BORDER, BORDER, SCREEN_W-2*BORDER, SCREEN_H-2*BORDER, DIM);

    c->setTextSize(1);
    c->setTextColor(ACCENT);
    c->setCursor(MX + 4, MY + 4);
    c->print("Select RadiaCode device:");
    c->drawFastHLine(MX, MY + 16, MW, DIM);

    if (devices.empty()) {
        c->setTextColor(WARN);
        c->setCursor(MX + 4, MY + 28);
        c->printf("Scan %ds left", scanSecs);
        c->setTextColor(DIM);
        c->setCursor(MX + 4, MY + 50);
        c->print("Make sure RadiaCode is");
        c->setCursor(MX + 4, MY + 62);
        c->print("powered on and in range.");
        c->pushSprite(0, 0);
        return;
    }

    if (scanSecs > 0) {
        c->setTextColor(DIM);
        c->setCursor(MX + MW - 28, MY + 4);
        c->printf("%ds", scanSecs);
    }

    int y = MY + 22, ih = 18, maxV = (MY + MH - y - 10) / ih;
    int sc = (sel >= maxV) ? sel - maxV + 1 : 0;
    for (int i = 0; i < (int)devices.size(); i++) {
        if (i < sc) continue;
        if (i - sc >= maxV) break;
        int py = y + (i - sc) * ih;
        if (i == sel) c->fillRect(MX, py, MW, ih, SEL_BG);
        c->setTextColor(i == sel ? TFT_BLACK : FG, i == sel ? SEL_BG : BG);
        c->setCursor(MX + 4, py + 1);
        c->print(devices[i].name.c_str());
        c->setTextColor(i == sel ? 0x4208 : DIM, i == sel ? SEL_BG : BG);
        c->setCursor(MX + MW - 40, py + 1);
        c->printf("%d dBm", devices[i].rssi);
    }
    c->setTextColor(DIM);
    c->setCursor(MX + 4, MY + MH - 10);
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
