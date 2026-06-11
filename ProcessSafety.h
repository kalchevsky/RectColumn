#pragma once

#include <Arduino.h>
#include "config.h"
#include "TimeBase.h"
#include "EventLog.h"
#include "SensorManager.h"
#include "OutputManager.h"
#include "ConfirmationManager.h"

class ProcessSafety {
public:
    void begin(TimeBase& tb, EventLog& log, SensorManager& sm,
               OutputManager& om, ConfirmationManager& cm)
    {
        _tb = &tb;
        _log = &log;
        _sm = &sm;
        _om = &om;
        _cm = &cm;
    }

    void loop() {
        const uint32_t now = millis();
        if (!_tb || !_log || !_sm || !_om || !_cm) return;

        _handleLevelEmergency(now);
        _handleFlowLoss(now);
        _handlePressureHigh();
        _handleWerTimeout();
    }

    bool safetyAlarmActive() const {
        return false;
    }

    void resetLatchedFaults() {
        for (uint8_t i = 0; i < 4; i++) _werFaultLatched[i] = false;
        if (_cm) _cm->resetFaults();
        if (_log && _sm) _alarm("Оператор сбросил защёлкнутые аварии safety-слоя");
    }

private:
    TimeBase* _tb = nullptr;
    EventLog* _log = nullptr;
    SensorManager* _sm = nullptr;
    OutputManager* _om = nullptr;
    ConfirmationManager* _cm = nullptr;

    bool _levelRawLatched = false;
    bool _levelAlarmLatched = false;
    uint32_t _levelStartedMs = 0;

    bool _flowConditionLatched = false;
    bool _flowAlarmLatched = false;
    uint32_t _flowStartedMs = 0;

    bool _werFaultLatched[4] = { false, false, false, false };

    void _alarm(const String& msg) {
        _log->add(msg, _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
    }

    void _handleLevelEmergency(uint32_t now) {
        SensorBase* ls = _sm->s[SEN_L];
        if (!ls) return;

        ls->externalAlarmLogic = true;
        ls->externalAlarmMaskBits = 0;
        for (uint8_t ai = 1; ai < N_ALARMS; ai++) ls->alarm[ai].triggered = false;

        const bool level = ls->enabled && _sm->levelActive();
        const uint32_t alarmDelayMs = ls->alarmDelayMs;

        if (level) {
            if (!_levelRawLatched) {
                _levelRawLatched = true;
                _levelStartedMs = now;
                _levelAlarmLatched = false;
            }

            if (!_levelAlarmLatched && (now - _levelStartedMs >= alarmDelayMs)) {
                _levelAlarmLatched = true;
                _alarm("Уровень MAX!");
            }

            if ((now - _levelStartedMs >= alarmDelayMs) && ls->alarm[0].enabled) {
                ls->externalAlarmMaskBits |= (1u << 0);
                ls->alarm[0].triggered = true;
            } else {
                ls->alarm[0].triggered = false;
            }
            return;
        }

        if (_levelRawLatched || _levelAlarmLatched) {
            _alarm("Авария уровня сброшена");
        }
        ls->alarm[0].triggered = false;
        _levelRawLatched = false;
        _levelAlarmLatched = false;
        _levelStartedMs = 0;
    }

    void _handleFlowLoss(uint32_t now) {
        SensorBase* fs = _sm->s[SEN_F];
        if (!fs) return;

        fs->externalAlarmLogic = true;
        fs->externalAlarmMaskBits = 0;
        for (uint8_t ai = 1; ai < N_ALARMS; ai++) fs->alarm[ai].triggered = false;

        if (fs->tracksSensorLoss() && (fs->isSensorErrorActive() || !fs->present)) {
            fs->alarm[0].triggered = false;
            _flowConditionLatched = false;
            _flowAlarmLatched = false;
            _flowStartedMs = 0;
            return;
        }

        // ── Тревога "нет протока" — единый источник истины ──────────────────
        // Состояние берётся из фазовой машины OutputManager: тревога активна
        // только в фазе FAULT (окно ожидания grace истекло и протока нет).
        // Выдержку уже отработала фазовая машина (grace = ctrlDelayMs F),
        // поэтому здесь дополнительной задержки нет. В NEUTRAL/WAITING тревога
        // снимается автоматически (не липкая). Каналы здесь НЕ гасятся.
        // ────────────────────────────────────────────────────────────────────
        const bool flowFault = fs->enabled && _om && _om->flowPhaseIsFault();

        if (flowFault) {
            _flowConditionLatched = true;   // оставлено для индикации/совместимости
            if (_flowStartedMs == 0) _flowStartedMs = now;

            if (fs->alarm[0].enabled) {
                fs->externalAlarmMaskBits |= (1u << 0);
                fs->alarm[0].triggered = true;
            } else {
                fs->alarm[0].triggered = false;
            }

            if (!_flowAlarmLatched) {
                _flowAlarmLatched = true;
                _alarm("Авария потока: нет протока (окно ожидания истекло)");
            }
            return;
        }

        if (_flowAlarmLatched) {
            _alarm("Поток восстановлен");
        }
        fs->alarm[0].triggered = false;
        _flowConditionLatched = false;
        _flowAlarmLatched = false;
        _flowStartedMs = 0;
    }

    void _handlePressureHigh() {
        _om->clearSafetyForbid(OUT_CH1, RULEIDX_SAFETY_PRESSURE);
    }

    void _handleWerTimeout() {
        for (uint8_t i = 0; i < 4; i++) {
            const ConfirmationChannel& c = _cm->get(i);
            const bool faultedMain = requiresWerConfirmation(c.outputIdx) && c.faultLatched;

            if (faultedMain && !_werFaultLatched[i]) {
                _werFaultLatched[i] = true;
                _alarm(String(c.id) + ": нет подтверждения реле - " +
                       ConfirmationManager::faultNameRu(c.fault) +
                       ". Только индикация, без отключения канала.");
            } else if (!faultedMain && _werFaultLatched[i]) {
                _werFaultLatched[i] = false;
            }
        }
    }
};
