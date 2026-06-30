#pragma once

#include <Arduino.h>

namespace FactoryResetConfig {
inline constexpr uint32_t HOLD_MS = 10000UL;
inline constexpr uint8_t ARMED_BEEP_COUNT = 3;
inline constexpr uint32_t ARMED_BEEP_ON_MS = 80UL;
inline constexpr uint32_t ARMED_BEEP_OFF_MS = 120UL;
inline constexpr uint32_t HOLDING_LED_BLINK_MS = 250UL;
}  // namespace FactoryResetConfig
