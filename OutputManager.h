// ================================================================
// OutputManager.h 
// ================================================================
#pragma once
#include <math.h>
#include <string.h>
#include "Output.h"
#include "SensorManager.h"
#include "EventLog.h"
#include "config.h"

enum RelayCommandError : uint8_t {
    RELAY_CMDERR_NONE = 0,
    RELAY_CMDERR_BLOCKED = 1,
    RELAY_CMDERR_TIMEOUT = 2,
};

struct RelayCommandResult {
    bool accepted = false;
    const char* reason = "";
    const char* detail = "";
};

class OutputManager {
public:
    Output* out[OUT_COUNT];
    bool    soundMuted = false;
    uint8_t chMode[3] = { LOGIC_HEAT, LOGIC_HEAT, LOGIC_HEAT };
    bool    ch4Enabled = true;
    bool    ch5Enabled = true;

    OutputManager() {
        out[OUT_CH1] = new Output("CH1", PIN_CH1);
        out[OUT_CH2] = new Output("CH2", PIN_CH2);
        out[OUT_CH3] = new Output("CH3", PIN_CH3);
        out[OUT_CH4] = new Output("CH4", PIN_CH4);
        out[OUT_CH5] = new Output("CH5", PIN_CH5);

        for (int i = 0; i < OUT_COUNT; i++) {
            _lastForbid[i] = 0;
            _lastWant[i]   = 0;
            _operatorHoldOff[i] = false;
            _safetyForbid[i] = 0;
            _lastCmdError[i] = RELAY_CMDERR_NONE;
            _lastCmdErrorMs[i] = 0;
            _lastCmdDetail[i] = "";
            _cmdPrevManual[i] = false;
            _cmdPrevHoldOff[i] = false;
        }
    }

    void begin() {
        out[OUT_CH4]->enabled = ch4Enabled;
        out[OUT_CH5]->enabled = ch5Enabled;
        for (int i = 0; i < OUT_COUNT; i++) out[i]->begin();
        _begun = true;

        // Legacy persisted manual latches must not restart CH1..CH3 after
        // reboot or after an automatic OFF condition. Manual commands are
        // applied as one-shot state changes; neutral cycles then hold actualOn.
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            if (_operatorHoldOff[oi]) {
                _operatorHoldOff[oi] = false;
                _manualStateDirty = true;
            }
            if (out[oi]->manualWant()) {
                out[oi]->restoreManualWant(false);
                _manualStateDirty = true;
            }
        }

