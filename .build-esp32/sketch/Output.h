#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\Output.h"
// ================================================================
// Output.h
// ================================================================
#pragma once
#include <Arduino.h>
#include "config.h"

enum RelayCommand : uint8_t {
    CMD_NONE = 0,
    CMD_ON   = 1,
    CMD_OFF  = 2,
};

static inline const char* relayCommandName(RelayCommand cmd) {
    switch (cmd) {
        case CMD_ON:  return "on";
        case CMD_OFF: return "off";
        default:      return "none";
    }
}

class Output {
public:
    String   name;
    uint8_t  pin;
    bool     inverted = false;
    bool     enabled  = true;

    Output(const String& n, uint8_t p, bool inv = false)
        : name(n), pin(p), inverted(inv) {}

    void begin() {
        pinMode(pin, OUTPUT);
        _actualOn = false;
        _recomputeRequestedLevel();
        _write(false);
    }

    // Level-driven resolver used by CH4/CH5 and other non-latched outputs:
    // output follows (wantOnMask || manualWant) while it is enabled and not forbidden.
    void applyResolved(uint32_t forbidMask, uint32_t wantOnMask) {
        _forbidMask = forbidMask;
        _wantOnMask = wantOnMask;
        _recomputeRequestedLevel();
        _syncPhysicalNow();
    }

    // Stateful resolver used by CH1..CH3. It matches the channel-control chart:
    // OFF has priority, then ON, then a new manual command; in the neutral zone
    // the relay keeps its current physical state.
    void applyResolvedHold(uint32_t forbidMask, uint32_t wantOnMask) {
        _forbidMask = forbidMask;
        _wantOnMask = wantOnMask;

        if (!enabled) {
            _manualWant = false;
            _requestedOn = false;
        } else if (_forbidMask != 0) {
            // A manual ON request is a command, not a persistent auto-restart latch.
            // Once any OFF/forbid condition has turned the output off, the old
            // manual request must not turn the relay on again when the forbid clears.
            _manualWant = false;
            _requestedOn = false;
        } else if (_wantOnMask != 0) {
            _requestedOn = true;
        } else {
            // Neutral hysteresis band: hold the last actual relay state.
            _requestedOn = _actualOn;
        }

        _syncPhysicalNow();
    }

    // Legacy level-driven manual request. Kept for CH4/CH5 compatibility.
    bool setManual(bool on) {
        if (on && (_forbidMask != 0 || !enabled) && !_manualWant) {
            return false;
        }

        _manualWant = on;
        _recomputeRequestedLevel();
        _syncPhysicalNow();

        return !on || (_forbidMask == 0 && enabled);
    }

    // Manual command for stateful CH1..CH3: apply once, then the neutral zone
    // keeps the physical state. A later forbid clears _manualWant.
    bool setManualHold(bool on) {
        if (on && (_forbidMask != 0 || !enabled)) {
            return false;
        }

        _manualWant = on;
        if (!on && enabled && _forbidMask == 0 && _wantOnMask != 0) {
            // Automatic ON has priority over manual OFF in the chart.
            _requestedOn = true;
        } else {
            _requestedOn = enabled && (_forbidMask == 0) && on;
            if (!on) _requestedOn = false;
        }
        _syncPhysicalNow();

        return !on || (_forbidMask == 0 && enabled);
    }

    void forceOff(bool clearManual = true) {
        if (clearManual) _manualWant = false;
        _requestedOn = false;
        _syncPhysicalNow();
    }

    void restoreManualWant(bool on) {
        _manualWant = on;
        _recomputeRequestedLevel();
    }

    void beginCommand(RelayCommand cmd) {
        _cmd = cmd;
        _cmdSentAt = millis();
    }

    void clearCommand() {
        _cmd = CMD_NONE;
        _cmdSentAt = 0;
    }

    void clearTransientOverrides() {
        _pulseActive = false;
        _pulseUntilMs = 0;
        _bellPatternActive = false;
        _bellPhaseOn = false;
        _bellPhaseStartedMs = 0;
    }

