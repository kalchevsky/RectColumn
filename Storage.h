// Storage.h  
// ================================================================
#pragma once
#include <Preferences.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include "SensorManager.h"
#include "OutputManager.h"

class Storage {
public:
    bool ready() const { return _nvsReady; }
    bool recovered() const { return _nvsRecovered; }
    uint32_t saveOutputsLastMs() const { return _saveOutputsLastMs; }
    uint32_t saveOutputsMaxMs() const { return _saveOutputsMaxMs; }
    String statusText() const {
        if (_lastStatus.length() > 0) return _lastStatus;
        if (_nvsReady) return "ok";
        return "not initialized";
    }

    void noteSaveOutputsDuration(uint32_t durationMs) {
        _saveOutputsLastMs = durationMs;
        if (durationMs > _saveOutputsMaxMs) _saveOutputsMaxMs = durationMs;
    }

    void saveWifiSTA(const String& ssid, const String& pass) {
        Preferences p;
        if (!_openPrefs(p, "wifi", false)) return;
        p.putString("sta_ssid", ssid);
        p.putString("sta_pass", pass);
        p.end();
    }

    bool loadWifiSTA(String& ssid, String& pass) {
        Preferences p;
        if (!_openPrefs(p, "wifi", true)) {
            ssid = "";
            pass = "";
            return false;
        }
        ssid = p.getString("sta_ssid", "");
        pass = p.getString("sta_pass", "");
        p.end();
        return ssid.length() > 0;
    }

    void saveAPPassword(const String& pass) {
        Preferences p;
        if (!_openPrefs(p, "wifi", false)) return;
        p.putString("ap_pass", pass);
        p.end();
    }

    String loadAPPassword() {
        Preferences p;
        if (!_openPrefs(p, "wifi", true)) return "";
        String pass = p.getString("ap_pass", "");
        p.end();
        return pass;
    }

    void saveWifiWizardDone(bool done) {
        Preferences p;
        if (!_openPrefs(p, "wifi", false)) return;
        p.putBool("wizard_done", done);
        p.end();
    }

    bool loadWifiWizardDone() {
        Preferences p;
        if (!_openPrefs(p, "wifi", true)) return false;
        bool done = p.getBool("wizard_done", false);
        p.end();
        return done;
    }

    void saveSensors(SensorManager& sm) {
        _sanitizeSensors(sm);
        SensorsBlob blob{};
        _fillSensorsBlob(sm, blob);

        Preferences p;
        if (!_openPrefs(p, "sensors", false)) return;
        if (!_writeBlob(p, "blob", blob)) {
            _lastStatus = "Preferences write failed for namespace 'sensors'";
        }
        p.end();
    }

    void loadSensors(SensorManager& sm) {
        Preferences p;
        if (!_openPrefs(p, "sensors", true)) {
            _sanitizeSensors(sm);
            return;
        }

        SensorsBlob blob{};
        const bool hasBlob = _readBlob(p, "blob", blob, SENSOR_BLOB_VERSION)
                          && (blob.sensorCount == SEN_COUNT);
        if (hasBlob) {
            p.end();
            _applySensorsBlobGeneric(sm, blob);
            _sanitizeSensors(sm);
            return;
        }

        SensorsBlobV2 blobV2{};
        const bool hasBlobV2 = _readBlob(p, "blob", blobV2, 2U)
                            && (blobV2.sensorCount == SEN_COUNT);
        if (hasBlobV2) {
            p.end();
            _applySensorsBlobGeneric(sm, blobV2);
            _sanitizeSensors(sm);
            _migrateSensorsToBlob(sm);
            return;
        }

        SensorsBlobV1 blobV1{};
        const bool hasBlobV1 = _readBlob(p, "blob", blobV1, 1U)
                            && (blobV1.sensorCount == SEN_COUNT);
        if (hasBlobV1) {
            p.end();
            _applySensorsBlobGeneric(sm, blobV1);
            _sanitizeSensors(sm);
            _migrateSensorsToBlob(sm);
            return;
        }

        const bool hasLegacy = _hasLegacySensorConfig(p);
        if (hasLegacy) _loadLegacySensors(sm, p);
        p.end();

        _sanitizeSensors(sm);
        if (hasLegacy) _migrateSensorsToBlob(sm);
    }

