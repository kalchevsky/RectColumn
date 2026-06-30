#pragma once

#include <Arduino.h>

#include "Sensors.h"
#include "config.h"

namespace FactoryDefaultsTable {

inline constexpr const char* WIFI_AP_PASS = "";
inline constexpr const char* WIFI_STA_SSID = "";
inline constexpr const char* WIFI_STA_PASS = "";
inline constexpr bool WIFI_AP_ONLY = true;
inline constexpr bool WIFI_WIZARD_DONE = false;
inline constexpr bool SYSTEM_FACTORY_DONE = true;

inline constexpr uint8_t OUTPUT_CH_MODE[3] = {
    LOGIC_HEAT,
    LOGIC_HEAT,
    LOGIC_HEAT
};
inline constexpr bool OUTPUT_CH4_ENABLED = false;
inline constexpr bool OUTPUT_CH5_ENABLED = false;
inline constexpr bool OUTPUT_SOUND_MUTED = true;

struct AlarmPreset {
    bool  enabled;
    float threshold;
    bool  isMax;
};

struct TempFamilyPreset {
    bool      enabled;
    bool      ctrlEnabled;
    float     ctrlMinVal;
    float     ctrlMaxVal;
    AlarmPreset alarm0;
    AlarmPreset alarm1;
    uint32_t  alarmDelayMs;
    uint32_t  ctrlDelayMs;
};

struct PressurePreset {
    bool     enabled;
    bool     ctrlEnabled;
    float    ctrlMinVal;
    float    ctrlMaxVal;
    bool     alarmsEnabled[N_ALARMS];
    uint32_t alarmDelayMs;
    uint32_t ctrlDelayMs;
};

inline constexpr TempFamilyPreset TEMP_FAMILY = {
    false,
    false,
    -20.0f,
    100.0f,
    { false, -20.0f, false },
    { false, 100.0f, true },
    0UL,
    0UL
};

inline constexpr PressurePreset PRESSURE = {
    false,
    false,
    900.0f,
    1100.0f,
    { false, false, false, false },
    0UL,
    0UL
};

inline constexpr bool OFF_ONLY_ENABLED = false;
inline constexpr bool OFF_ONLY_ALARM_ENABLED = false;

inline constexpr bool NOTIFY_ENABLED = false;
inline constexpr const char* NOTIFY_URL = "";
inline constexpr const char* NOTIFY_TOKEN = "";

} // namespace FactoryDefaultsTable