    RelayCommand command() const { return _cmd; }
    bool commandPending() const { return _cmd != CMD_NONE; }
    uint32_t commandSentAt() const { return _cmdSentAt; }
    bool commandTargetOn() const { return _cmd == CMD_ON; }

    void requestPulse(uint32_t durationMs) {
        if (!enabled || durationMs == 0) return;
        _pulseActive = true;
        _pulseUntilMs = millis() + durationMs;
        _applyPhysical(_resolvePhysicalRequest(true));
    }

    void setBellPatternActive(bool active) {
        if (_bellPatternActive == active) {
            if (!active) _syncPhysicalNow();
            return;
        }

        _bellPatternActive = active;
        if (active) {
            _bellPhaseOn = true;
            _bellPhaseStartedMs = millis();
            _applyPhysical(_resolvePhysicalRequest(true));
        } else {
            _syncPhysicalNow();
        }
    }

    bool loop() {
        bool changed = false;

        if (_pulseActive) {
            const uint32_t now = millis();
            if ((int32_t)(now - _pulseUntilMs) >= 0) {
                _pulseActive = false;
                if (_bellPatternActive) changed |= _applyPhysical(_resolvePhysicalRequest(_bellPhaseOn));
                else changed |= _applyPhysical(_resolvePhysicalRequest(_requestedOn));
            } else {
                changed |= _applyPhysical(_resolvePhysicalRequest(true));
                return changed;
            }
        }

        if (!_bellPatternActive) return changed;

        const uint32_t now = millis();
        if (_bellPhaseOn) {
            if (now - _bellPhaseStartedMs >= BELL_ON_MS) {
                _bellPhaseOn = false;
                _bellPhaseStartedMs = now;
                return changed | _applyPhysical(_resolvePhysicalRequest(false));
            }
        } else {
            if (now - _bellPhaseStartedMs >= BELL_OFF_MS) {
                _bellPhaseOn = true;
                _bellPhaseStartedMs = now;
                return changed | _applyPhysical(_resolvePhysicalRequest(true));
            }
        }
        return changed;
    }

    bool isOn() const { return _actualOn; }
    bool actualOn() const { return _actualOn; }
    bool requestedOn() const { return _requestedOn; }
    bool finalRequestedOn() const { return _resolvePhysicalRequest(_requestedOn); }
    bool manualWant() const { return _manualWant; }
    bool isBellPatternActive() const { return _bellPatternActive; }
    bool finalOnAllowed() const { return _finalOnAllowed; }

    uint32_t forbidMask() const { return _forbidMask; }
    uint32_t wantOnMask() const { return _wantOnMask; }
    bool forbidden() const { return _forbidMask != 0; }

    void setFinalOnAllowed(bool allowed) {
        _finalOnAllowed = allowed;
        _syncPhysicalNow();
    }

private:
    bool     _actualOn  = false;
    bool     _requestedOn = false;
    bool     _manualWant = false;
    uint32_t _forbidMask = 0;
    uint32_t _wantOnMask = 0;

    RelayCommand _cmd = CMD_NONE;
    uint32_t _cmdSentAt = 0;

    bool     _bellPatternActive = false;
    bool     _bellPhaseOn = false;
    uint32_t _bellPhaseStartedMs = 0;

    bool     _pulseActive = false;
    uint32_t _pulseUntilMs = 0;
    bool     _finalOnAllowed = true;

    void _recomputeRequestedLevel() {
        const bool canBeOn = enabled && (_forbidMask == 0);
        const bool wantsOn = (_wantOnMask != 0) || _manualWant;
        _requestedOn = canBeOn && wantsOn;
    }

    void _syncPhysicalNow() {
        if (_pulseActive) return;
        if (_bellPatternActive) return;
        _applyPhysical(_resolvePhysicalRequest(_requestedOn));
    }

    bool _resolvePhysicalRequest(bool on) const {
        return on && _finalOnAllowed;
    }

    bool _applyPhysical(bool on) {
        if (_actualOn == on) return false;
        _actualOn = on;
        _write(on);
        return true;
    }

    void _write(bool on) {
        digitalWrite(pin, inverted ? !on : on);
    }
};