    void saveOutputConfig(OutputManager& om) {
        _sanitizeOutputs(om);
        OutputsBlob blob{};
        _fillOutputsBlob(om, blob);

        Preferences p;
        if (!_openPrefs(p, "outputs", false)) return;
        if (!_writeBlob(p, "blob", blob)) {
            _lastStatus = "Preferences write failed for namespace 'outputs'";
        }
        p.end();
    }

    void loadOutputConfig(OutputManager& om) {
        Preferences p;
        if (!_openPrefs(p, "outputs", true)) {
            om.applyConfig();
            return;
        }
        OutputsBlob blob{};
        const bool hasBlob = _readBlob(p, "blob", blob, OUTPUT_BLOB_VERSION);
        if (hasBlob) {
            p.end();
            _applyOutputsBlob(om, blob);
            _sanitizeOutputs(om);
            om.applyConfig();
            return;
        }

        OutputsBlobV2 blobV2{};
        const bool hasBlobV2 = _readBlob(p, "blob", blobV2, blobV2.version);
        if (hasBlobV2) {
            p.end();
            _applyOutputsBlob(om, blobV2);
            _sanitizeOutputs(om);
            _migrateOutputsToBlob(om);
            om.applyConfig();
            return;
        }

        OutputsBlobV1 blobV1{};
        const bool hasBlobV1 = _readBlob(p, "blob", blobV1, blobV1.version);
        if (hasBlobV1) {
            p.end();
            _applyOutputsBlob(om, blobV1);
            _sanitizeOutputs(om);
            _migrateOutputsToBlob(om);
            om.applyConfig();
            return;
        }

        const bool hasLegacy = _hasLegacyOutputConfig(p);
        if (hasLegacy) _loadLegacyOutputs(om, p);
        p.end();

        if (hasLegacy) _migrateOutputsToBlob(om);
        _sanitizeOutputs(om);
        om.applyConfig();
    }

    void saveOutputs(OutputManager& om) {
        saveOutputConfig(om);
    }

    void loadOutputs(OutputManager& om) {
        loadOutputConfig(om);
    }

    void saveNotifyConfig(bool enabled, const String& publishUrl, const String& token) {
        Preferences p;
        if (!_openPrefs(p, "notify", false)) return;
        p.putBool("enabled", enabled);
        p.putString("url", publishUrl);
        p.putString("token", token);
        p.end();
    }

    void loadNotifyConfig(bool& enabled, String& publishUrl, String& token) {
        Preferences p;
        if (!_openPrefs(p, "notify", true)) {
            enabled = false;
            publishUrl = "";
            token = "";
            return;
        }
        enabled    = p.getBool("enabled", false);
        publishUrl = p.getString("url", "");
        token      = p.getString("token", "");
        p.end();
    }

private:
    // Compact blobs avoid exhausting the default NVS partition
    // with hundreds of per-field sensor keys.
    static constexpr uint16_t SENSOR_BLOB_VERSION = 3;
    static constexpr uint16_t OUTPUT_BLOB_VERSION = 3;

    struct AlarmBlob {
        float   threshold = 0.0f;
        uint8_t enabled = 0;
        uint8_t isMax = 1;
        uint8_t reserved[2] = {};
    };

    struct CtrlBlob {
        float   minVal = 0.0f;
        float   maxVal = 100.0f;
        uint8_t enabled = 0;
        uint8_t outIdx = 0;
        uint8_t logic = LOGIC_HEAT;
        uint8_t reserved = 0;
    };

    struct SensorBlob {
        uint32_t periodMs = DEF_T_PERIOD_MS;
        uint32_t alarmDelayMs = 0;
        uint32_t ctrlDelayMs = 0;
        AlarmBlob alarm[N_ALARMS];
        CtrlBlob  ctrl[N_CTRL_OUT];
        uint8_t   enabled = 1;
        uint8_t   reserved[3] = {};
    };

