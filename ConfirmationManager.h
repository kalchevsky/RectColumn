// ================================================================
// ConfirmationManager.h 
// ================================================================
#pragma once

#include <Arduino.h>
#include "config.h"
#include "OutputManager.h"
#include "EventLog.h"
#include "SensorManager.h"

enum ConfirmationFault : uint8_t {
    CONFIRM_FAULT_NONE = 0,
    CONFIRM_FAULT_NO_ON_CONFIRM,
    CONFIRM_FAULT_STUCK_ON,
    CONFIRM_FAULT_STUCK_HIGH_BEFORE_ON,
};

struct ConfirmationChannel {
    const char* id       = "";
    const char* outputId = "";
    uint8_t     outputIdx = 0;
    int8_t      pin = -1;

    bool available = false;
    bool raw = false;
    bool actual = false;
    bool expected = false;
    bool confirmed = false;
    bool pending = false;
    bool mismatch = false;
    bool timeout = false;
    bool faultLatched = false;
    uint8_t fault = CONFIRM_FAULT_NONE;

    uint32_t debounceMs = WER_DEBOUNCE_MS;
    uint32_t timeoutMs  = WER_CONFIRM_TIMEOUT_MS;

    uint32_t rawChangedMs = 0;
    uint32_t actualChangedMs = 0;
    uint32_t expectedChangedMs = 0;
    bool     rawInitialized = false;
    const char* note = "";
};

class ConfirmationManager {
public:
    ConfirmationManager() {
        _ch[0].id = "WER_CH1"; _ch[0].outputId = "CH1"; _ch[0].outputIdx = OUT_CH1; _ch[0].pin = PIN_WER_CH1;
        _ch[1].id = "WER_CH2"; _ch[1].outputId = "CH2"; _ch[1].outputIdx = OUT_CH2; _ch[1].pin = PIN_WER_CH2;
        _ch[2].id = "WER_CH3"; _ch[2].outputId = "CH3"; _ch[2].outputIdx = OUT_CH3; _ch[2].pin = PIN_WER_CH3;
        _ch[3].id = "WER_CH4"; _ch[3].outputId = "CH4"; _ch[3].outputIdx = OUT_CH4; _ch[3].pin = PIN_WER_CH4;

    #if RECT_HW_HAS_WER_BELL
        // зарезервировано для будущей ревизии платы
    #endif
    }

    void begin() {
        for (uint8_t i = 0; i < 4; i++) {
            ConfirmationChannel& c = _ch[i];
            c.available = _isAvailable(i);
            c.note = _noteFor(i);

            if (c.available) {
                if (_supportsInternalPulldown(c.pin)) pinMode(c.pin, INPUT_PULLDOWN);
                else pinMode(c.pin, INPUT);
            }

            bool start = false;
            if (c.available) start = _readPhysical(i);

            c.raw = start;
            c.actual = start;
            c.expected = false;
            c.confirmed = (!c.expected && !c.actual);
            c.pending = false;
            c.mismatch = false;
            c.timeout = false;
            c.faultLatched = false;
            c.fault = CONFIRM_FAULT_NONE;
            c.rawChangedMs = millis();
            c.actualChangedMs = millis();
            c.expectedChangedMs = millis();
            c.rawInitialized = true;
        }
    }

