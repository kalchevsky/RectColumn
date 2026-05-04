#pragma once

#include <Arduino.h>
#include <math.h>

#include "config.h"
#include "TimeBase.h"
#include "EventLog.h"
#include "SensorManager.h"
#include "OutputManager.h"
#include "ConfirmationManager.h"

#ifndef SAFETY_T2_COOLDOWN_THRESHOLD
#define SAFETY_T2_COOLDOWN_THRESHOLD  35.0f
#endif

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
        if (_levelAlarmLatched || _levelShutdownCh1Done || _flowAlarmLatched ||
            _flowEmergencyLatched || _pressureEmergencyLatched) {
            return true;
        }
        for (uint8_t i = 0; i < 4; i++) {
            if (_werFaultLatched[i]) return true;
        }
        return false;
    }

    void resetLatchedFaults() {
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            _om->clearSafetyForbid(oi, RULEIDX_SAFETY_WER);
        }
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
    bool _levelShutdownCh1Done = false;
    bool _levelCooldownCh2Done = false;
    uint32_t _levelStartedMs = 0;

    bool _flowConditionLatched = false;
    bool _flowAlarmLatched = false;
    bool _flowEmergencyLatched = false;
    uint32_t _flowStartedMs = 0;

    bool _pressureEmergencyLatched = false;
    bool _werFaultLatched[4] = { false, false, false, false };

    void _alarm(const String& msg) {
        _log->add(msg, _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
    }

    void _stopProcess(const String& why) {
        // Process safety is an explicit emergency layer above normal sensor
        // control. Keep STOP latched and expose the FLOW safety reason as
        // separate forbid bits for diagnostics.
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            _om->setSafetyForbid(oi, RULEIDX_SAFETY_FLOW, true);
        }
        _om->setMainStopLatched(true);
        _alarm(why + ". STOP защёлкнут, ручное включение CH1-CH3 заблокировано до снятия STOP.");
    }

    void _handleLevelEmergency(uint32_t now) {
        SensorBase* ls = _sm->s[SEN_L];
        if (!ls) return;

        ls->externalAlarmLogic = true;
        ls->externalAlarmMaskBits = 0;
        for (uint8_t ai = 1; ai < N_ALARMS; ai++) ls->alarm[ai].triggered = false;

        const bool level = ls->enabled && _sm->levelActive();
        const uint32_t alarmDelayMs = ls->alarmDelayMs;
        const uint32_t ctrlDelayMs = SAFETY_LEVEL_SHUTDOWN_MS;

        if (level) {
            if (!_levelRawLatched) {
                _levelRawLatched = true;
                _levelStartedMs = now;
                _levelAlarmLatched = false;
                _levelShutdownCh1Done = false;
                _levelCooldownCh2Done = false;
            }

            if (!_levelAlarmLatched && (now - _levelStartedMs >= alarmDelayMs)) {
                _levelAlarmLatched = true;
                _alarm("Авария уровня: цепь L разомкнута");
            }

            if ((now - _levelStartedMs >= alarmDelayMs) && ls->alarm[0].enabled) {
                ls->externalAlarmMaskBits |= (1u << 0);
                ls->alarm[0].triggered = true;
            } else {
                ls->alarm[0].triggered = false;
            }

            if (!_levelShutdownCh1Done && (now - _levelStartedMs >= ctrlDelayMs)) {
                _om->setSafetyForbid(OUT_CH1, RULEIDX_SAFETY_LEVEL, true);
                _levelShutdownCh1Done = true;
                _alarm("Задержка управления уровнем истекла: CH1 выкл.");
            }

            if (_levelShutdownCh1Done && !_levelCooldownCh2Done) {
                const float t2 = _sm->getT2();
                SensorBase* t2s = _sm->s[SEN_T2];
                if (!t2s || !t2s->hasUsableValue()) {
                    _om->setSafetyForbid(OUT_CH2, RULEIDX_SAFETY_LEVEL, true);
                    _levelCooldownCh2Done = true;
                    _alarm("Аварийное охлаждение по уровню: T2 невалиден, CH2 выкл.");
                } else if (!isnan(t2) && t2 < SAFETY_T2_COOLDOWN_THRESHOLD) {
                    _om->setSafetyForbid(OUT_CH2, RULEIDX_SAFETY_LEVEL, true);
                    _levelCooldownCh2Done = true;
                    _alarm("Аварийное охлаждение по уровню: T2 ниже порога, CH2 выкл.");
                }
            }
            return;
        }

        if (_levelRawLatched || _levelAlarmLatched || _levelShutdownCh1Done || _levelCooldownCh2Done) {
            _alarm("Авария уровня сброшена");
        }
        _om->clearSafetyForbid(OUT_CH1, RULEIDX_SAFETY_LEVEL);
        _om->clearSafetyForbid(OUT_CH2, RULEIDX_SAFETY_LEVEL);
        ls->alarm[0].triggered = false;
        _levelRawLatched = false;
        _levelAlarmLatched = false;
        _levelShutdownCh1Done = false;
        _levelCooldownCh2Done = false;
        _levelStartedMs = 0;
    }

    void _handleFlowLoss(uint32_t now) {
        SensorBase* fs = _sm->s[SEN_F];
        if (!fs) return;

        fs->externalAlarmLogic = true;
        fs->externalAlarmMaskBits = 0;
        for (uint8_t ai = 1; ai < N_ALARMS; ai++) fs->alarm[ai].triggered = false;

        const ConfirmationChannel& ch2 = _cm->get(1);
        const bool valveConfirmed = ch2.available ? ch2.actual : _om->out[OUT_CH2]->actualOn();
        const bool valveRequested = _om->out[OUT_CH2]->requestedOn() || _om->out[OUT_CH2]->actualOn();
        const bool flowFault = fs->enabled && !_sm->flowActive();
        const uint32_t alarmDelayMs = fs->alarmDelayMs;
        const uint32_t ctrlDelayMs = SAFETY_FLOW_LOSS_MS;
        bool alarmActive = false;

        if (flowFault) {
            if (!_flowConditionLatched) {
                _flowConditionLatched = true;
                _flowStartedMs = now;
                _flowAlarmLatched = false;
                _flowEmergencyLatched = false;
            }

            alarmActive = (now - _flowStartedMs >= alarmDelayMs);

            if (alarmActive && fs->alarm[0].enabled) {
                fs->externalAlarmMaskBits |= (1u << 0);
                fs->alarm[0].triggered = true;
            } else {
                fs->alarm[0].triggered = false;
            }

            if (alarmActive && !_flowAlarmLatched) {
                _flowAlarmLatched = true;
                _alarm("Авария потока: цепь F разомкнута / нет потока");
            }

            if (alarmActive && (valveConfirmed || valveRequested) &&
                !_flowEmergencyLatched && (now - _flowStartedMs >= ctrlDelayMs)) {
                _flowEmergencyLatched = true;
                _stopProcess("Поток потерян при запросе CH2: останов процесса");
            }
            return;
        }

        if (_flowConditionLatched || _flowAlarmLatched || _flowEmergencyLatched) {
            _alarm("Поток восстановлен");
        }
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            _om->clearSafetyForbid(oi, RULEIDX_SAFETY_FLOW);
        }
        fs->alarm[0].triggered = false;
        _flowConditionLatched = false;
        _flowAlarmLatched = false;
        _flowEmergencyLatched = false;
        _flowStartedMs = 0;
    }

    void _handlePressureHigh() {
        SensorBase* p = _sm->s[SEN_P];
        if (!p) return;

        bool high = (p->alarmMask() != 0);
        if (!high && p->hasUsableValue()) {
            for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
                const CtrlRule& r = p->ctrl[oi];
                if (r.enabled && r.outIdx == oi && p->value > p->effectiveThreshold(r.maxVal)) {
                    high = true;
                    break;
                }
            }
        }
        if (high && !_pressureEmergencyLatched) {
            _pressureEmergencyLatched = true;
            _om->setSafetyForbid(OUT_CH1, RULEIDX_SAFETY_PRESSURE, true);
            _alarm("Высокое давление: CH1 выкл.");
        }

        if (!high && _pressureEmergencyLatched) {
            _pressureEmergencyLatched = false;
            _om->clearSafetyForbid(OUT_CH1, RULEIDX_SAFETY_PRESSURE);
            _alarm("Авария давления сброшена");
        }
    }

    void _handleWerTimeout() {
        for (uint8_t i = 0; i < 4; i++) {
            const ConfirmationChannel& c = _cm->get(i);
            const bool faultedMain =
                (c.outputIdx == OUT_CH1 || c.outputIdx == OUT_CH2 || c.outputIdx == OUT_CH3) &&
                c.faultLatched;

            if (faultedMain && !_werFaultLatched[i]) {
                _werFaultLatched[i] = true;
                _om->setSafetyForbid(c.outputIdx, RULEIDX_SAFETY_WER, true);
                _om->out[c.outputIdx]->forceOff(true);
                _alarm(String(c.id) + ": защёлкнута авария подтверждения - " +
                       ConfirmationManager::faultNameRu(c.fault) +
                       ". Канал заблокирован до сброса аварии.");
            }
        }
    }
};