    struct SensorsBlob {
        uint16_t version = SENSOR_BLOB_VERSION;
        uint16_t sensorCount = SEN_COUNT;
        SensorBlob sensors[SEN_COUNT];
    };

    struct SensorsBlobV1 {
        uint16_t version = 1;
        uint16_t sensorCount = SEN_COUNT;
        SensorBlob sensors[SEN_COUNT];
    };

    struct SensorsBlobV2 {
        uint16_t version = 2;
        uint16_t sensorCount = SEN_COUNT;
        SensorBlob sensors[SEN_COUNT];
    };

    struct OutputsBlobV1 {
        uint16_t version = 1;
        uint8_t  soundMuted = 0;
        uint8_t  chMode[3] = { LOGIC_HEAT, LOGIC_HEAT, LOGIC_HEAT };
        uint8_t  ch4Enabled = 1;
        uint8_t  ch5Enabled = 1;
        uint8_t  reserved[2] = {};
    };

    struct OutputsBlobV2 {
        uint16_t version = 2;
        uint8_t  soundMuted = 0;
        uint8_t  chMode[3] = { LOGIC_HEAT, LOGIC_HEAT, LOGIC_HEAT };
        uint8_t  ch4Enabled = 1;
        uint8_t  ch5Enabled = 1;
        uint8_t  manualMain[3] = {};
        uint8_t  reserved = 0;
    };

    struct OutputsBlob {
        uint16_t version = OUTPUT_BLOB_VERSION;
        uint8_t  soundMuted = 0;
        uint8_t  chMode[3] = { LOGIC_HEAT, LOGIC_HEAT, LOGIC_HEAT };
        uint8_t  ch4Enabled = 1;
        uint8_t  ch5Enabled = 1;
        uint8_t  manualMain[3] = {};
        uint8_t  stopMainLatched = 0;
        uint8_t  operatorHoldOffMain[3] = {};
        uint8_t  reserved[5] = {};
    };

    bool   _nvsChecked = false;
    bool   _nvsReady = false;
    bool   _nvsRecovered = false;
    uint32_t _saveOutputsLastMs = 0;
    uint32_t _saveOutputsMaxMs = 0;
    String _lastStatus = "";

    template <typename BlobT>
    bool _readBlob(Preferences& p, const char* key, BlobT& blob, uint16_t expectedVersion) {
        if (p.getBytesLength(key) != sizeof(BlobT)) return false;
        if (p.getBytes(key, &blob, sizeof(blob)) != sizeof(blob)) return false;
        return blob.version == expectedVersion;
    }

    template <typename BlobT>
    bool _writeBlob(Preferences& p, const char* key, const BlobT& blob) {
        return p.putBytes(key, &blob, sizeof(blob)) == sizeof(blob);
    }