    void loop(const OutputManager& om, const SensorManager& sm, EventLog* log) {
        const uint32_t now = millis();

        for (uint8_t i = 0; i < 4; i++) {
            ConfirmationChannel& c = _ch[i];
            c.available = _isAvailable(i);
            c.note = _noteFor(i);

            const bool prevMismatch = c.mismatch;
            const bool prevTimeout = c.timeout;
            const uint8_t prevFault = c.fault;

            if (!c.available) {
                c.raw = false;
                c.actual = false;
                c.expected = om.out[c.outputIdx]->actualOn();
                c.confirmed = false;
                c.pending = false;
                c.mismatch = false;
                c.timeout = false;
                continue;
            }

            const bool sample = _readPhysical(i);

            if (!c.rawInitialized) {
                c.raw = sample;
                c.actual = sample;
                c.rawChangedMs = now;
                c.actualChangedMs = now;
                c.expectedChangedMs = now;
                c.rawInitialized = true;
            }

            if (sample != c.raw) {
                c.raw = sample;
                c.rawChangedMs = now;
            }

            if (c.actual != c.raw && (now - c.rawChangedMs >= c.debounceMs)) {
                c.actual = c.raw;
                c.actualChangedMs = now;
            }

            const bool newExpected = om.out[c.outputIdx]->actualOn();
            if (newExpected != c.expected) {
                c.expected = newExpected;
                c.expectedChangedMs = now;
                if (c.expected && c.actual && !c.faultLatched) {
                    c.faultLatched = true;
                    c.fault = CONFIRM_FAULT_STUCK_HIGH_BEFORE_ON;
                }
            }

            c.pending = false;
            c.mismatch = false;
            c.timeout = false;
            c.confirmed = false;

            if (c.faultLatched) {
                c.mismatch = true;
                c.timeout = true;
            } else if (c.expected) {
                if (c.actual && c.actualChangedMs >= c.expectedChangedMs) {
                    c.confirmed = true;
                } else {
                    const uint32_t dt = now - c.expectedChangedMs;
                    c.pending = (dt < c.timeoutMs);
                    c.timeout = (dt >= c.timeoutMs);
                    c.mismatch = c.timeout;
                    if (c.timeout) {
                        c.faultLatched = true;
                        c.fault = CONFIRM_FAULT_NO_ON_CONFIRM;
                    }
                }
            } else {
                if (!c.actual) {
                    c.confirmed = true;
                } else {
                    c.mismatch = true;
                    if ((now - c.expectedChangedMs) >= c.timeoutMs) {
                        c.timeout = true;
                        c.faultLatched = true;
                        c.fault = CONFIRM_FAULT_STUCK_ON;
                    }
                }
            }

            if (prevFault != c.fault && c.fault != CONFIRM_FAULT_NONE && log) {
                log->add(_faultMessage(c), sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
            } else if (!prevMismatch && c.mismatch && log) {
                if (c.timeout) {
                    log->add(_mismatchMessage(c), sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
                }
            } else if (prevMismatch && !c.mismatch && log) {
                if (prevTimeout) {
                    log->add(String(c.id) + " restored after timeout", sm.getT1(), sm.getT2(), sm.getT3(), sm.getDT());
                }
            }
        }
    }

    const ConfirmationChannel& get(uint8_t idx) const { return _ch[idx]; }

    void setEmuActive(uint8_t idx, bool active) {
    #if EMU_MODE
        if (idx < 4) _emuActive[idx] = active;
    #else
        (void)idx; (void)active;
    #endif
    }

    bool anyMismatch() const {
        for (uint8_t i = 0; i < 4; i++) {
            if (_ch[i].mismatch) return true;
        }
        return false;
    }

    bool anyFaultLatched() const {
        for (uint8_t i = 0; i < 4; i++) {
            if (_ch[i].faultLatched) return true;
        }
        return false;
    }

    void resetFaults() {
        for (uint8_t i = 0; i < 4; i++) {
            _ch[i].faultLatched = false;
            _ch[i].fault = CONFIRM_FAULT_NONE;
            _ch[i].timeout = false;
            _ch[i].mismatch = false;
        }
    }

    static const char* faultName(uint8_t fault) {
        switch (fault) {
            case CONFIRM_FAULT_NO_ON_CONFIRM:          return "no_on_confirm";
            case CONFIRM_FAULT_STUCK_ON:               return "stuck_on";
            case CONFIRM_FAULT_STUCK_HIGH_BEFORE_ON:   return "stuck_high_before_on";
            default:                                   return "";
        }
    }

private:
    ConfirmationChannel _ch[4];
    bool _emuActive[4] = {false, false, false, false};

    bool _supportsInternalPulldown(int8_t pin) const {
        return pin == PIN_WER_CH1;
    }

    bool _isAvailable(uint8_t idx) const {
    #if EMU_MODE
        return true;
    #else
        if (idx == 1 && GPIO35_MODE != GPIO35_MODE_WER_CH2) return false;
        return true;
    #endif
    }

    const char* _noteFor(uint8_t idx) const {
    #if EMU_MODE
        if (idx == 1 && GPIO35_MODE != GPIO35_MODE_WER_CH2) {
            return "EMU override; real hardware disables WER_CH2 in GPIO35_MODE_V_SENSOR";
        }
        return "";
    #else
        if (idx == 1 && GPIO35_MODE != GPIO35_MODE_WER_CH2) {
            return "Unavailable: GPIO35 reserved for V sensor in this build";
        }
        return "";
    #endif
    }

    bool _readPhysical(uint8_t idx) const {
    #if EMU_MODE
        return _emuActive[idx];
    #else
        int raw = digitalRead(_ch[idx].pin);
        if (WER_ACTIVE_LOW) return raw == LOW;
        return raw == HIGH;
    #endif
    }

    String _mismatchMessage(const ConfirmationChannel& c) const {
        if (c.expected && !c.actual && c.timeout) {
            return String(c.id) + " timeout: output ON but confirmation missing";
        }
        if (!c.expected && c.actual) {
            return String(c.id) + " mismatch: confirmation active while output OFF";
        }
        return String(c.id) + " mismatch";
    }

    String _faultMessage(const ConfirmationChannel& c) const {
        switch (c.fault) {
            case CONFIRM_FAULT_NO_ON_CONFIRM:
                return String(c.id) + " fault: output ON but confirmation missing";
            case CONFIRM_FAULT_STUCK_ON:
                return String(c.id) + " fault: confirmation stays ON while output OFF";
            case CONFIRM_FAULT_STUCK_HIGH_BEFORE_ON:
                return String(c.id) + " fault: confirmation was HIGH before ON command";
            default:
                return String(c.id) + " fault";
        }
    }
};