        for (int i = 0; i < OUT_COUNT; i++) _applyCurrent(i);
    }

    uint32_t loop(SensorManager& sm) {
        uint32_t changed = 0;
        bool prevState[OUT_COUNT];
        for (int i = 0; i < OUT_COUNT; i++) prevState[i] = out[i]->isOn();
        _syncRuntimeState(sm, prevState);

        for (int i = 0; i < OUT_COUNT; i++) {
            if (out[i]->loop()) changed |= (1u << i);
        }

        for (int i = 0; i < OUT_COUNT; i++) {
            if (out[i]->isOn() != prevState[i]) changed |= (1u << i);
        }
        return changed;
    }

    void syncRuntimeState(SensorManager& sm) {
        bool prevState[OUT_COUNT];
        for (int i = 0; i < OUT_COUNT; i++) prevState[i] = out[i]->isOn();
        _syncRuntimeState(sm, prevState);
    }

    void beepAcceptedCommand(uint32_t durationMs = CMD_BEEP_MS) {
        const uint32_t now = millis();
        if (now - _lastBeepMs < CMD_BEEP_COOLDOWN_MS) return;
        if (!ch5Enabled || soundMuted) return;
        _lastBeepMs = now;
        out[OUT_CH5]->requestPulse(durationMs);
    }

    // Low-level manual request. For CH1..CH3 it is routed through the same
    // one-shot relay command path as the API, so the flowchart priorities are
    // enforced независимо от источника команды.
    bool setManual(uint8_t outIdx, bool on) {
        if (outIdx >= OUT_COUNT || !out[outIdx]) return false;
        if (_isMainOutput(outIdx)) {
            RelayCommandResult r = handleRelayCommand(outIdx, on ? CMD_ON : CMD_OFF);
            return r.accepted;
        }
        const bool prevManual = out[outIdx]->manualWant();
        const bool accepted = _setManualForOutput(outIdx, on);
        if (out[outIdx]->manualWant() != prevManual) {
            _manualStateDirty = true;
        }
        return accepted;
    }

    // Новый операторский цикл: команда хранится только до аппаратного
    // подтверждения или таймаута. Если включение запрещено, команда сразу
    // сбрасывается и не "висит" до исчезновения блокировки.
    RelayCommandResult handleRelayCommand(uint8_t outIdx, RelayCommand cmd,
                                           EventLog* log = nullptr,
                                           SensorManager* sm = nullptr)
    {
        RelayCommandResult result;
        if (outIdx >= OUT_COUNT || !out[outIdx] || cmd == CMD_NONE) {
            result.reason = "invalid_command";
            result.detail = "invalid_command";
            return result;
        }

        if (sm) {
            syncRuntimeState(*sm);
        }

        const bool werRequired = requiresWerConfirmation(outIdx);
        if (!werRequired && out[outIdx]->commandPending()) {
            out[outIdx]->clearCommand();
        }

        if (werRequired && out[outIdx]->commandPending()) {
            result.reason = (out[outIdx]->command() == cmd) ? "duplicate" : "busy";
            result.detail = result.reason;
            return result;
        }

        const bool targetOn = (cmd == CMD_ON);
        _lastCmdError[outIdx] = RELAY_CMDERR_NONE;
        _lastCmdErrorMs[outIdx] = 0;
        _lastCmdDetail[outIdx] = "";

        const char* blockReason = nullptr;
        if (!_relayCommandAllowed(outIdx, targetOn, blockReason)) {
            if (werRequired) out[outIdx]->clearCommand();
            _lastCmdError[outIdx] = RELAY_CMDERR_BLOCKED;
            _lastCmdErrorMs[outIdx] = millis();
            result.reason = "blocked";
            result.detail = blockReason ? blockReason : "blocked";
            _lastCmdDetail[outIdx] = result.detail;
            if (log) _logRelayCommand(log, sm, outIdx, cmd, false, result.detail);
            return result;
        }

        if (werRequired) {
            _cmdPrevManual[outIdx] = out[outIdx]->manualWant();
            _cmdPrevHoldOff[outIdx] = _operatorHoldOff[outIdx];
            out[outIdx]->beginCommand(cmd);
        } else if (out[outIdx]->commandPending()) {
            out[outIdx]->clearCommand();
        }

        _applyRelayCommand(outIdx, targetOn);
        if (log) _logRelayCommand(log, sm, outIdx, cmd, true, "");
        result.accepted = true;
        return result;
    }

    // Backward-compatible wrapper for the old manual endpoint.
    bool setOperatorManual(uint8_t outIdx, bool on) {
        RelayCommandResult r = handleRelayCommand(outIdx, on ? CMD_ON : CMD_OFF);
        return r.accepted;
    }

    void updateRelayCommandFeedback(const bool feedbackOn[OUT_COUNT],
                                    const bool feedbackAvailable[OUT_COUNT],
                                    EventLog* log,
                                    SensorManager* sm)
    {
        const uint32_t now = millis();
        for (uint8_t oi = 0; oi < OUT_COUNT; oi++) {
            if (!out[oi] || !out[oi]->commandPending()) continue;
            if (!requiresWerConfirmation(oi)) {
                out[oi]->clearCommand();
                _lastCmdError[oi] = RELAY_CMDERR_NONE;
                _lastCmdErrorMs[oi] = 0;
                _lastCmdDetail[oi] = "";
                continue;
            }

            const RelayCommand cmd = out[oi]->command();
            const bool targetOn = (cmd == CMD_ON);
            const bool actualFeedback = feedbackAvailable[oi] ? feedbackOn[oi] : out[oi]->actualOn();

            if (actualFeedback == targetOn) {
                if (_isMainOutput(oi) && out[oi]->manualWant()) {
                    out[oi]->restoreManualWant(false);
                    _manualStateDirty = true;
                }
                out[oi]->clearCommand();
                _lastCmdError[oi] = RELAY_CMDERR_NONE;
                _lastCmdErrorMs[oi] = 0;
                _lastCmdDetail[oi] = "";
                continue;
            }

            if (now - out[oi]->commandSentAt() >= _relayConfirmTimeoutMs(oi)) {
                if (cmd == CMD_ON) {
                    // Do not leave a delayed ON request after a failed confirmation.
                    // For stateful CH1..CH3, neutral hold would otherwise keep the
                    // physical request ON, so force the relay off explicitly.
                    out[oi]->restoreManualWant(_cmdPrevManual[oi]);
                    _operatorHoldOff[oi] = _cmdPrevHoldOff[oi];
                    out[oi]->forceOff(true);
                    _manualStateDirty = true;
                }

                out[oi]->clearCommand();
                _lastCmdError[oi] = RELAY_CMDERR_TIMEOUT;
                _lastCmdErrorMs[oi] = now;
                _lastCmdDetail[oi] = "timeout";
                if (log) _logRelayCommand(log, sm, oi, cmd, false, "timeout");
            }
        }
    }

    bool relayCommandPending(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT && out[outIdx]) ? out[outIdx]->commandPending() : false;
    }

    RelayCommand relayCommand(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT && out[outIdx]) ? out[outIdx]->command() : CMD_NONE;
    }

    RelayCommandError relayCommandError(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? (RelayCommandError)_lastCmdError[outIdx] : RELAY_CMDERR_NONE;
    }

    uint32_t relayCommandErrorMs(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _lastCmdErrorMs[outIdx] : 0;
    }

    const char* relayCommandErrorName(uint8_t outIdx) const {
        switch (relayCommandError(outIdx)) {
            case RELAY_CMDERR_BLOCKED: return "blocked";
            case RELAY_CMDERR_TIMEOUT: return "timeout";
            default: return "";
        }
    }

    bool setMainStopLatched(bool active) {
        const bool changed = (_mainStopLatched != active);
        _mainStopLatched = active;

        if (active) {
            _applyGlobalStop();
        } else {
            for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
                _applyCurrent(oi);
            }
        }

        if (changed) _manualStateDirty = true;
        return true;
    }

    bool mainStopLatched() const { return _mainStopLatched; }

    bool operatorHoldOff(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _operatorHoldOff[outIdx] : false;
    }

    bool manualWanted(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT && out[outIdx]) ? out[outIdx]->manualWant() : false;
    }

    void restoreManualState(uint8_t outIdx, bool on) {
        if (outIdx >= OUT_COUNT || !out[outIdx]) return;
        out[outIdx]->restoreManualWant(on);
    }

    void restoreOperatorHoldOff(uint8_t outIdx, bool active) {
        if (outIdx >= OUT_COUNT) return;
        _operatorHoldOff[outIdx] = active;
    }

    void restoreMainStopLatched(bool active) {
        _mainStopLatched = active;
    }

    bool consumeManualStateDirty() {
        const bool dirty = _manualStateDirty;
        _manualStateDirty = false;
        return dirty;
    }

    void mute(bool m) {
        soundMuted = m;
        if (m) {
            out[OUT_CH4]->setBellPatternActive(false);
            uint32_t want = _lastWant[OUT_CH5] & ~(1u << RULEIDX_SOUND);
            out[OUT_CH5]->applyResolved(_effectiveForbidMask(OUT_CH5), want);
        }
    }

    void applyConfig() {
        out[OUT_CH4]->enabled = ch4Enabled;
        out[OUT_CH5]->enabled = ch5Enabled;
        if (!_begun) return;
        if (!ch4Enabled) out[OUT_CH4]->setBellPatternActive(false);
        if (!ch5Enabled) {
            uint32_t want = _lastWant[OUT_CH5] & ~(1u << RULEIDX_SOUND);
            out[OUT_CH5]->applyResolved(_effectiveForbidMask(OUT_CH5), want);
        }
        for (int i = 0; i < OUT_COUNT; i++) _applyCurrent(i);
    }

    uint32_t lastForbidMask(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _lastForbid[outIdx] : 0;
    }

    uint32_t lastWantMask(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _lastWant[outIdx] : 0;
    }

    uint32_t effectiveForbidMask(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _effectiveForbidMask(outIdx) : 0;
    }

    bool autoOffActive(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? (_lastForbid[outIdx] != 0) : false;
    }

    bool safetyBlocked(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? ((_operatorForbidMask(outIdx) | _safetyForbid[outIdx]) != 0) : false;
    }

    bool controlSensorAutoOff(uint8_t outIdx, uint8_t sensorIdx) const {
        if (outIdx >= OUT_COUNT || sensorIdx >= SEN_COUNT) return false;
        return (_lastForbid[outIdx] & (1u << sensorIdx)) != 0;
    }

    bool controlSensorAutoOn(uint8_t outIdx, uint8_t sensorIdx) const {
        if (outIdx >= OUT_COUNT || sensorIdx >= SEN_COUNT) return false;
        return (_lastWant[outIdx] & (1u << sensorIdx)) != 0;
    }

    bool manualRequestOn(uint8_t outIdx) const {
        if (outIdx >= OUT_COUNT || !out[outIdx]) return false;
        return out[outIdx]->manualWant() ||
               (out[outIdx]->commandPending() && out[outIdx]->command() == CMD_ON);
    }

    String relayBlockDetailText(uint8_t outIdx, const char* reason) const {
        if (!reason || !reason[0]) return "";
        if (strcmp(reason, "stop_active") == 0) {
            return "активен STOP: ручное включение CH1-CH3 заблокировано";
        }
        if (strcmp(reason, "forbidden") == 0) {
            const String reasons = formatForbidReasons(_effectiveForbidMask(outIdx));
            if (reasons.length() > 0) {
                return String("действуют запреты автоматики: ") + reasons;
            }
            return "действуют запреты автоматики";
        }
        if (strcmp(reason, "disabled") == 0) {
            return "выход отключён в конфигурации";
        }
        if (strcmp(reason, "auto_on_active") == 0) {
            const String reasons = formatWantReasons(_lastWant[outIdx]);
            if (reasons.length() > 0) {
                return String("автоматика требует удерживать канал включённым: ") + reasons;
            }
            return "автоматика требует удерживать канал включённым";
        }
        if (strcmp(reason, "busy") == 0) {
            return "предыдущая команда ещё ожидает подтверждения";
        }
        if (strcmp(reason, "duplicate") == 0) {
            return "такая же команда уже выполняется";
        }
        if (strcmp(reason, "invalid_output") == 0) {
            return "указан недопустимый выход";
        }
        if (strcmp(reason, "blocked") == 0) {
            return "команда заблокирована";
        }
        if (strcmp(reason, "timeout") == 0) {
            return "таймаут подтверждения реле";
        }
        return String(reason);
    }

    String relayTimeoutDetailText(uint8_t outIdx) const {
        if (outIdx >= OUT_COUNT || !out[outIdx]) return "таймаут подтверждения реле";
        if (!requiresWerConfirmation(outIdx)) {
            return String("канал ") + out[outIdx]->name +
                   " не использует WER-подтверждение";
        }
        return String("таймаут подтверждения реле ") + out[outIdx]->name +
               ": ожидается сигнал " + _confirmationId(outIdx);
    }

    String relayErrorText(uint8_t outIdx) const {
        switch (relayCommandError(outIdx)) {
            case RELAY_CMDERR_BLOCKED:
                return relayBlockDetailText(outIdx, _lastCmdDetail[outIdx]);
            case RELAY_CMDERR_TIMEOUT:
                return relayTimeoutDetailText(outIdx);
            default:
                return "";
        }
    }

    String formatForbidReasons(uint32_t mask) const {
        String outText;
        for (uint8_t si = 0; si < SEN_COUNT; si++) {
            if (!(mask & (1u << si))) continue;
            _appendReason(outText, _sensorReasonName(si));
        }
        if (mask & (1u << RULEIDX_STOP)) _appendReason(outText, "STOP");
        if (mask & (1u << RULEIDX_SAFETY_LEVEL)) _appendReason(outText, "авария уровня");
        if (mask & (1u << RULEIDX_SAFETY_FLOW)) _appendReason(outText, "авария потока");
        if (mask & (1u << RULEIDX_SAFETY_PRESSURE)) _appendReason(outText, "авария давления");
        if (mask & (1u << RULEIDX_SAFETY_WER)) _appendReason(outText, "авария подтверждения WER");
        return outText;
    }

    String formatWantReasons(uint32_t mask) const {
        String outText;
        for (uint8_t si = 0; si < SEN_COUNT; si++) {
            if (!(mask & (1u << si))) continue;
            _appendReason(outText, _sensorReasonName(si));
        }
        return outText;
    }

    void acknowledgeCurrentAlarms(const SensorManager& sm) {
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            const uint8_t active = s ? s->alarmMask() : 0;
            _ackedAlarmMask[si] |= active;
        }
    }

    uint16_t activeAlarmCount(const SensorManager& sm) const {
        uint16_t count = 0;
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;
            const uint8_t mask = s->alarmMask();
            for (uint8_t ai = 0; ai < N_ALARMS; ai++) if (mask & (1u << ai)) count++;
        }
        return count;
    }

    uint16_t unackedAlarmCount(const SensorManager& sm) {
        _pruneAcknowledged(sm);
        uint16_t count = 0;
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;
            const uint8_t mask = (uint8_t)(s->alarmMask() & (uint8_t)(~_ackedAlarmMask[si]));
            for (uint8_t ai = 0; ai < N_ALARMS; ai++) if (mask & (1u << ai)) count++;
        }
        return count;
    }

    bool hasUnackedAlarms(const SensorManager& sm) {
        _pruneAcknowledged(sm);
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;
            const uint8_t active = s->alarmMask();
            if ((active & (uint8_t)(~_ackedAlarmMask[si])) != 0) return true;
        }
        return false;
    }

    uint8_t unackedAlarmMaskFor(const SensorManager& sm, uint8_t sensorIdx) {
        _pruneAcknowledged(sm);
        if (sensorIdx >= SEN_COUNT) return 0;
        SensorBase* s = sm.s[sensorIdx];
        if (!s) return 0;
        return (uint8_t)(s->alarmMask() & (uint8_t)(~_ackedAlarmMask[sensorIdx]));
    }

    void setSafetyForbid(uint8_t outIdx, uint8_t ruleIdx, bool active) {
        if (outIdx >= OUT_COUNT || ruleIdx >= 32 || !out[outIdx]) return;
        const uint32_t bit = (1u << ruleIdx);
        const uint32_t prev = _safetyForbid[outIdx];
        if (active) _safetyForbid[outIdx] |= bit;
        else        _safetyForbid[outIdx] &= ~bit;
        if (_safetyForbid[outIdx] == prev) return;

        if (active) {
            if (out[outIdx]->manualWant()) {
                out[outIdx]->restoreManualWant(false);
                _manualStateDirty = true;
            }
            if (out[outIdx]->commandPending()) out[outIdx]->clearCommand();
        }
        _applyCurrent(outIdx);
    }

    void clearSafetyForbid(uint8_t outIdx, uint8_t ruleIdx) {
        setSafetyForbid(outIdx, ruleIdx, false);
    }

    uint32_t safetyForbidMask(uint8_t outIdx) const {
        return (outIdx < OUT_COUNT) ? _safetyForbid[outIdx] : 0;
    }

    void setSafetyAlarmActive(bool active) {
        _safetyAlarmActive = active;
    }

    bool safetyAlarmActive() const { return _safetyAlarmActive; }

