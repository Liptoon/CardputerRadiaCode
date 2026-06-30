#include "radiacode_ble.h"
#include "config.h"
#include <cstring>

RadiaCodeBLE* g_inst = nullptr;

class _ScanCB : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice dev) override {
        if (!g_inst) return;
        if (!dev.haveName()) return;
        String name = dev.getName().c_str();
        if (name.indexOf("RadiaCode") < 0) return;
        g_inst->addDevice(BleDevice(dev.getAddress(), name, dev.getRSSI()));
    }
};

RadiaCodeBLE::RadiaCodeBLE() { g_inst = this; }
RadiaCodeBLE::~RadiaCodeBLE() { disconnect(); g_inst = nullptr; }

bool RadiaCodeBLE::begin() {
    BLEDevice::init("Cardputer-RC");
    _phase = PH_INIT;
    _scanStartMs = millis();
    return true;
}

void RadiaCodeBLE::_startScan() {
    _devices.clear();
    _sel = -1;
    _phase = PH_SCAN;
    _scanStartMs = millis();

    auto* s = BLEDevice::getScan();
    s->setAdvertisedDeviceCallbacks(new _ScanCB(), true);
    s->setActiveScan(true);
    s->setInterval(100);
    s->setWindow(99);
    s->start(10, (void(*)(BLEScanResults))nullptr, false);
}

void RadiaCodeBLE::update() {
    uint32_t now = millis();

    switch (_phase) {
        case PH_INIT:
            // Start scanning on first update() call
            _startScan();
            break;

        case PH_SCAN:
            if (now - _scanStartMs >= 10000) {
                BLEDevice::getScan()->stop();
                _phase = PH_SELECT;
                _sel = 0;
            }
            break;

        case PH_SELECT:
            break;

        case PH_CONNECT:
            if (now - _scanStartMs > 8000) {
                disconnect();
                _startScan();
            }
            break;

        case PH_INIT_BLE:
            _goInit();
            break;

        case PH_READY:
            break;
    }
}

void RadiaCodeBLE::connectToDevice(BLEAddress addr) {
    BLEDevice::getScan()->stop();
    _phase = PH_CONNECT;
    _scanStartMs = millis();

    _cl = BLEDevice::createClient();
    _cl->setClientCallbacks(this);
    if (!_cl->connect(addr, BLE_ADDR_TYPE_PUBLIC)) {
        disconnect();
        _startScan();
        return;
    }

    _sv = _cl->getService(BLEUUID(RC_SERVICE_UUID));
    if (!_sv) { disconnect(); _startScan(); return; }

    _wc = _sv->getCharacteristic(BLEUUID(RC_WRITE_UUID));
    _nc = _sv->getCharacteristic(BLEUUID(RC_NOTIFY_UUID));
    if (!_wc || !_nc) { disconnect(); _startScan(); return; }

    _nc->registerForNotify(_onNfy, true);
    BLERemoteDescriptor* cccd = _nc->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (cccd) { uint8_t v[] = {0x01,0x00}; cccd->writeValue(v,2,true); }

    _phase = PH_INIT_BLE;
    _is = 0;
}

bool RadiaCodeBLE::selectDevice(int index) {
    if (index < 0 || index >= (int)_devices.size()) return false;
    _sel = index;
    _phase = PH_CONNECT;
    _scanStartMs = millis();

    const auto& dev = _devices[index];
    _cl = BLEDevice::createClient();
    _cl->setClientCallbacks(this);
    if (!_cl->connect(dev.addr, BLE_ADDR_TYPE_PUBLIC)) {
        disconnect();
        _startScan();
        return false;
    }

    _sv = _cl->getService(BLEUUID(RC_SERVICE_UUID));
    if (!_sv) { disconnect(); _startScan(); return false; }

    _wc = _sv->getCharacteristic(BLEUUID(RC_WRITE_UUID));
    _nc = _sv->getCharacteristic(BLEUUID(RC_NOTIFY_UUID));
    if (!_wc || !_nc) { disconnect(); _startScan(); return false; }

    _nc->registerForNotify(_onNfy, true);
    BLERemoteDescriptor* cccd = _nc->getDescriptor(BLEUUID((uint16_t)0x2902));
    if (cccd) { uint8_t v[] = {0x01,0x00}; cccd->writeValue(v,2,true); }

    _phase = PH_INIT_BLE;
    _is = 0;
    return true;
}

void RadiaCodeBLE::selectNext() {
    if (_devices.empty()) return;
    _sel++; if (_sel >= (int)_devices.size()) _sel = 0;
}

void RadiaCodeBLE::selectPrev() {
    if (_devices.empty()) return;
    _sel--; if (_sel < 0) _sel = (int)_devices.size() - 1;
}

void RadiaCodeBLE::addDevice(const BleDevice& dev) {
    for (auto& d : _devices) { if (d.addr.equals(dev.addr)) return; }
    _devices.push_back(dev);
}

int RadiaCodeBLE::scanSecondsRemaining() const {
    uint32_t elapsed = millis() - _scanStartMs;
    int rem = 10 - (elapsed / 1000);
    return (rem >= 0) ? rem : 0;
}

