#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\AckButton.h"
#pragma once

#include <Arduino.h>

#include "config.h"
#include "OutputManager.h"
#include "SensorManager.h"
#include "EventLog.h"

class AckButton {
public:
    void begin() {
        if (PIN_ACK_BUTTON < 0) return;
        pinMode(PIN_ACK_BUTTON, INPUT_PULLUP);
        _lastRaw = digitalRead(PIN_ACK_BUTTON);
        _stableState = _isPressed(_lastRaw);
        _lastDebounceMs = millis();
    }

    void loop(OutputManager& om, SensorManager& sm, EventLog* log) {
        if (PIN_ACK_BUTTON < 0) return;

        const int raw = digitalRead(PIN_ACK_BUTTON);
        const uint32_t now = millis();
        if (raw != _lastRaw) {
            _lastDebounceMs = now;
            _lastRaw = raw;
        }
        if ((now - _lastDebounceMs) < ACK_BUTTON_DEBOUNCE_MS) return;

        const bool pressed = _isPressed(raw);
        if (pressed == _stableState) return;
        _stableState = pressed;
        if (!pressed) return;

        const uint16_t before = om.activeAlarmCount(sm);
        om.acknowledgeCurrentAlarms(sm);
        om.beepAcceptedCommand();
        if (log && before > 0) {
            log->add("Оператор подтвердил тревоги (кнопка)",
                     sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
        }
    }

private:
    static bool _isPressed(int raw) {
#if ACK_BUTTON_ACTIVE_LOW
        return raw == LOW;
#else
        return raw != LOW;
#endif
    }

    int _lastRaw = HIGH;
    uint32_t _lastDebounceMs = 0;
    bool _stableState = false;
};
