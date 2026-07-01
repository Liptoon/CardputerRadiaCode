#pragma once

#include <cstdint>
#include <WString.h>

// ── BLE Service / Characteristics ──
static constexpr const char* RC_SERVICE_UUID  = "e63215e5-7003-49d8-96b0-b024798fb901";
static constexpr const char* RC_WRITE_UUID    = "e63215e6-7003-49d8-96b0-b024798fb901";
static constexpr const char* RC_NOTIFY_UUID   = "e63215e7-7003-49d8-96b0-b024798fb901";

// ── Commands ──
enum class Command : uint16_t {
    GET_STATUS        = 0x0005,
    SET_EXCHANGE      = 0x0007,
    GET_VERSION       = 0x000A,
    GET_SERIAL        = 0x000B,
    FW_SIGNATURE      = 0x0101,
    RD_VIRT_SFR       = 0x0824,
    WR_VIRT_SFR       = 0x0825,
    RD_VIRT_STRING    = 0x0826,
    WR_VIRT_STRING    = 0x0827,
    RD_VIRT_SFR_BATCH = 0x082A,
    WR_VIRT_SFR_BATCH = 0x082B,
    SET_TIME          = 0x0A04,
};

// ── Virtual Strings (VS) ──
enum class VS : uint32_t {
    CONFIGURATION = 2,
    FW_DESCRIPTOR = 3,
    SERIAL_NUMBER = 8,
    TEXT_MESSAGE  = 0xF,
    MEM_SNAPSHOT  = 0xE0,
    DATA_BUF      = 0x100,
    SFR_FILE      = 0x101,
    SPECTRUM      = 0x200,
    ENERGY_CALIB  = 0x202,
    SPEC_ACCUM    = 0x205,
    SPEC_DIFF     = 0x206,
};

// ── Virtual SFRs (VSFR) ──
enum class VSFR : uint32_t {
    DEVICE_CTRL   = 0x0500,
    DEVICE_LANG   = 0x0502,
    DEVICE_ON     = 0x0503,
    DEVICE_TIME   = 0x0504,

    DISP_BRT      = 0x0511,
    DISP_OFF_TIME = 0x0513,

    SOUND_ON      = 0x0522,
    VIBRO_ON      = 0x0531,

    ALARM_MODE    = 0x05E0,
    PLAY_SIGNAL   = 0x05E1,

    DR_LEV1_uR_h  = 0x8000,
    DR_LEV2_uR_h  = 0x8001,
    DS_LEV1_uR    = 0x8014,
    DS_LEV2_uR    = 0x8015,
    CR_LEV1_cp10s = 0x8008,
    CR_LEV2_cp10s = 0x8009,

    CPS           = 0x8020,
    DR_uR_h       = 0x8021,
    DS_uR         = 0x8022,
    TEMP_degC     = 0x8024,
    ACC_X         = 0x8025,
    ACC_Y         = 0x8026,
    ACC_Z         = 0x8027,

    CHN_TO_keV_A0 = 0x8010,
    CHN_TO_keV_A1 = 0x8011,
    CHN_TO_keV_A2 = 0x8012,
};

// ── DATA_BUF group / event IDs ──
enum class DataGroup : uint8_t {
    GRP_RealTimeData = 0,
    GRP_RawData      = 1,
    GRP_DoseRateDB   = 2,
    GRP_RareData     = 3,
    GRP_UserData     = 4,
    GRP_SheduleData  = 5,
    GRP_AccelData    = 6,
    GRP_Event        = 7,
    GRP_RawCountRate = 8,
    GRP_RawDoseRate  = 9,
};

// ── Alarm flags in RealTimeData.flags ──
enum AlarmFlag {
    ALARM_DR_L1   = 1 << 2,
    ALARM_DR_L2   = 1 << 3,
    ALARM_DOSE_L1 = 1 << 5,
    ALARM_DOSE_L2 = 1 << 6,
    ALARM_CR_L1   = 1 << 10,
    ALARM_CR_L2   = 1 << 11,
};
#define ALARM_ANY (ALARM_DR_L1|ALARM_DR_L2|ALARM_DOSE_L1|ALARM_DOSE_L2|ALARM_CR_L1|ALARM_CR_L2)

// ── RealTimeData from DATA_BUF ──
struct RealTimeData {
    float   count_rate;
    float   count_rate_err;   // 0.1 %
    uint16_t dose_rate;       // μSv/h
    uint16_t dose_rate_err;   // 0.1 %
    uint16_t flags;
    uint8_t  real_time_flags;
};

// ── RareData from DATA_BUF ──
struct RareData {
    uint32_t duration;        // seconds of accumulation
    float    dose;            // accumulated dose
    float    temperature;     // °C
    float    charge_level;    // 0.0 – 1.0 (battery)
    uint16_t flags;
};

// ── Spectrum data ──
#define SPECTRUM_CHANNELS 1024

struct SpectrumData {
    uint32_t live_time;       // seconds
    float    a0, a1, a2;     // calibration coeffs
    uint32_t counts[SPECTRUM_CHANNELS];
    uint16_t valid_channels;
};
