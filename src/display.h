#pragma once

#include "radiacode_data.h"
#include "radiacode_ble.h"
#include <vector>

void displayInit();
void displayUpdate(const DataState& st);
void drawMainView(const DataState& st);
void drawSpectrumView(const DataState& st);
void drawMenuView(const DataState& st);
void drawStatusBar(const DataState& st);
void drawNavBar(const DataState& st);
void drawBatteryIcon(int x, int y, float pct);
void drawScanningScreen(int animFrame);
void drawDeviceList(const std::vector<BleDevice>& devices, int sel, int scanSecs, bool connecting);
void drawStatusMsg(const char* msg);
