// ================================================================
// Emulator.h  –  Hardware emulator
// ================================================================
#pragma once
#include <Arduino.h>
#include <math.h>
#include "SensorManager.h"

#if EMU_MODE

enum class EmuScenario { NONE, WARMUP, BOIL, HEADS, BODY };

class Emulator {
public:
    bool active = true;

    struct EmuValues {
        float T1 = 20.0f;
        float T2 = 20.0f;
        float T3 = 20.0f;
        float P  = 1013.0f;
        bool  L  = true;
        bool  F  = true;
        float C  = 0.0f;
        float V  = 0.0f;
        bool  T1err = false;
        bool  T2err = false;
        bool  T3err = false;
    } val;

    EmuScenario scenario = EmuScenario::NONE;

    void begin() {
        _scenarioStart = millis();
        _lastUpdate = millis();
    }

    void injectAll(SensorManager& sm) {
        _updateScenario();

        const uint32_t now = millis();

        sm.t1->value = val.T1err ? NAN : val.T1;
        sm.t1->error = val.T1err;
        sm.t1->present = !val.T1err;
        sm.t1->_lastPollMs = now;

        sm.t2->value = val.T2err ? NAN : val.T2;
        sm.t2->error = val.T2err;
        sm.t2->present = !val.T2err;
        sm.t2->_lastPollMs = now;

        sm.t3->value = val.T3err ? NAN : val.T3;
        sm.t3->error = val.T3err;
        sm.t3->present = !val.T3err;
        sm.t3->_lastPollMs = now;

        sm.dt->poll();

        sm.p->value = val.P;
        sm.p->error = false;
        sm.p->present = true;
        sm.p->_lastPollMs = now;

        sm.l->value = val.L ? 1.0f : 0.0f;
        sm.l->error = false;
        sm.l->present = true;
        sm.l->_lastPollMs = now;

        sm.f->value = val.F ? 1.0f : 0.0f;
        sm.f->error = false;
        sm.f->present = true;
        sm.f->_lastPollMs = now;

        sm.c->value = val.C;
        sm.c->error = false;
        sm.c->present = true;
        sm.c->_lastPollMs = now;

        sm.v->value = val.V;
        sm.v->error = false;
        sm.v->present = true;
        sm.v->_lastPollMs = now;
    }

    void setScenario(const String& name) {
        if      (name == "warmup") scenario = EmuScenario::WARMUP;
        else if (name == "boil")   scenario = EmuScenario::BOIL;
        else if (name == "heads")  scenario = EmuScenario::HEADS;
        else if (name == "body")   scenario = EmuScenario::BODY;
        else                       scenario = EmuScenario::NONE;

        _scenarioStart = millis();
    }

private:
    uint32_t _scenarioStart = 0;
    uint32_t _lastUpdate    = 0;

    void _updateScenario() {
        if (scenario == EmuScenario::NONE) return;
        if (millis() - _lastUpdate < 1000UL) return;

        _lastUpdate = millis();
        const float t = (millis() - _scenarioStart) / 1000.0f;

        switch (scenario) {
            case EmuScenario::WARMUP:
                val.T1 = constrain(20.0f + t * 0.30f, 20.0f, 78.0f);
                val.T2 = constrain(20.0f + t * 0.20f, 20.0f, 75.0f);
                val.T3 = constrain(20.0f + t * 0.10f, 20.0f, 50.0f);
                val.L  = true;
                val.F  = false;
                val.C  = 600.0f;
                break;

            case EmuScenario::BOIL:
                val.T1 = 78.0f + sinf(t * 0.10f) * 0.5f;
                val.T2 = 72.0f + sinf(t * 0.10f) * 1.0f;
                val.T3 = 25.0f;
                val.L  = true;
                val.F  = false;
                break;

            case EmuScenario::HEADS:
                val.T1 = 78.5f + sinf(t * 0.05f) * 0.3f;
                val.T2 = 75.0f + t * 0.05f;
                val.T3 = 30.0f + t * 0.1f;
                val.L  = true;
                val.F  = (t > 3.0f);
                break;

            case EmuScenario::BODY:
                val.T1 = 78.3f + sinf(t * 0.02f) * 0.2f;
                val.T2 = 78.0f + sinf(t * 0.02f) * 0.2f;
                val.T3 = 35.0f + sinf(t * 0.01f) * 1.0f;
                val.L  = true;
                val.F  = true;
                val.C  = 550.0f;
                break;

            default:
                break;
        }
    }
};

#else

class Emulator {
public:
    bool active = false;
    void begin() {}
    void injectAll(SensorManager&) {}
    void setScenario(const String&) {}
};

#endif