private:
    static bool _isMainOutput(uint8_t outIdx) {
        return requiresWerConfirmation(outIdx);
    }

    int _sensorCommandForOutput(const SensorManager& sm, const bool prevState[OUT_COUNT],
                                uint8_t sensorIdx, uint8_t outIdx)
    {
        SensorBase* sen = sm.s[sensorIdx];
        if (!sen) return 0;

        bool invalidMeansOff = true;
        bool controlGate = true;
        if (_isMainOutput(outIdx) && SensorManager::isSchemeControlSensorIndex(sensorIdx)) {
            // For CH1..CH3 an enabled control sensor in error/absent/NAN state
            // must form AUTO OFF and block manual ON until the fault clears.
            invalidMeansOff = true;
            if (sensorIdx == SEN_F) {
                controlGate = _flowControlGate(prevState, outIdx);
            }
        }

        return sen->evalCtrl(outIdx, invalidMeansOff, controlGate);
    }

    bool _flowControlGate(const bool prevState[OUT_COUNT], uint8_t outIdx) const {
        if (outIdx == OUT_CH1) {
            // Source scheme explicitly says CH1 flow loss is relevant only when
            // the linked relay CH2 (valve) is already ON.
            return prevState[OUT_CH2];
        }

        // CH2/CH3 have no explicit linked-relay mapping in the source scheme.
        // Keep the current per-channel gate until the project gains a dedicated
        // configuration for their flow dependency.
        return prevState[outIdx];
    }

    void _applyGlobalStop() {
        bool dirty = false;

        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            _lastForbid[oi] = 0;
            _lastWant[oi] = 0;

            if (_operatorHoldOff[oi]) {
                _operatorHoldOff[oi] = false;
                dirty = true;
            }

            if (out[oi]->manualWant()) {
                out[oi]->restoreManualWant(false);
                dirty = true;
            }

            if (out[oi]->commandPending()) out[oi]->clearCommand();
            out[oi]->clearTransientOverrides();
            _lastCmdError[oi] = RELAY_CMDERR_NONE;
            _lastCmdErrorMs[oi] = 0;
            _lastCmdDetail[oi] = "";

            if (!_begun) continue;

            out[oi]->setFinalOnAllowed(_effectiveForbidMask(oi) == 0);
            out[oi]->applyResolvedHold(_effectiveForbidMask(oi), 0);
        }

        if (dirty) _manualStateDirty = true;
    }

    bool _relayCommandAllowed(uint8_t outIdx, bool targetOn, const char*& reason) const {
        reason = "";
        if (outIdx >= OUT_COUNT || !out[outIdx]) {
            reason = "invalid_output";
            return false;
        }
        if (targetOn && !out[outIdx]->enabled) {
            reason = "disabled";
            return false;
        }
        if (targetOn && _isMainOutput(outIdx) && _mainStopLatched) {
            reason = "stop_active";
            return false;
        }
        if (targetOn && (_effectiveForbidMask(outIdx) != 0)) {
            reason = "forbidden";
            return false;
        }
        if (!targetOn && _isMainOutput(outIdx) && _lastWant[outIdx] != 0 &&
            _effectiveForbidMask(outIdx) == 0) {
            reason = "auto_on_active";
            return false;
        }
        return true;
    }

    void _applyRelayCommand(uint8_t outIdx, bool targetOn) {
        if (_isMainOutput(outIdx)) {
            bool dirty = false;

            const bool prevManual = out[outIdx]->manualWant();
            const bool prevHoldOff = _operatorHoldOff[outIdx];

            if (targetOn) {
                _operatorHoldOff[outIdx] = false;
                out[outIdx]->restoreManualWant(true);
            } else {
                out[outIdx]->restoreManualWant(false);
                _operatorHoldOff[outIdx] = true;
            }

            _applyCurrent(outIdx);

            if (out[outIdx]->manualWant() != prevManual || _operatorHoldOff[outIdx] != prevHoldOff) {
                dirty = true;
            }
            if (dirty) _manualStateDirty = true;
            return;
        }

        bool dirty = false;

        if (targetOn) {
            if (_isMainOutput(outIdx) && _operatorHoldOff[outIdx]) {
                _operatorHoldOff[outIdx] = false;
                dirty = true;
                _applyCurrent(outIdx);

                // Если автоматика уже просит включить канал, команда ON означает
                // "разрешить управление автоматике", а не липкий manual ON.
                if (_lastWant[outIdx] != 0 && !out[outIdx]->manualWant()) {
                    if (dirty) _manualStateDirty = true;
                    return;
                }
            }

            const bool prevManual = out[outIdx]->manualWant();
            _setManualForOutput(outIdx, true);
            if (out[outIdx]->manualWant() != prevManual) dirty = true;
        } else {
            const bool prevManual = out[outIdx]->manualWant();
            _setManualForOutput(outIdx, false);
            if (out[outIdx]->manualWant() != prevManual) dirty = true;

            if (_isMainOutput(outIdx) && _operatorHoldOff[outIdx]) {
                _operatorHoldOff[outIdx] = false;
                dirty = true;
            }

            _applyCurrent(outIdx);
        }

        if (dirty) _manualStateDirty = true;
    }

    uint32_t _operatorForbidMask(uint8_t outIdx) const {
        uint32_t mask = 0;
        if (_isMainOutput(outIdx)) {
            if (_mainStopLatched)         mask |= (1u << RULEIDX_STOP);
        }
        return mask;
    }

    uint32_t _effectiveForbidMask(uint8_t outIdx) const {
        if (outIdx >= OUT_COUNT) return 0;
        return _lastForbid[outIdx] | _operatorForbidMask(outIdx) | _safetyForbid[outIdx];
    }

    bool _setManualForOutput(uint8_t outIdx, bool on) {
        if (outIdx >= OUT_COUNT || !out[outIdx]) return false;
        if (_isMainOutput(outIdx)) return out[outIdx]->setManualHold(on);
        return out[outIdx]->setManual(on);
    }

    void _applyCurrent(uint8_t outIdx) {
        if (!_begun || outIdx >= OUT_COUNT || !out[outIdx]) return;
        if (_isMainOutput(outIdx)) {
            _applyCurrentMain(outIdx);
            return;
        }
        out[outIdx]->setFinalOnAllowed(true);
        const bool prevManual = out[outIdx]->manualWant();
        out[outIdx]->applyResolved(_effectiveForbidMask(outIdx), _lastWant[outIdx]);
        if (out[outIdx]->manualWant() != prevManual) _manualStateDirty = true;
    }

    void _applyCurrentMain(uint8_t outIdx) {
        const bool prevManual = out[outIdx]->manualWant();
        const bool prevHoldOff = _operatorHoldOff[outIdx];
        const uint32_t forbidMask = _effectiveForbidMask(outIdx);
        const uint32_t wantMask = _lastWant[outIdx];
        out[outIdx]->setFinalOnAllowed(forbidMask == 0);

        if (forbidMask != 0) {
            if (out[outIdx]->commandPending()) {
                out[outIdx]->clearCommand();
                _lastCmdError[outIdx] = RELAY_CMDERR_NONE;
                _lastCmdErrorMs[outIdx] = 0;
                _lastCmdDetail[outIdx] = "";
            }
            _operatorHoldOff[outIdx] = false;
            if (out[outIdx]->manualWant()) out[outIdx]->restoreManualWant(false);
            out[outIdx]->applyResolvedHold(forbidMask, wantMask);
        } else if (wantMask != 0) {
            _operatorHoldOff[outIdx] = false;
            if (out[outIdx]->manualWant()) out[outIdx]->restoreManualWant(false);
            out[outIdx]->applyResolvedHold(0, wantMask);
        } else if (_operatorHoldOff[outIdx]) {
            _operatorHoldOff[outIdx] = false;
            if (out[outIdx]->manualWant()) out[outIdx]->restoreManualWant(false);
            out[outIdx]->forceOff(true);
            out[outIdx]->applyResolvedHold(0, 0);
        } else if (out[outIdx]->manualWant()) {
            out[outIdx]->setManualHold(true);
            out[outIdx]->restoreManualWant(false);
            out[outIdx]->applyResolvedHold(0, 0);
        } else {
            out[outIdx]->applyResolvedHold(0, 0);
        }

        if (out[outIdx]->manualWant() != prevManual || _operatorHoldOff[outIdx] != prevHoldOff) {
            _manualStateDirty = true;
        }
    }

    void _syncRuntimeState(SensorManager& sm, const bool prevState[OUT_COUNT]) {
        uint32_t newForbid[OUT_COUNT] = {};
        uint32_t newWant[OUT_COUNT]   = {};

        sm.normalizeDigitalOffOnlyRules();

        out[OUT_CH4]->enabled = ch4Enabled;
        out[OUT_CH5]->enabled = ch5Enabled;

        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* sen = sm.s[si];
            if (!sen) continue;

            for (int oi = 0; oi < OUT_COUNT; oi++) {
                // STOP is a top-level short-circuit only for the technological
                // channels CH1..CH3. Auxiliary outputs keep their own logic.
                if (_mainStopLatched && _isMainOutput((uint8_t)oi)) continue;

                const int cmd = _sensorCommandForOutput(sm, prevState, (uint8_t)si, (uint8_t)oi);
                if (cmd == 1) newWant[oi] |= (1u << si);
                else if (cmd == -1) newForbid[oi] |= (1u << si);
            }
        }

        const bool anyUnackedAlarm = hasUnackedAlarms(sm);
        const bool soundRequired = anyUnackedAlarm || _safetyAlarmActive;

        if (ch5Enabled && !soundMuted && soundRequired) {
            newWant[OUT_CH5] |= (1u << RULEIDX_SOUND);
        }

        if (_mainStopLatched) {
            _applyGlobalStop();
        }

        for (int oi = 0; oi < OUT_COUNT; oi++) {
            if (_mainStopLatched && _isMainOutput((uint8_t)oi)) continue;
            _lastForbid[oi] = newForbid[oi];
            _lastWant[oi]   = newWant[oi];
            _applyCurrent((uint8_t)oi);
        }

        out[OUT_CH4]->setBellPatternActive(ch4Enabled && !soundMuted && soundRequired);
    }

    void _logRelayCommand(EventLog* log, SensorManager* sm, uint8_t outIdx,
                          RelayCommand cmd, bool accepted, const char* detail)
    {
        if (!log) return;
        String msg = String("Команда реле: ") + out[outIdx]->name + " " +
                     (cmd == CMD_ON ? "ВКЛ" : "ВЫКЛ");
        if (accepted) {
            msg += " принята";
        } else if (detail && strcmp(detail, "timeout") == 0) {
            msg += " не подтверждена: " + relayTimeoutDetailText(outIdx);
        } else {
            const String why = relayBlockDetailText(outIdx, detail);
            msg += " отклонена";
            if (why.length() > 0) msg += ": " + why;
        }
        log->add(msg,
                 sm ? sm->getT1() : NAN,
                 sm ? sm->getT2() : NAN,
                 sm ? sm->getT3() : NAN,
                 sm ? sm->getDT() : NAN);
    }

    static void _appendReason(String& dst, const char* text) {
        if (!text || !text[0]) return;
        if (dst.length() > 0) dst += ", ";
        dst += text;
    }

    static const char* _sensorReasonName(uint8_t sensorIdx) {
        switch (sensorIdx) {
            case SEN_T1: return "датчик T1";
            case SEN_T2: return "датчик T2";
            case SEN_T3: return "датчик T3";
            case SEN_DT: return "датчик dT";
            case SEN_P:  return "датчик давления";
            case SEN_L:  return "датчик уровня";
            case SEN_F:  return "датчик протока";
            case SEN_C:  return "датчик тока";
            case SEN_V:  return "датчик V";
            default:     return "датчик";
        }
    }

    static const char* _confirmationId(uint8_t outIdx) {
        switch (outIdx) {
            case OUT_CH1: return "WER_CH1";
            case OUT_CH2: return "WER_CH2";
            case OUT_CH3: return "WER_CH3";
            default:      return "WER";
        }
    }

    static uint32_t _relayConfirmTimeoutMs(uint8_t outIdx) {
        if (!requiresWerConfirmation(outIdx)) return 0;
        switch (outIdx) {
            case OUT_CH1: return RELAY_CONFIRM_TIMEOUT_CH1_MS;
            case OUT_CH2: return RELAY_CONFIRM_TIMEOUT_CH2_MS;
            case OUT_CH3: return RELAY_CONFIRM_TIMEOUT_CH3_MS;
            default:      return RELAY_CONFIRM_TIMEOUT_MS;
        }
    }

    void _pruneAcknowledged(const SensorManager& sm) {
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            const uint8_t active = s ? s->alarmMask() : 0;
            _ackedAlarmMask[si] &= active;
        }
    }

    uint32_t _lastForbid[OUT_COUNT];
    uint32_t _lastWant[OUT_COUNT];
    uint32_t _safetyForbid[OUT_COUNT] = {};
    uint8_t  _ackedAlarmMask[SEN_COUNT] = {};
    uint32_t _lastBeepMs = 0;
    bool     _manualStateDirty = false;
    bool     _begun = false;
    bool     _mainStopLatched = false;
    bool     _safetyAlarmActive = false;
    bool     _operatorHoldOff[OUT_COUNT] = {};
    bool     _cmdPrevManual[OUT_COUNT] = {};
    bool     _cmdPrevHoldOff[OUT_COUNT] = {};
    uint8_t  _lastCmdError[OUT_COUNT] = {};
    uint32_t _lastCmdErrorMs[OUT_COUNT] = {};
    const char* _lastCmdDetail[OUT_COUNT] = {};
};