    void _fillSensorsBlob(SensorManager& sm, SensorsBlob& blob) {
        blob.version = SENSOR_BLOB_VERSION;
        blob.sensorCount = SEN_COUNT;
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;

            SensorBlob& dst = blob.sensors[si];
            dst.enabled = s->enabled ? 1 : 0;
            dst.periodMs = s->periodMs;
            dst.alarmDelayMs = s->alarmDelayMs;
            dst.ctrlDelayMs = s->ctrlDelayMs;

            for (int ai = 0; ai < N_ALARMS; ai++) {
                dst.alarm[ai].enabled = s->alarm[ai].enabled ? 1 : 0;
                dst.alarm[ai].threshold = s->alarm[ai].threshold;
                dst.alarm[ai].isMax = s->alarm[ai].isMax ? 1 : 0;
            }

            for (int oi = 0; oi < N_CTRL_OUT; oi++) {
                dst.ctrl[oi].enabled = s->ctrl[oi].enabled ? 1 : 0;
                dst.ctrl[oi].outIdx = s->ctrl[oi].outIdx;
                dst.ctrl[oi].logic = s->ctrl[oi].logic;
                dst.ctrl[oi].minVal = s->ctrl[oi].minVal;
                dst.ctrl[oi].maxVal = s->ctrl[oi].maxVal;
            }
        }
    }

    void _sanitizeSensors(SensorManager& sm) {
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;

            if (s->periodMs < SENSOR_PERIOD_MIN_MS) s->periodMs = SENSOR_PERIOD_MIN_MS;
            if (s->periodMs > SENSOR_PERIOD_MAX_MS) s->periodMs = SENSOR_PERIOD_MAX_MS;
            if (s->alarmDelayMs > SENSOR_ALARM_DELAY_MAX_MS) s->alarmDelayMs = SENSOR_ALARM_DELAY_MAX_MS;
            if (s->ctrlDelayMs > SENSOR_CTRL_DELAY_MAX_MS) s->ctrlDelayMs = SENSOR_CTRL_DELAY_MAX_MS;

            for (int oi = 0; oi < N_CTRL_OUT; oi++) {
                CtrlRule& r = s->ctrl[oi];
                if (r.outIdx >= N_CTRL_OUT) r.outIdx = oi;
                if (r.logic != LOGIC_HEAT && r.logic != LOGIC_COOL) r.logic = LOGIC_HEAT;
                if (isnan(r.minVal) || isnan(r.maxVal) ||
                    r.minVal + CTRL_MIN_DEADBAND > r.maxVal) {
                    r.minVal = 0.0f;
                    r.maxVal = 100.0f;
                }
                if (!SensorManager::isRuleAllowedForOutput((uint8_t)si, (uint8_t)oi)) {
                    r.enabled = false;
                }
            }
        }
        sm.normalizeDigitalOffOnlyRules();
        sm.normalizeSchemeControlRules();
    }

    template <typename BlobT>
    void _applySensorsBlobGeneric(SensorManager& sm, const BlobT& blob) {
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;

            const SensorBlob& src = blob.sensors[si];
            s->enabled = (src.enabled != 0);
            s->periodMs = src.periodMs;
            s->alarmDelayMs = src.alarmDelayMs;
            s->ctrlDelayMs = src.ctrlDelayMs;

            for (int ai = 0; ai < N_ALARMS; ai++) {
                s->alarm[ai].enabled = (src.alarm[ai].enabled != 0);
                s->alarm[ai].threshold = src.alarm[ai].threshold;
                s->alarm[ai].isMax = (src.alarm[ai].isMax != 0);
            }

            for (int oi = 0; oi < N_CTRL_OUT; oi++) {
                s->ctrl[oi].enabled = (src.ctrl[oi].enabled != 0);
                s->ctrl[oi].outIdx = src.ctrl[oi].outIdx;
                s->ctrl[oi].logic = src.ctrl[oi].logic;
                s->ctrl[oi].minVal = src.ctrl[oi].minVal;
                s->ctrl[oi].maxVal = src.ctrl[oi].maxVal;
            }
        }
    }

    bool _hasLegacySensorConfig(Preferences& p) {
        return p.isKey("s0_en") || p.isKey("s0_per") || p.isKey("s0_adel") || p.isKey("s0_cdel");
    }

    void _loadLegacySensors(SensorManager& sm, Preferences& p) {
        for (int si = 0; si < SEN_COUNT; si++) {
            SensorBase* s = sm.s[si];
            if (!s) continue;

            const String pref = String("s") + si + "_";

            s->enabled  = p.getBool((pref + "en").c_str(), s->enabled);
            s->periodMs = p.getUInt((pref + "per").c_str(), s->periodMs);
            s->alarmDelayMs = p.getUInt((pref + "adel").c_str(), s->alarmDelayMs);
            s->ctrlDelayMs  = p.getUInt((pref + "cdel").c_str(), s->ctrlDelayMs);

            for (int ai = 0; ai < N_ALARMS; ai++) {
                const String ap = pref + "a" + ai + "_";
                s->alarm[ai].enabled   = p.getBool ((ap + "en").c_str(),  false);
                s->alarm[ai].threshold = p.getFloat((ap + "thr").c_str(), 0.0f);
                s->alarm[ai].isMax     = p.getBool ((ap + "max").c_str(), true);
            }

            for (int oi = 0; oi < N_CTRL_OUT; oi++) {
                const String cp = pref + "c" + oi + "_";
                s->ctrl[oi].enabled = p.getBool ((cp + "en").c_str(),  false);
                s->ctrl[oi].outIdx  = p.getUChar((cp + "out").c_str(), oi);
                s->ctrl[oi].logic   = p.getUChar((cp + "log").c_str(), LOGIC_HEAT);
                s->ctrl[oi].minVal  = p.getFloat((cp + "min").c_str(), 0.0f);
                s->ctrl[oi].maxVal  = p.getFloat((cp + "max").c_str(), 100.0f);
            }
        }
    }

    void _migrateSensorsToBlob(SensorManager& sm) {
        _sanitizeSensors(sm);
        SensorsBlob blob{};
        _fillSensorsBlob(sm, blob);

        Preferences p;
        if (!_openPrefs(p, "sensors", false)) return;
        p.clear();
        if (!_writeBlob(p, "blob", blob)) {
            _lastStatus = "Preferences write failed for namespace 'sensors'";
        }
        p.end();
    }

    void _fillOutputsBlob(OutputManager& om, OutputsBlob& blob) {
        blob.version = OUTPUT_BLOB_VERSION;
        blob.soundMuted = om.soundMuted ? 1 : 0;
        blob.chMode[0] = om.chMode[0];
        blob.chMode[1] = om.chMode[1];
        blob.chMode[2] = om.chMode[2];
        blob.ch4Enabled = om.ch4Enabled ? 1 : 0;
        blob.ch5Enabled = om.ch5Enabled ? 1 : 0;
        blob.manualMain[0] = om.manualWanted(OUT_CH1) ? 1 : 0;
        blob.manualMain[1] = om.manualWanted(OUT_CH2) ? 1 : 0;
        blob.manualMain[2] = om.manualWanted(OUT_CH3) ? 1 : 0;
        blob.stopMainLatched = 0;
        blob.operatorHoldOffMain[0] = om.operatorHoldOff(OUT_CH1) ? 1 : 0;
        blob.operatorHoldOffMain[1] = om.operatorHoldOff(OUT_CH2) ? 1 : 0;
        blob.operatorHoldOffMain[2] = om.operatorHoldOff(OUT_CH3) ? 1 : 0;
    }

    void _sanitizeOutputs(OutputManager& om) {
        for (uint8_t i = 0; i < 3; i++) {
            if (om.chMode[i] != LOGIC_HEAT && om.chMode[i] != LOGIC_COOL) {
                om.chMode[i] = LOGIC_HEAT;
            }
        }
    }

    void _applyOutputsBlob(OutputManager& om, const OutputsBlob& blob) {
        om.soundMuted = (blob.soundMuted != 0);
        om.chMode[0] = blob.chMode[0];
        om.chMode[1] = blob.chMode[1];
        om.chMode[2] = blob.chMode[2];
        om.ch4Enabled = (blob.ch4Enabled != 0);
        om.ch5Enabled = (blob.ch5Enabled != 0);
        om.restoreManualState(OUT_CH1, blob.manualMain[0] != 0);
        om.restoreManualState(OUT_CH2, blob.manualMain[1] != 0);
        om.restoreManualState(OUT_CH3, blob.manualMain[2] != 0);
        om.restoreMainStopLatched(false);
        om.restoreOperatorHoldOff(OUT_CH1, blob.operatorHoldOffMain[0] != 0);
        om.restoreOperatorHoldOff(OUT_CH2, blob.operatorHoldOffMain[1] != 0);
        om.restoreOperatorHoldOff(OUT_CH3, blob.operatorHoldOffMain[2] != 0);
    }

    void _applyOutputsBlob(OutputManager& om, const OutputsBlobV2& blob) {
        om.soundMuted = (blob.soundMuted != 0);
        om.chMode[0] = blob.chMode[0];
        om.chMode[1] = blob.chMode[1];
        om.chMode[2] = blob.chMode[2];
        om.ch4Enabled = (blob.ch4Enabled != 0);
        om.ch5Enabled = (blob.ch5Enabled != 0);
        om.restoreManualState(OUT_CH1, blob.manualMain[0] != 0);
        om.restoreManualState(OUT_CH2, blob.manualMain[1] != 0);
        om.restoreManualState(OUT_CH3, blob.manualMain[2] != 0);
        om.restoreMainStopLatched(false);
        om.restoreOperatorHoldOff(OUT_CH1, false);
        om.restoreOperatorHoldOff(OUT_CH2, false);
        om.restoreOperatorHoldOff(OUT_CH3, false);
    }

    void _applyOutputsBlob(OutputManager& om, const OutputsBlobV1& blob) {
        om.soundMuted = (blob.soundMuted != 0);
        om.chMode[0] = blob.chMode[0];
        om.chMode[1] = blob.chMode[1];
        om.chMode[2] = blob.chMode[2];
        om.ch4Enabled = (blob.ch4Enabled != 0);
        om.ch5Enabled = (blob.ch5Enabled != 0);
        om.restoreManualState(OUT_CH1, false);
        om.restoreManualState(OUT_CH2, false);
        om.restoreManualState(OUT_CH3, false);
        om.restoreMainStopLatched(false);
        om.restoreOperatorHoldOff(OUT_CH1, false);
        om.restoreOperatorHoldOff(OUT_CH2, false);
        om.restoreOperatorHoldOff(OUT_CH3, false);
    }

    bool _hasLegacyOutputConfig(Preferences& p) {
        return p.isKey("muted") || p.isKey("ch1_mode") || p.isKey("ch4_en") || p.isKey("ch5_en");
    }

    void _loadLegacyOutputs(OutputManager& om, Preferences& p) {
        om.soundMuted   = p.getBool("muted", false);
        om.chMode[0]    = p.getUChar("ch1_mode", LOGIC_HEAT);
        om.chMode[1]    = p.getUChar("ch2_mode", LOGIC_HEAT);
        om.chMode[2]    = p.getUChar("ch3_mode", LOGIC_HEAT);
        om.ch4Enabled   = p.getBool("ch4_en", true);
        om.ch5Enabled   = p.getBool("ch5_en", true);
        om.restoreManualState(OUT_CH1, false);
        om.restoreManualState(OUT_CH2, false);
        om.restoreManualState(OUT_CH3, false);
        om.restoreMainStopLatched(false);
        om.restoreOperatorHoldOff(OUT_CH1, false);
        om.restoreOperatorHoldOff(OUT_CH2, false);
        om.restoreOperatorHoldOff(OUT_CH3, false);
    }

    void _migrateOutputsToBlob(OutputManager& om) {
        OutputsBlob blob{};
        _fillOutputsBlob(om, blob);

        Preferences p;
        if (!_openPrefs(p, "outputs", false)) return;
        p.clear();
        if (!_writeBlob(p, "blob", blob)) {
            _lastStatus = "Preferences write failed for namespace 'outputs'";
        }
        p.end();
    }

    bool _ensureNvs() {
        if (_nvsChecked) return _nvsReady;
        _nvsChecked = true;

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            _nvsRecovered = true;
            _lastStatus = String("NVS recovered after ") + esp_err_to_name(err) + "; stored settings were reset";
            err = nvs_flash_erase();
            if (err == ESP_OK) err = nvs_flash_init();
        }

        _nvsReady = (err == ESP_OK);
        if (_nvsReady) {
            if (_lastStatus.length() == 0) _lastStatus = "ok";
            return true;
        }

        _lastStatus = String("NVS init failed: ") + esp_err_to_name(err);
        return false;
    }

    bool _openPrefs(Preferences& p, const char* ns, bool readOnly) {
        if (!_ensureNvs()) return false;
        if (p.begin(ns, readOnly)) return true;
        _lastStatus = String("Preferences open failed for namespace '") + ns + "'";
        return false;
    }
};
