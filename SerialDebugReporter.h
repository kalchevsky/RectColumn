// ================================================================
// SerialDebugReporter.h
// ================================================================
#pragma once

#include <Arduino.h>
#include "config.h"
#include "TimeBase.h"
#include "SensorManager.h"
#include "ConfirmationManager.h"

class SerialDebugReporter {
public:
    void begin() { _lastPrintMs = 0; }

    void loop(const TimeBase& tb, const SensorManager& sm, const ConfirmationManager& cm) {
        const uint32_t now = millis();
        if (now - _lastPrintMs < SERIAL_DEBUG_SNAPSHOT_INTERVAL_MS) return;
        _lastPrintMs = now;

        Serial.print("[");
        if (tb.isSynced()) Serial.print(tb.nowStr());
        else {
            Serial.print("T+");
            Serial.print(now / 1000UL);
            Serial.print("s");
        }
        Serial.print("][");
        Serial.print(EMU_MODE ? "EMU" : "REAL");
        Serial.print("] ");

        for (int si = 0; si < SEN_COUNT; si++) {
            if (si) Serial.print(" ");
            printSensorToken(si, sm.s[si]);
        }

        for (int ci = 0; ci < 4; ci++) {
            Serial.print(" ");
            printConfirmationToken(cm.get(ci));
        }

        Serial.println();
    }

private:
    uint32_t _lastPrintMs = 0;

    static void printSensorToken(int idx, const SensorBase* s) {
        Serial.print(SensorManager::sensorName(idx));
        Serial.print("=");

        if (!s->enabled) { Serial.print("DISABLED"); return; }
        if (s->hwLimited && s->error) { Serial.print("ERR"); return; }
        if (!s->present && s->error) { Serial.print("ABSENT"); return; }
        if (s->error) { Serial.print("ERR"); return; }
        if (!s->present) { Serial.print("ABSENT"); return; }

        switch (idx) {
            case SEN_L:
            case SEN_F:
                Serial.print(s->value > 0.5f ? "CLOSED" : "OPEN");
                break;
            case SEN_T1:
            case SEN_T2:
            case SEN_T3:
            case SEN_DT:
                Serial.print(s->value, 2);
                Serial.print("C");
                break;
            case SEN_P:
                Serial.print(s->value, 1);
                Serial.print("гПа");
                break;
            default:
                Serial.print(s->value, 0);
                break;
        }
    }

    static void printConfirmationToken(const ConfirmationChannel& c) {
        Serial.print(c.id);
        Serial.print("=");
        if (!c.available) { Serial.print("DISABLED"); return; }
        if (c.timeout) { Serial.print("TIMEOUT"); return; }
        if (c.pending) { Serial.print("PENDING"); return; }
        if (c.mismatch) { Serial.print("MISMATCH"); return; }
        Serial.print(c.actual ? "ON" : "OFF");
    }
};
