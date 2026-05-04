// ================================================================
// SensorManager.h 
// ================================================================
#pragma once
#include "Sensors.h"
#include "config.h"

class SensorManager {
public:
    SensorBase* s[SEN_COUNT];

    TempSensor*     t1 = nullptr;
    TempSensor*     t2 = nullptr;
    TempSensor*     t3 = nullptr;
    VirtualSensor*  dt = nullptr;
    PressureSensor* p  = nullptr;
    DigitalSensor*  l  = nullptr;
    DigitalSensor*  f  = nullptr;
    AnalogSensor*   c  = nullptr;
    AnalogSensor*   v  = nullptr;

    SensorManager() {
        for (int i = 0; i < SEN_COUNT; i++) s[i] = nullptr;

        t1 = new TempSensor("T1", PIN_T1);
        t2 = new TempSensor("T2", PIN_T2);
        t3 = new TempSensor("T3", PIN_T3);
        dt = new VirtualSensor(t1, t2);
        p  = new PressureSensor();
        l  = new DigitalSensor("L", PIN_L);
        f  = new DigitalSensor("F", PIN_F);
        c  = new AnalogSensor("C", PIN_C, false, false);
        v  = new AnalogSensor("V", PIN_V, false, (GPIO35_MODE == GPIO35_MODE_WER_CH2));
        c->thresholdPercentInput = true;

        // Базовые задержки по умолчанию для текущей логики проекта.
        l->alarmDelayMs = DIGITAL_ALARM_DEBOUNCE_MS;
        l->ctrlDelayMs  = SAFETY_LEVEL_SHUTDOWN_MS;
        f->alarmDelayMs = DIGITAL_ALARM_DEBOUNCE_MS;
        f->ctrlDelayMs  = 5000UL;
        c->alarmDelayMs = 1000UL;
        c->ctrlDelayMs  = 0;

        // ── Умолчания правил управления для цифровых датчиков ─────────────
        // Датчики L и F двоичные (value = 0.0 или 1.0).
        // Схема: L=0 (обрыв цепи = нет уровня) → FORBID (CH_xLOF=1) с задержкой.
        //        F=0 (нет протока)              → FORBID (CH_xFOF=1) с задержкой.
        //
        // Проблема старых умолчаний CtrlRule (LOGIC_HEAT, minVal=0.0, maxVal=100.0):
        //   evalCtrl() при HEAT: cmd=-1 если value > maxVal  → 0 > 100 → false, нет forbid!
        //   evalCtrl() при HEAT: cmd=+1 если value < minVal  → 0 < 0.0 → false, нет want!
        //   Результат: цифровые датчики никогда не генерировали управляющих команд.
        //
        // Исправление — LOGIC_COOL c minVal=0.5:
        //   evalCtrl() при COOL: cmd=-1 если value < minVal  → 0 < 0.5 → true → FORBID ✓
        //   evalCtrl() при COOL: cmd=+1 если value > maxVal  → 1 > 2.0 → false → нет WANT ✓
        //   Т.е. датчик может только запрещать (OF=1), никогда не требует включения (ON=0).
        //
        // ctrl[oi].enabled остаётся false — оператор включает нужные каналы через UI.
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            l->ctrl[oi].logic  = LOGIC_COOL;
            l->ctrl[oi].minVal = 0.5f;
            l->ctrl[oi].maxVal = 2.0f;
        }
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            f->ctrl[oi].logic  = LOGIC_COOL;
            f->ctrl[oi].minVal = 0.5f;
            f->ctrl[oi].maxVal = 2.0f;
        }
        normalizeDigitalOffOnlyRules();
        normalizeSchemeControlRules();

        s[SEN_T1] = t1;
        s[SEN_T2] = t2;
        s[SEN_T3] = t3;
        s[SEN_DT] = dt;
        s[SEN_P]  = p;
        s[SEN_L]  = l;
        s[SEN_F]  = f;
        s[SEN_C]  = c;
        s[SEN_V]  = v;
        normalizeSchemeControlRules();
    }

    static bool isMainOutputIndex(uint8_t outIdx) {
        return outIdx == OUT_CH1 || outIdx == OUT_CH2 || outIdx == OUT_CH3;
    }

    static bool isDigitalOffOnlySensorIndex(uint8_t sensorIdx) {
        return sensorIdx == SEN_L || sensorIdx == SEN_F;
    }

    // These are the analog sensors from the channel-control chart whose
    // HEAT/COOL logic follows the output mode. dT/C/V remain explicit
    // per-sensor extensions and are not rewritten by output mode changes.
    static bool isSchemeAnalogControlSensorIndex(uint8_t sensorIdx) {
        return sensorIdx == SEN_T1 || sensorIdx == SEN_T2 ||
               sensorIdx == SEN_T3 || sensorIdx == SEN_P;
    }

    static bool isSchemeControlSensorIndex(uint8_t sensorIdx) {
        return isSchemeAnalogControlSensorIndex(sensorIdx) ||
               sensorIdx == SEN_L || sensorIdx == SEN_F;
    }

    static bool isRuleAllowedForOutput(uint8_t sensorIdx, uint8_t outIdx) {
        if (!isMainOutputIndex(outIdx)) return true;
        return isSchemeControlSensorIndex(sensorIdx);
    }

    static bool isDigitalOffOnlyRule(uint8_t sensorIdx, uint8_t outIdx) {
        return isDigitalOffOnlySensorIndex(sensorIdx) && isMainOutputIndex(outIdx);
    }

    static void normalizeDigitalOffOnlySensor(SensorBase* sensor) {
        if (!sensor) return;
        for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            CtrlRule& r = sensor->ctrl[oi];
            const bool keepEnabled = r.enabled;
            r.enabled = keepEnabled;
            r.outIdx = oi;
            r.logic  = LOGIC_COOL;
            r.minVal = 0.5f;
            r.maxVal = 2.0f;
        }
    }

    void normalizeDigitalOffOnlyRules() {
        normalizeDigitalOffOnlySensor(l);
        normalizeDigitalOffOnlySensor(f);
    }

    void normalizeSchemeControlRules() {
        for (uint8_t si = 0; si < SEN_COUNT; si++) {
            SensorBase* sensor = s[si];
            if (!sensor) continue;
            for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
                CtrlRule& r = sensor->ctrl[oi];
                r.outIdx = oi;
                if (!isRuleAllowedForOutput(si, oi)) r.enabled = false;
            }
        }
    }

    void begin() {
        normalizeDigitalOffOnlyRules();
        normalizeSchemeControlRules();
        for (int i = 0; i < SEN_COUNT; i++) {
            if (s[i]) s[i]->begin();
        }
    }

    uint32_t loop() {
        uint32_t alarmChanged = 0;

    #if EMU_MODE
        if (dt) {
            dt->poll();
        }
        for (int i = 0; i < SEN_COUNT; i++) {
            if (s[i] && s[i]->checkAlarms()) {
                alarmChanged |= (1u << i);
            }
        }
        return alarmChanged;
    #endif

        const uint32_t now = millis();
        bool tempUpdated = false;

        // У T1/T2/T3 отдельные шины OneWire, поэтому их можно обслуживать
        // независимо без искусственной очереди по 1 датчику в секунду.
        for (int ti = 0; ti < 3; ti++) {
            const int idx = _tempIdx[ti];
            TempSensor* ts = _asTemp(idx);
            if (!ts) continue;
            if (ts->isReadyToRead(now) && ts->readConversion(now)) {
                if (s[idx]->checkAlarms()) alarmChanged |= (1u << idx);
                tempUpdated = true;
            }
        }

        if (tempUpdated) {
            _refreshVirtualDT(alarmChanged);
        }

        for (int ti = 0; ti < 3; ti++) {
            const int idx = _tempIdx[ti];
            TempSensor* ts = _asTemp(idx);
            if (ts && ts->isDueToStart(now)) {
                ts->startConversion(now);
            }
        }

        if (p && p->isDue()) {
            p->poll();
            if (p->checkAlarms()) alarmChanged |= (1u << SEN_P);
        }

        for (int fi = 0; fi < 4; fi++) {
            const int idx = _fastIdx[fi];
            SensorBase* sen = s[idx];
            if (sen && sen->isDue()) {
                sen->poll();
                if (sen->checkAlarms()) alarmChanged |= (1u << idx);
            }
        }

        return alarmChanged;
    }

    static const char* sensorName(int idx) {
        static const char* names[SEN_COUNT] = {"T1","T2","T3","dT","P","L","F","C","V"};
        return (idx >= 0 && idx < SEN_COUNT) ? names[idx] : "";
    }

    static const char* sensorUnit(int idx) {
        static const char* units[SEN_COUNT] = {"C","C","C","C","hPa","","","",""};
        return (idx >= 0 && idx < SEN_COUNT) ? units[idx] : "";
    }

    float getT1() const { return (t1 && t1->enabled) ? t1->value : NAN; }
    float getT2() const { return (t2 && t2->enabled) ? t2->value : NAN; }
    float getT3() const { return (t3 && t3->enabled) ? t3->value : NAN; }
    float getDT() const { return (dt && dt->enabled) ? dt->value : NAN; }
    float getP()  const { return (p  && p->enabled ) ? p->value  : NAN; }
    bool  levelActive() const { return (l && l->enabled) && l->isCircuitOpen(); }
    bool  flowActive()  const { return (f && f->enabled) && f->isCircuitClosed(); }
    float getC()  const { return (c  && c->enabled ) ? c->value  : NAN; }
    float getV()  const { return (v  && v->enabled ) ? v->value  : NAN; }

private:
    TempSensor* _asTemp(int idx) const {
        switch (idx) {
            case SEN_T1: return t1;
            case SEN_T2: return t2;
            case SEN_T3: return t3;
            default:     return nullptr;
        }
    }

    void _refreshVirtualDT(uint32_t& alarmChanged) {
        if (!dt) return;
        dt->poll();
        if (dt->checkAlarms()) alarmChanged |= (1u << SEN_DT);
    }

    int _tempIdx[3] = { SEN_T1, SEN_T2, SEN_T3 };
    int _fastIdx[4] = { SEN_L, SEN_F, SEN_C, SEN_V };
};