void RadiaCodeBLE::_goInit() {
    switch (_is) {
        case 0: {
            uint8_t a[] = {0x01,0xff,0x12,0xff};
            if (_exe((uint16_t)Command::SET_EXCHANGE, a, 4)) _is = 1;
            else { disconnect(); _startScan(); }
            break;
        }
        case 1: {
            time_t t = millis()/1000;
            struct tm* ti = localtime(&t);
            uint8_t a[8];
            if (ti) {
                a[0]=ti->tm_mday; a[1]=ti->tm_mon+1; a[2]=(ti->tm_year+1900)-2000; a[3]=0;
                a[4]=ti->tm_sec; a[5]=ti->tm_min; a[6]=ti->tm_hour; a[7]=0;
            } else { memset(a,0,8); a[0]=1; a[1]=1; a[2]=24; }
            if (_exe((uint16_t)Command::SET_TIME, a, 8)) _is = 2;
            else { disconnect(); _startScan(); }
            break;
        }
        case 2: {
            uint32_t id = (uint32_t)VSFR::DEVICE_TIME;
            uint32_t val = 0;
            uint8_t z[8];
            memcpy(z, &id, 4); memcpy(z+4, &val, 4);
            if (_exe((uint16_t)Command::WR_VIRT_SFR, z, 8)) {
                _connected = true;
                _rdy = true; _phase = PH_READY; _tp = millis();
            } else { disconnect(); _startScan(); }
            break;
        }
    }
}

void RadiaCodeBLE::disconnect() {
    if (_cl) {
        if (_cl->isConnected()) _cl->disconnect();
        _cl = nullptr;
    }
    _sv = nullptr; _wc = nullptr; _nc = nullptr;
    _connected = false;
    _rdy = false; _seq = 0;
    _devices.clear();
    _sel = -1;
}

bool RadiaCodeBLE::pollDataState(DataState& st) {
    if (_phase != PH_READY || !_rdy) return false;
    uint32_t now = millis();
    if (now - _tp < 1000) return false;
    _tp = now;

    // 1) Read DATA_BUF for real-time & rare data
    uint8_t buf[512];
    size_t len = sizeof(buf);
    if (!_rdVS(VS::DATA_BUF, buf, &len)) {
        return false;
    }

    size_t pos = 0;
    while (pos + 7 <= len) {
        bool brk = false;
        uint8_t eid = buf[pos + 1];
        uint8_t gid = buf[pos + 2];
        pos += 7;
        size_t bsz = 0;

        if (eid == 0) {
            switch (gid) {
                case 0:
                    if (pos + 15 <= len) {
                        float cr, dr;
                        memcpy(&cr, buf+pos, 4);
                        memcpy(&dr, buf+pos+4, 4);
                        st.count_rate = cr;
                        st.dose_rate  = dr * 10000.0f;
                    }
                    bsz = 15;
                    break;
                case 1: bsz = 8;  break;
                case 2: bsz = 16; break;
                case 3:
                    if (pos + 14 <= len) {
                        float dr;
                        uint16_t tr, cr;
                        memcpy(&dr, buf+pos+4, 4);
                        tr = buf[pos+8]|((uint16_t)buf[pos+9]<<8);
                        cr = buf[pos+10]|((uint16_t)buf[pos+11]<<8);
                        st.dose = dr * 10000.0f;
                        st.temperature = (tr-2000)/100.0f;
                        st.battery = cr/100.0f;
                    }
                    bsz = 14;
                    break;
                case 4: case 5: bsz = 16; break;
                case 6: bsz = 6;  break;
                case 7: bsz = 4;  break;
                case 8: case 9: bsz = 6; break;
                default: brk = true; break;
            }
        } else if (eid == 1 && gid >= 1 && gid <= 3) {
            if (pos + 6 > len) { brk = true; }
            else {
                uint16_t snum = buf[pos]|((uint16_t)buf[pos+1]<<8);
                size_t extra = (gid==1) ? 8 : (gid==2) ? 16 : 14;
                bsz = 6 + snum * extra;
            }
        } else {
            brk = true;
        }

        if (brk) break;
        pos += bsz;
    }

    // 2) Read temperature directly via batch VSFR
    uint32_t ids[] = { (uint32_t)VSFR::TEMP_degC };
    uint32_t tmpVal;
    if (_rdVSFRbatch(ids, 1, &tmpVal)) {
        st.temperature = *(float*)&tmpVal;
    }

    st.ble_status = BLEStatus::Connected;
    st.last_data_ms = now;
    return true;
}

void RadiaCodeBLE::onConnect(BLEClient*) {}
void RadiaCodeBLE::onDisconnect(BLEClient*) { _connected = false; _rdy = false; }

