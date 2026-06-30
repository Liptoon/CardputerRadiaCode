#pragma once

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>
#include <vector>

#include "radiacode_protocol.h"
#include "radiacode_data.h"

struct BleDevice {
    BLEAddress addr;
    String     name;
    int        rssi;

    BleDevice() : addr((uint8_t*)"\0\0\0\0\0\0"), rssi(0) {}
    BleDevice(BLEAddress a, String n, int r) : addr(a), name(n), rssi(r) {}
};

extern class RadiaCodeBLE* g_inst;

class RadiaCodeBLE : public BLEClientCallbacks {
public:
    RadiaCodeBLE();
    ~RadiaCodeBLE();

    bool begin();
    void update();
    bool connected() const { return _connected; }
    void disconnect();
    bool pollDataState(DataState& state);

    const std::vector<BleDevice>& devices() const { return _devices; }
    bool selectDevice(int index);
    int  selectedIndex() const { return _sel; }
    void selectNext();
    void selectPrev();
    void addDevice(const BleDevice& dev);
    int  scanSecondsRemaining() const;
    bool isScanning() const { return _phase == PH_SCAN; }

    void onConnect(BLEClient*) override;
    void onDisconnect(BLEClient*) override;
    void connectToDevice(BLEAddress addr);

private:
    enum Phase : uint8_t {
        PH_INIT,      // BLE init, scan starts immediately
        PH_SCAN,      // scanning for 10s
        PH_SELECT,    // device list ready, waiting for user
        PH_CONNECT,   // connecting to selected device
        PH_INIT_BLE,  // RadiaCode init handshake
        PH_READY,     // connected and initialized
    };

    Phase _phase = PH_INIT;

    BLEClient*      _cl = nullptr;
    BLERemoteService*        _sv = nullptr;
    BLERemoteCharacteristic* _wc = nullptr;
    BLERemoteCharacteristic* _nc = nullptr;

    std::vector<BleDevice> _devices;
    int  _sel = -1;

    uint8_t  _seq = 0;
    bool     _rdy = false;
    bool     _connected = false;

    uint8_t  _rb[1024];
    size_t   _rl = 0;
    size_t   _re = 0;
    bool     _rr = false;

    uint32_t _scanStartMs = 0;
    uint32_t _tp = 0;
    uint8_t  _is = 0;

    void _startScan();
    void _goInit();
    bool _send(uint16_t c, const uint8_t* a, size_t al);
    bool _wait(uint32_t ms);
    bool _exe(uint16_t c, const uint8_t* a = nullptr, size_t al = 0);
    bool _exeR(uint16_t c, const uint8_t* a, size_t al, uint8_t* r, size_t* rl);
    bool _rdVS(VS id, uint8_t* d, size_t* l);
    bool _rdVSFRbatch(const uint32_t* ids, size_t n, uint32_t* vals);

    static void _onNfy(BLERemoteCharacteristic* c, uint8_t* d, size_t l, bool n);
};
