#pragma once

#include <Arduino.h>

#include "config.h"
#include "FactoryResetConfig.h"
#include "OutputManager.h"
#include "SensorManager.h"
#include "EventLog.h"

class AckButton {
public:
    enum class WiFiLedOverrideMode : uint8_t {
        None = 0,
        Holding = 1,
        Armed = 2,
    };

    void begin() {
        if (PIN_ACK_BUTTON < 0) return;
        pinMode(PIN_ACK_BUTTON, INPUT_PULLUP);
        _lastRaw = digitalRead(PIN_ACK_BUTTON);
        _stableState = _isPressed(_lastRaw);
        _lastDebounceMs = millis();
        _pressActive = false;
        _factoryResetArmed = false;
        _factoryResetReleasePending = false;
        _armedBeepActive = false;
        _armedBeepPulsesRemaining = 0;
        _nextArmedBeepMs = 0;
    }

    void loop(OutputManager& om, SensorManager& sm, EventLog* log) {
        if (PIN_ACK_BUTTON < 0) return;

        const uint32_t now = millis();
        _advanceArmedBeep(om, now);

        const int raw = digitalRead(PIN_ACK_BUTTON);
        if (raw != _lastRaw) {
            _lastDebounceMs = now;
            _lastRaw = raw;
        }
        if ((now - _lastDebounceMs) >= ACK_BUTTON_DEBOUNCE_MS) {
            const bool pressed = _isPressed(raw);
            if (pressed != _stableState) {
                _stableState = pressed;
                if (pressed) _handlePressed(om, sm, log, now);
                else _handleReleased(log, sm);
            }
        }

        _advanceHold(om, now);
    }

    WiFiLedOverrideMode wifiLedOverrideMode() const {
        if (_factoryResetArmed) return WiFiLedOverrideMode::Armed;
        if (_pressActive && _stableState) return WiFiLedOverrideMode::Holding;
        return WiFiLedOverrideMode::None;
    }

    bool wifiLedOverrideLevel(uint32_t now) const {
        if (_factoryResetArmed) return true;
        if (_pressActive && _stableState) {
            return ((now / FactoryResetConfig::HOLDING_LED_BLINK_MS) % 2u) == 0u;
        }
        return false;
    }

    bool consumeFactoryResetReleasePending() {
        const bool pending = _factoryResetReleasePending;
        _factoryResetReleasePending = false;
        return pending;
    }

private:
    static bool _isPressed(int raw) {
#if ACK_BUTTON_ACTIVE_LOW
        return raw == LOW;
#else
        return raw != LOW;
#endif
    }

    void _handlePressed(OutputManager& om, SensorManager& sm, EventLog* log, uint32_t now) {
        _pressActive = true;
        _pressStartedMs = now;
        _factoryResetArmed = false;
        _factoryResetReleasePending = false;
        _armedBeepActive = false;
        _armedBeepPulsesRemaining = 0;
        _nextArmedBeepMs = 0;

        const uint16_t before = om.activeAlarmCount(sm);
        om.acknowledgeCurrentAlarms(sm);
        om.beepAcceptedCommand();
        if (log && before > 0) {
            log->add("Оператор подтвердил тревоги (кнопка)",
                     sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
        }
    }

    void _handleReleased(EventLog* log, SensorManager& sm) {
        const bool wasArmed = _factoryResetArmed;
        _pressActive = false;
        _factoryResetArmed = false;
        if (!wasArmed) return;

        _factoryResetReleasePending = true;
        if (log) {
            log->add("factory reset armed (release)",
                     sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
        }
    }

    void _advanceHold(OutputManager& om, uint32_t now) {
        if (!_pressActive || !_stableState || _factoryResetArmed) return;
        if ((now - _pressStartedMs) < FactoryResetConfig::HOLD_MS) return;

        _factoryResetArmed = true;
        _startArmedBeep(om, now);
    }

    void _startArmedBeep(OutputManager& om, uint32_t now) {
        _armedBeepActive = false;
        _armedBeepPulsesRemaining = 0;
        _nextArmedBeepMs = 0;

        Output* buzzer = (OUT_CH5 < OUT_COUNT) ? om.out[OUT_CH5] : nullptr;
        if (!om.ch5Enabled || !buzzer || buzzer->isOn()) return;

        buzzer->requestPulse(FactoryResetConfig::ARMED_BEEP_ON_MS);
        if (FactoryResetConfig::ARMED_BEEP_COUNT <= 1) return;

        _armedBeepActive = true;
        _armedBeepPulsesRemaining = (uint8_t)(FactoryResetConfig::ARMED_BEEP_COUNT - 1);
        _nextArmedBeepMs = now + FactoryResetConfig::ARMED_BEEP_ON_MS + FactoryResetConfig::ARMED_BEEP_OFF_MS;
    }

    void _advanceArmedBeep(OutputManager& om, uint32_t now) {
        if (!_armedBeepActive) return;
        if ((int32_t)(now - _nextArmedBeepMs) < 0) return;

        Output* buzzer = (OUT_CH5 < OUT_COUNT) ? om.out[OUT_CH5] : nullptr;
        if (!om.ch5Enabled || !buzzer || buzzer->isOn()) {
            _armedBeepActive = false;
            _armedBeepPulsesRemaining = 0;
            return;
        }

        buzzer->requestPulse(FactoryResetConfig::ARMED_BEEP_ON_MS);
        if (_armedBeepPulsesRemaining <= 1) {
            _armedBeepActive = false;
            _armedBeepPulsesRemaining = 0;
            return;
        }

        _armedBeepPulsesRemaining--;
        _nextArmedBeepMs = now + FactoryResetConfig::ARMED_BEEP_ON_MS + FactoryResetConfig::ARMED_BEEP_OFF_MS;
    }

    int _lastRaw = HIGH;
    uint32_t _lastDebounceMs = 0;
    bool _stableState = false;
    bool _pressActive = false;
    uint32_t _pressStartedMs = 0;
    bool _factoryResetArmed = false;
    bool _factoryResetReleasePending = false;
    bool _armedBeepActive = false;
    uint8_t _armedBeepPulsesRemaining = 0;
    uint32_t _nextArmedBeepMs = 0;
};