void RadiaCodeBLE::_onNfy(BLERemoteCharacteristic*, uint8_t* d, size_t l, bool) {
    if (!g_inst) return;
    auto& R = *g_inst;
    if (R._re == 0) {
        if (l < 4) return;
        uint32_t t; memcpy(&t, d, 4);
        R._re = 4 + t;
        size_t cp = (l > 4) ? (l - 4) : 0;
        if (cp > sizeof(R._rb)) cp = sizeof(R._rb);
        if (cp) memcpy(R._rb, d + 4, cp);
        R._rl = cp;
    } else {
        size_t cp = l;
        if (R._rl + cp > sizeof(R._rb)) cp = sizeof(R._rb) - R._rl;
        if (cp) memcpy(R._rb + R._rl, d, cp);
        R._rl += cp;
    }
    R._re -= l;
    if (R._re <= 0) { R._rr = true; R._re = 0; }
}

bool RadiaCodeBLE::_send(uint16_t c, const uint8_t* a, size_t al) {
    if (!_wc) return false;
    uint8_t sq = 0x80 | (_seq & 0x1F);
    _seq = (_seq + 1) & 0x1F;
    uint8_t h[4] = {(uint8_t)c,(uint8_t)(c>>8),0,sq};
    size_t pl = 4 + al, tl = 4 + pl;
    uint8_t* b = (uint8_t*)malloc(tl);
    if (!b) return false;
    b[0]=pl&0xFF; b[1]=(pl>>8)&0xFF; b[2]=(pl>>16)&0xFF; b[3]=(pl>>24)&0xFF;
    memcpy(b+4, h, 4); if (al) memcpy(b+8, a, al);
    size_t p = 0;
    while (p < tl) {
        size_t ch = tl - p; if (ch > 18) ch = 18;
        _wc->writeValue(b + p, ch, false);
        delay(5);
        p += ch;
    }
    free(b); return true;
}

bool RadiaCodeBLE::_wait(uint32_t ms) {
    _rr = false; _rl = 0; _re = 0;
    uint32_t s = millis();
    while ((millis()-s) < ms) { if (_rr) return true; delay(1); }
    return false;
}

bool RadiaCodeBLE::_exe(uint16_t c, const uint8_t* a, size_t al) {
    return _send(c, a, al) && _wait(3000);
}

bool RadiaCodeBLE::_exeR(uint16_t c, const uint8_t* a, size_t al, uint8_t* r, size_t* rl) {
    if (!_send(c, a, al)) return false;
    if (!_wait(3000)) return false;
    if (_rl < 4) return false;
    size_t pl = _rl - 4;
    if (r && rl) { size_t cp = (pl < *rl) ? pl : *rl; memcpy(r, _rb + 4, cp); *rl = pl; }
    return true;
}

bool RadiaCodeBLE::_rdVS(VS id, uint8_t* d, size_t* l) {
    uint32_t v = (uint32_t)id;
    uint8_t a[4] = {(uint8_t)v,(uint8_t)(v>>8),(uint8_t)(v>>16),(uint8_t)(v>>24)};
    uint8_t r[1024]; size_t rl = sizeof(r);
    if (!_exeR((uint16_t)Command::RD_VIRT_STRING, a, 4, r, &rl)) return false;
    if (rl < 8) return false;
    uint32_t rc = r[0]|((uint32_t)r[1]<<8)|((uint32_t)r[2]<<16)|((uint32_t)r[3]<<24);
    if (rc != 1) return false;
    uint32_t fl = r[4]|((uint32_t)r[5]<<8)|((uint32_t)r[6]<<16)|((uint32_t)r[7]<<24);
    size_t dl = rl - 8;
    if (dl == fl + 1 && r[rl-1] == 0x00) dl = fl;
    if (fl > dl) return false;
    if (d && l) { size_t cp = (fl < *l) ? fl : *l; memcpy(d, r + 8, cp); *l = fl; }
    return true;
}

bool RadiaCodeBLE::_rdVSFRbatch(const uint32_t* ids, size_t n, uint32_t* vals) {
    if (!n) return false;
    uint8_t a[4 + 4*n];
    a[0]=n&0xFF; a[1]=(n>>8)&0xFF; a[2]=(n>>16)&0xFF; a[3]=(n>>24)&0xFF;
    for (size_t i=0; i<n; i++) {
        size_t o=4+i*4;
        a[o]=ids[i]&0xFF; a[o+1]=(ids[i]>>8)&0xFF;
        a[o+2]=(ids[i]>>16)&0xFF; a[o+3]=(ids[i]>>24)&0xFF;
    }
    uint8_t r[256]; size_t rl = sizeof(r);
    if (!_exeR((uint16_t)Command::RD_VIRT_SFR_BATCH, a, 4+4*n, r, &rl)) return false;
    if (rl < 4+4*n) return false;
    uint32_t vf = r[0]|((uint32_t)r[1]<<8)|((uint32_t)r[2]<<16)|((uint32_t)r[3]<<24);
    if (vf != ((1<<n)-1)) return false;
    for (size_t i=0; i<n; i++) {
        size_t o=4+i*4;
        vals[i] = r[o]|((uint32_t)r[o+1]<<8)|((uint32_t)r[o+2]<<16)|((uint32_t)r[o+3]<<24);
    }
    return true;
}
