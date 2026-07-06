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
inline constexpr bool OUTPUT_SOUND_MUTED = false;

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
    uint32_t  periodMs;
    uint32_t  alarmDelayMs;
    uint32_t  ctrlDelayMs;
};

struct PressurePreset {
    bool     enabled;
    bool     ctrlEnabled;
    float    ctrlMinVal;
    float    ctrlMaxVal;
    bool     alarmsEnabled[N_ALARMS];
    float    alarmThreshold[N_ALARMS];
    bool     alarmIsMax[N_ALARMS];
    uint32_t periodMs;
    uint32_t alarmDelayMs;
    uint32_t ctrlDelayMs;
};

inline constexpr TempFamilyPreset TEMP_FAMILY = {
    false,
    false,
    -20.0f,
    100.0f,
    { false, -20.0f, false },
    { false, -20.0f, false },
    1000UL,
    0UL,
    0UL
};

inline constexpr PressurePreset PRESSURE = {
    false,
    false,
    900.0f,
    1100.0f,
    { false, false, false, false },
    { 900.0f, 900.0f, 1100.0f, 1100.0f },
    { false, false, true, true },
    1000UL,
    0UL,
    0UL
};

inline constexpr bool OFF_ONLY_ENABLED = false;
inline constexpr bool OFF_ONLY_ALARM_ENABLED = false;
inline constexpr uint32_t OFF_ONLY_PERIOD_MS = 1000UL;
inline constexpr uint32_t OFF_ONLY_ALARM_DELAY_MS = 0UL;
inline constexpr uint32_t OFF_ONLY_CTRL_DELAY_MS = 0UL;
inline constexpr bool OFF_ONLY_CTRL_ENABLED = false;

// Ток нагрузки: 0.5 A -> проценты по калибровке фронтенда:
// percent = amps*SLOPE + ZERO = 0.5*1.67 + 31.27 = 32.105
// Калибровка (31.27/1.67/0.05) сейчас живёт только в OUT/page-app.js.
// TODO(backlog): вынести калибровку тока в единый C++-источник.
inline constexpr float CURRENT_ALARM_THRESHOLD_PERCENT = 32.105f;

inline constexpr bool NOTIFY_ENABLED = false;
inline constexpr const char* NOTIFY_URL = "";
inline constexpr const char* NOTIFY_TOKEN = "";

} // namespace FactoryDefaultsTable
