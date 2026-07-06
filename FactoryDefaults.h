#pragma once

#include "FactoryDefaultsTable.h"
#include "EventLog.h"
#include "Storage.h"

namespace FactoryDefaults {

inline void prepareFactoryDefaultsMigration(Storage& storage) {
    if (!storage.ready()) return;
    if (storage.loadFactoryDone()) return;
    if (storage.hasAnyPersistedConfig()) {
        (void)storage.saveFactoryDone(true);
    }
}

inline bool needsFactoryDefaults(Storage& storage) {
    if (!storage.ready()) return false;
    if (storage.loadFactoryDone()) return false;
    return true;
}

inline void _applyOffOnlySensor(SensorBase* sensor) {
    if (!sensor) return;
    sensor->enabled = FactoryDefaultsTable::OFF_ONLY_ENABLED;
    sensor->periodMs = FactoryDefaultsTable::OFF_ONLY_PERIOD_MS;
    sensor->alarmDelayMs = FactoryDefaultsTable::OFF_ONLY_ALARM_DELAY_MS;
    sensor->ctrlDelayMs = FactoryDefaultsTable::OFF_ONLY_CTRL_DELAY_MS;
    for (int oi = 0; oi < N_CTRL_OUT; oi++) {
        sensor->ctrl[oi].enabled = FactoryDefaultsTable::OFF_ONLY_CTRL_ENABLED;
    }
    for (int ai = 0; ai < N_ALARMS; ai++) {
        sensor->alarm[ai].enabled = FactoryDefaultsTable::OFF_ONLY_ALARM_ENABLED;
    }
}

inline void _applyTempFamilyProfile(SensorBase* sensor) {
    if (!sensor) return;

    const auto& preset = FactoryDefaultsTable::TEMP_FAMILY;
    sensor->enabled = preset.enabled;
    sensor->periodMs = preset.periodMs;
    sensor->alarmDelayMs = preset.alarmDelayMs;
    sensor->ctrlDelayMs = preset.ctrlDelayMs;

    for (int oi = 0; oi < N_CTRL_OUT; oi++) {
        sensor->ctrl[oi].enabled = preset.ctrlEnabled;
        sensor->ctrl[oi].minVal = preset.ctrlMinVal;
        sensor->ctrl[oi].maxVal = preset.ctrlMaxVal;
    }

    sensor->alarm[0].enabled = preset.alarm0.enabled;
    sensor->alarm[0].threshold = preset.alarm0.threshold;
    sensor->alarm[0].isMax = preset.alarm0.isMax;

    sensor->alarm[1].enabled = preset.alarm1.enabled;
    sensor->alarm[1].threshold = preset.alarm1.threshold;
    sensor->alarm[1].isMax = preset.alarm1.isMax;

    for (int ai = 2; ai < N_ALARMS; ai++) {
        sensor->alarm[ai].enabled = false;
        sensor->alarm[ai].threshold = 100.0f;
        sensor->alarm[ai].isMax = true;
    }
}

inline void _applyPressureProfile(SensorBase* sensor) {
    if (!sensor) return;

    const auto& preset = FactoryDefaultsTable::PRESSURE;
    sensor->enabled = preset.enabled;
    sensor->periodMs = preset.periodMs;
    sensor->alarmDelayMs = preset.alarmDelayMs;
    sensor->ctrlDelayMs = preset.ctrlDelayMs;

    for (int oi = 0; oi < N_CTRL_OUT; oi++) {
        sensor->ctrl[oi].enabled = preset.ctrlEnabled;
        sensor->ctrl[oi].minVal = preset.ctrlMinVal;
        sensor->ctrl[oi].maxVal = preset.ctrlMaxVal;
    }

    for (int ai = 0; ai < N_ALARMS; ai++) {
        sensor->alarm[ai].enabled = preset.alarmsEnabled[ai];
        sensor->alarm[ai].threshold = preset.alarmThreshold[ai];
        sensor->alarm[ai].isMax = preset.alarmIsMax[ai];
    }
}

inline void _resetOutputsToOff(OutputManager& om) {
    om.soundMuted = FactoryDefaultsTable::OUTPUT_SOUND_MUTED;
    om.chMode[0] = FactoryDefaultsTable::OUTPUT_CH_MODE[0];
    om.chMode[1] = FactoryDefaultsTable::OUTPUT_CH_MODE[1];
    om.chMode[2] = FactoryDefaultsTable::OUTPUT_CH_MODE[2];
    om.ch4Enabled = FactoryDefaultsTable::OUTPUT_CH4_ENABLED;
    om.ch5Enabled = FactoryDefaultsTable::OUTPUT_CH5_ENABLED;

    for (int i = 0; i < OUT_COUNT; i++) {
        if (!om.out[i]) continue;
        om.out[i]->clearCommand();
        om.out[i]->clearTransientOverrides();
        om.out[i]->forceOff(true);
    }

    om.restoreManualState(OUT_CH1, false);
    om.restoreManualState(OUT_CH2, false);
    om.restoreManualState(OUT_CH3, false);
    om.restoreOperatorHoldOff(OUT_CH1, false);
    om.restoreOperatorHoldOff(OUT_CH2, false);
    om.restoreOperatorHoldOff(OUT_CH3, false);
    om.restoreMainStopLatched(false);
    om.applyConfig();
}

inline void _syncCtrlLogicFromOutputModes(SensorManager& sm, OutputManager& om) {
    sm.normalizeDigitalOffOnlyRules();
    for (int si = 0; si < SEN_COUNT; si++) {
        if (!SensorManager::isSchemeAnalogControlSensorIndex((uint8_t)si)) continue;
        SensorBase* sensor = sm.s[si];
        if (!sensor) continue;
        for (int oi = 0; oi < N_CTRL_OUT; oi++) {
            const uint8_t outIdx = sensor->ctrl[oi].outIdx;
            if (SensorManager::isMainOutputIndex(outIdx)) {
                sensor->ctrl[oi].logic = om.chMode[outIdx];
            }
        }
    }
    sm.normalizeDigitalOffOnlyRules();
    sm.normalizeSchemeControlRules();
}

inline bool applyFactoryDefaults(Storage& storage,
                                 SensorManager& sm,
                                 OutputManager& om,
                                 EventLog* log = nullptr) {
    if (log) log->add("Factory defaults: applying");

    _resetOutputsToOff(om);

    _applyTempFamilyProfile(sm.t1);
    _applyTempFamilyProfile(sm.t2);
    _applyTempFamilyProfile(sm.t3);
    _applyTempFamilyProfile(sm.dt);
    _applyPressureProfile(sm.p);

    _applyOffOnlySensor(sm.l);
    _applyOffOnlySensor(sm.f);
    _applyOffOnlySensor(sm.v);
    _applyOffOnlySensor(sm.c);
    if (sm.c) {
        sm.c->alarm[0].threshold = FactoryDefaultsTable::CURRENT_ALARM_THRESHOLD_PERCENT;
    }

    _syncCtrlLogicFromOutputModes(sm, om);

    bool ok = storage.saveSensorsChecked(sm);
    ok = ok && storage.saveOutputConfigChecked(om);
    ok = ok && storage.saveNotifyConfigChecked(
        FactoryDefaultsTable::NOTIFY_ENABLED,
        FactoryDefaultsTable::NOTIFY_URL,
        FactoryDefaultsTable::NOTIFY_TOKEN
    );
    ok = ok && storage.saveAPPasswordChecked(FactoryDefaultsTable::WIFI_AP_PASS);
    ok = ok && storage.saveWifiSTAChecked(
        FactoryDefaultsTable::WIFI_STA_SSID,
        FactoryDefaultsTable::WIFI_STA_PASS
    );
    ok = ok && storage.saveWifiApOnlyChecked(FactoryDefaultsTable::WIFI_AP_ONLY);
    ok = ok && storage.saveWifiWizardDoneChecked(FactoryDefaultsTable::WIFI_WIZARD_DONE);
    if (ok) {
        ok = storage.saveFactoryDone(FactoryDefaultsTable::SYSTEM_FACTORY_DONE);
    }

    if (log) {
        const String msg = ok
            ? String("Factory defaults: applied")
            : (String("Factory defaults: apply failed - ") + storage.statusText());
        log->add(msg);
    }
    return ok;
}

} // namespace FactoryDefaults
