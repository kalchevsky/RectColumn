// ================================================================
// Sensors.h  
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <math.h>

#include "config.h"

// ─── Alarm threshold ─────────────────────────────────────────────
struct AlarmLevel {
    bool  enabled   = false;
    float threshold = 0.0f;
    bool  isMax     = true;
    bool  triggered = false;
};

// ─── Control rule linking one sensor to one output ───────────────
struct CtrlRule {
    bool    enabled = false;
    uint8_t outIdx  = 0;
    uint8_t logic   = LOGIC_HEAT;
    float   minVal  = 0.0f;
    float   maxVal  = 100.0f;
};

#define N_ALARMS   4
#define N_CTRL_OUT OUT_COUNT
static constexpr uint8_t SENSOR_LOST_ALARM_BIT  = 7;
static constexpr uint8_t SENSOR_LOST_ALARM_MASK = (1u << SENSOR_LOST_ALARM_BIT);

enum SensorErrorReason : uint8_t {
    SENSOR_ERR_NONE = 0,
    SENSOR_ERR_NO_RESPONSE,
    SENSOR_ERR_OUT_OF_RANGE,
    SENSOR_ERR_NAN,
    SENSOR_ERR_TIMEOUT,
};

enum class SensorOperatorResetResult : uint8_t {
    None = 0,
    Restored,
    Relatched,
};

class SensorBase {
public:
    String   name;
    bool     enabled  = true;
    uint32_t periodMs = DEF_T_PERIOD_MS;
    bool     error    = false;
    bool     present  = false;
    bool     hwLimited = false;
    uint8_t  diagCode = SENSOR_DIAG_NONE;
    SensorErrorReason sensorErrorReason = SENSOR_ERR_NONE;
    bool     sensorErrorLatched = false;
    float    value    = NAN;
    uint32_t lastValidMs = 0;
    uint32_t maxAgeMs = 0;

    AlarmLevel alarm[N_ALARMS];
    CtrlRule   ctrl[N_CTRL_OUT];

    // Дополнительные задержки для интерфейса/логики.
    // alarmDelayMs — задержка подтверждения сигнализации.
    // ctrlDelayMs  — задержка применения команды управления.
    uint32_t alarmDelayMs = 0;
    uint32_t ctrlDelayMs  = 0;

    // Для некоторых аналоговых датчиков пороги в UI задаются не в "сырых"
    // единицах ADC, а в процентах. Тогда 0..100 автоматически
    // переводятся во внутренний диапазон 0..4095.
    bool     thresholdPercentInput = false;

    // Для F (и при необходимости других датчиков) можно подменить
    // обычную логику сигнализации внешней вычисленной логикой.
    bool    externalAlarmLogic = false;
    uint8_t externalAlarmMaskBits = 0;

    uint32_t _lastPollMs = 0;

    // === PATCH WARMUP BEGIN ===
    uint32_t _enableWarmupUntilMs = 0;

    void startEnableWarmup(uint32_t durationMs) {
        uint32_t target = millis() + durationMs;
        if (target == 0) target = 1;
        _enableWarmupUntilMs = target;
    }

    void clearEnableWarmup() {
        _enableWarmupUntilMs = 0;
    }

    bool isInEnableWarmup() const {
        if (_enableWarmupUntilMs == 0) return false;
        return (int32_t)(millis() - _enableWarmupUntilMs) < 0;
    }
    // === PATCH WARMUP END ===

    explicit SensorBase(const String& n, bool trackSensorLoss = false)
        : name(n), _trackSensorLoss(trackSensorLoss)
    {
        for (uint8_t i = 0; i < N_CTRL_OUT; i++) {
            ctrl[i].outIdx = i;
            _ctrlCandidateCmd[i] = 0;
            _ctrlCandidateSinceMs[i] = 0;
            _ctrlRearmPollAfterMs[i] = 0;
        }
        for (uint8_t i = 0; i < N_ALARMS; i++) {
            _alarmCandidateSinceMs[i] = 0;
        }
    }
    virtual ~SensorBase() = default;

    bool isDue() const {
        return enabled && ((millis() - _lastPollMs) >= periodMs);
    }

    bool checkAlarms() {
        bool changed = false;
        const uint32_t now = millis();

        if (!enabled) {
            for (uint8_t i = 0; i < N_ALARMS; i++) {
                if (alarm[i].triggered) changed = true;
            }
            resetAlarmRuntime();
            return changed;
        }

        for (uint8_t i = 0; i < N_ALARMS; i++) {
            AlarmLevel& a = alarm[i];
            const bool prev = a.triggered;

            if (externalAlarmLogic) {
                a.triggered = a.enabled && ((externalAlarmMaskBits & (1u << i)) != 0);
                _alarmCandidateSinceMs[i] = 0;
            } else if (!a.enabled) {
                a.triggered = false;
                _alarmCandidateSinceMs[i] = 0;
            } else if (_trackSensorLoss && sensorErrorLatched) {
                a.triggered = false;
                _alarmCandidateSinceMs[i] = 0;
            } else if (isnan(value)) {
                a.triggered = false;
                _alarmCandidateSinceMs[i] = 0;
            } else {
                const float cmpThreshold = effectiveThreshold(a.threshold);
                const bool rawTriggered = a.isMax ? (value > cmpThreshold)
                                                  : (value < cmpThreshold);
                if (!rawTriggered) {
                    a.triggered = false;
                    _alarmCandidateSinceMs[i] = 0;
                } else if (alarmDelayMs == 0) {
                    a.triggered = true;
                } else {
                    if (_alarmCandidateSinceMs[i] == 0) _alarmCandidateSinceMs[i] = now;
                    a.triggered = ((now - _alarmCandidateSinceMs[i]) >= alarmDelayMs);
                }
            }

            if (a.triggered != prev) changed = true;
        }
        return changed;
    }

    bool anyAlarmActive() const { return alarmMask() != 0; }

    uint8_t userAlarmMask() const {
        uint8_t mask = 0;
        for (uint8_t i = 0; i < N_ALARMS; i++) {
            if (alarm[i].enabled && alarm[i].triggered) mask |= (1u << i);
        }
        return mask;
    }

    uint8_t alarmMask() const {
        uint8_t mask = userAlarmMask();
        if (hasSensorLostAlarm()) mask |= SENSOR_LOST_ALARM_MASK;
        return mask;
    }

    bool tracksSensorLoss() const { return _trackSensorLoss; }
    bool hasSensorLostAlarm() const { return _trackSensorLoss && sensorErrorLatched; }
    bool operatorResetArmed() const { return _operatorResetArmed; }
    const char* sensorErrorReasonCode() const { return _sensorErrorReasonCode(sensorErrorReason); }

    String sensorLostNotice() const {
        return hasSensorLostAlarm() ? (String("Потеря датчика ") + name) : String("");
    }

    void armOperatorResetCycle() {
        if (_trackSensorLoss) _operatorResetArmed = true;
    }

    SensorOperatorResetResult applyOperatorResetCycle() {
        if (!_trackSensorLoss || !_operatorResetArmed) return SensorOperatorResetResult::None;
        _operatorResetArmed = false;
        if (!sensorErrorLatched) return SensorOperatorResetResult::None;

        if (_canClearLatchedErrorNow()) {
            sensorErrorLatched = false;
            sensorErrorReason = SENSOR_ERR_NONE;
            return SensorOperatorResetResult::Restored;
        }

        // FIX: sensorErrorLatched must stay true until sensor is genuinely restored.
        // Previously this line reset it to true even after a healthy reading,
        // which prevented the sticky error from being cleared by the operator
        // cycle until the next fault occurred.
        return SensorOperatorResetResult::Relatched;
    }

    bool hasUsableValue() const {
        return enabled && present && !error && !sensorErrorLatched && !isnan(value) && !isStale();
    }

    bool controlRuleEnabled(uint8_t outIdx) const {
        return outIdx < N_CTRL_OUT &&
               ctrl[outIdx].enabled &&
               ctrl[outIdx].outIdx == outIdx;
    }

    bool affectsOutput(uint8_t outIdx) const {
        return enabled && controlRuleEnabled(outIdx);
    }

    void resetControlRuntime(uint8_t outIdx) {
        if (outIdx >= N_CTRL_OUT) return;
        _ctrlCandidateCmd[outIdx] = 0;
        _ctrlCandidateSinceMs[outIdx] = 0;
        _ctrlRearmPollAfterMs[outIdx] = 0;
    }

    void resetAllControlRuntime() {
        for (uint8_t i = 0; i < N_CTRL_OUT; i++) resetControlRuntime(i);
    }

    void rearmControlAfterFreshPoll(uint8_t outIdx, uint32_t now = millis()) {
        if (outIdx >= N_CTRL_OUT) return;
        _ctrlCandidateCmd[outIdx] = 0;
        _ctrlCandidateSinceMs[outIdx] = 0;
        _ctrlRearmPollAfterMs[outIdx] = now ? now : 1;
    }

    void rearmAllControlAfterFreshPoll(uint32_t now = millis()) {
        for (uint8_t i = 0; i < N_CTRL_OUT; i++) rearmControlAfterFreshPoll(i, now);
    }

    int8_t controlRuntimeCmd(uint8_t outIdx) const {
        if (outIdx >= N_CTRL_OUT) return 0;
        return _ctrlCandidateCmd[outIdx];
    }

    uint32_t controlRuntimeSinceMs(uint8_t outIdx) const {
        if (outIdx >= N_CTRL_OUT) return 0;
        return _ctrlCandidateSinceMs[outIdx];
    }

    uint32_t controlRuntimeElapsedMs(uint8_t outIdx, uint32_t now = millis()) const {
        if (outIdx >= N_CTRL_OUT) return 0;
        const uint32_t startedAt = _ctrlCandidateSinceMs[outIdx];
        if (startedAt == 0) return 0;
        return now - startedAt;
    }

    void resetAlarmRuntime() {
        for (uint8_t i = 0; i < N_ALARMS; i++) {
            alarm[i].triggered = false;
            _alarmCandidateSinceMs[i] = 0;
        }
        externalAlarmMaskBits = 0;
    }

    bool isStale() const {
        if (isnan(value)) return false;
        const uint32_t ageLimit = maxAgeMs ? maxAgeMs : _defaultMaxAgeMs();
        if (ageLimit == 0 || lastValidMs == 0) return false;
        return (millis() - lastValidMs) > ageLimit;
    }

    float effectiveThreshold(float threshold) const {
        if (thresholdPercentInput && threshold >= 0.0f && threshold <= 100.0f) {
            return (threshold * 4095.0f) / 100.0f;
        }
        return threshold;
    }

    int evalCtrl(uint8_t outIdx, bool invalidMeansOff = true, bool controlGate = true) {
        if (outIdx >= N_CTRL_OUT) return 0;

        // === PATCH WARMUP BEGIN ===
        if (isInEnableWarmup()) {
            resetControlRuntime(outIdx);
            return 0;
        }
        // === PATCH WARMUP END ===

        const CtrlRule& r = ctrl[outIdx];
        if (!r.enabled || r.outIdx != outIdx) {
            resetControlRuntime(outIdx);
            return 0;
        }

        if (!controlGate) {
            // Gate only freezes the current delay budget. The runtime must stay
            // intact so a short confirmation dip does not restart ctrlDelayMs.
            return 0;
        }

        if (!enabled) {
            resetControlRuntime(outIdx);
            return 0;
        }

        if (_ctrlRearmPollAfterMs[outIdx] != 0) {
            _ctrlCandidateCmd[outIdx] = 0;
            _ctrlCandidateSinceMs[outIdx] = 0;
            if (_lastPollMs < _ctrlRearmPollAfterMs[outIdx]) return 0;
            _ctrlRearmPollAfterMs[outIdx] = 0;
        }

        if (!hasUsableValue()) {
            resetControlRuntime(outIdx);
            return invalidMeansOff ? -1 : 0;
        }

        const float minVal = effectiveThreshold(r.minVal);
        const float maxVal = effectiveThreshold(r.maxVal);
        int cmd = 0;
        if (r.logic == LOGIC_HEAT) {
            if (value < minVal) cmd = 1;
            else if (value > maxVal) cmd = -1;
        } else {
            if (value > maxVal) cmd = 1;
            else if (value < minVal) cmd = -1;
        }

        if (cmd == 0) {
            resetControlRuntime(outIdx);
            return 0;
        }
        if (ctrlDelayMs == 0) return cmd;

        const uint32_t now = millis();
        if (_ctrlCandidateCmd[outIdx] != cmd) {
            _ctrlCandidateCmd[outIdx] = cmd;
            _ctrlCandidateSinceMs[outIdx] = now;
            return 0;
        }
        if (_ctrlCandidateSinceMs[outIdx] == 0) _ctrlCandidateSinceMs[outIdx] = now;
        if ((now - _ctrlCandidateSinceMs[outIdx]) >= ctrlDelayMs) return cmd;
        return 0;
    }

    virtual void begin() = 0;
    virtual void poll()  = 0;

    virtual const char* diagText() const {
        switch (diagCode) {
            case SENSOR_DIAG_ADC2_WIFI_CONFLICT:         return "ADC2 недоступен при активном WiFi";
            case SENSOR_DIAG_GPIO35_RESERVED_FOR_WER_CH2:return "GPIO35 зарезервирован под WER_CH2 в этой сборке";
            case SENSOR_DIAG_TEMP_RECOVERY_HOLD:         return "Датчик восстановился, удерживается индикация ошибки";
            default:                                     return "";
        }
    }

protected:
    void markSensorFault(SensorErrorReason reason, uint32_t now, bool presentNow) {
        (void)now;
        present = presentNow;
        error = true;
        diagCode = SENSOR_DIAG_NONE;
        if (_trackSensorLoss) {
            sensorErrorReason = reason;
            sensorErrorLatched = true;
        }
        _healthySinceMs = 0;
    }

    void markSensorHealthy(uint32_t now, bool presentNow = true) {
        present = presentNow;
        if (_trackSensorLoss && error) {
            if (_healthySinceMs == 0) _healthySinceMs = now;
            if ((now - _healthySinceMs) < SENSOR_HEALTHY_HYSTERESIS_MS) {
                error = true;
                diagCode = SENSOR_DIAG_TEMP_RECOVERY_HOLD;
                return;
            }
            _healthySinceMs = 0;
        } else {
            _healthySinceMs = 0;
        }

        error = false;
        diagCode = SENSOR_DIAG_NONE;
        if (!_trackSensorLoss || !sensorErrorLatched) sensorErrorReason = SENSOR_ERR_NONE;
    }

protected:
    uint32_t _alarmCandidateSinceMs[N_ALARMS] = {};
    int8_t   _ctrlCandidateCmd[N_CTRL_OUT] = {};
    uint32_t _ctrlCandidateSinceMs[N_CTRL_OUT] = {};
    uint32_t _ctrlRearmPollAfterMs[N_CTRL_OUT] = {};
    bool     _trackSensorLoss = false;
    bool     _operatorResetArmed = false;
    uint32_t _healthySinceMs = 0;

private:
    uint32_t _defaultMaxAgeMs() const {
        if (periodMs == 0) return 0;
        uint32_t factor = SENSOR_MAX_AGE_FACTOR;
        if (factor < 1) factor = 1;
        return periodMs * factor;
    }

    bool _canClearLatchedErrorNow() const {
        return enabled && present && !error && !isnan(value) && !isStale();
    }

    static const char* _sensorErrorReasonCode(SensorErrorReason reason) {
        switch (reason) {
            case SENSOR_ERR_NO_RESPONSE: return "no_response";
            case SENSOR_ERR_OUT_OF_RANGE:return "out_of_range";
            case SENSOR_ERR_NAN:         return "nan";
            case SENSOR_ERR_TIMEOUT:     return "timeout";
            default:                     return "";
        }
    }
};

#include <OneWire.h>
#include <DallasTemperature.h>

class TempSensor : public SensorBase {
public:
    TempSensor(const String& n, uint8_t pin)
        : SensorBase(n, true), _ow(pin), _dt(&_ow)
    {
        periodMs = DEF_T_PERIOD_MS;
    }

    void begin() override {
        _dt.begin();
        _dt.setResolution(11);
        _dt.setWaitForConversion(false);
        _conversionWaitMs = 375UL; // 11-bit DS18B20 conversion time
        present = (_dt.getDeviceCount() > 0);
        error   = !present;
        sensorErrorReason = present ? SENSOR_ERR_NONE : SENSOR_ERR_NO_RESPONSE;
        sensorErrorLatched = !present;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
        lastValidMs = 0;
        _conversionPending = false;
        _conversionStartedMs = 0;
        _lastPollMs = millis() - periodMs; // first sample can start immediately
        _healthySinceMs = 0;

        // FIX: markSensorFault must be called so hasSensorLostAlarm() returns
        // true immediately when sensor is physically absent at boot. Without this,
        // sensorErrorLatched is set but hasSensorLostAlarm() stays false until
        // poll() runs a full cycle, delaying the alarm and notification.
        if (!present) {
            markSensorFault(SENSOR_ERR_NO_RESPONSE, millis(), false);
        }
    }

    bool isConversionPending() const { return _conversionPending; }

    bool isReadyToRead(uint32_t now) const {
        return enabled && _conversionPending && ((uint32_t)(now - _conversionStartedMs) >= _conversionWaitMs);
    }

    bool isDueToStart(uint32_t now) const {
        // FIX: When sensor is physically absent (present=false), stop requesting
        // conversions in a tight loop. The sensor should stay in the "lost" state
        // until physically reconnected. This prevents the "T1 подключён" log entries
        // that appear when _ensurePresent() returns false and the conversion starts
        // anyway on the next cycle.
        if (!present) return false;

        const uint32_t cappedHealthyMs =
            (periodMs < SENSOR_LOST_TIMEOUT_DS_MS) ? periodMs : SENSOR_LOST_TIMEOUT_DS_MS;
        const uint32_t retryMs = (error || !present) ? 1000UL : cappedHealthyMs;
        return enabled && !_conversionPending && ((uint32_t)(now - _lastPollMs) >= retryMs);
    }

    bool startConversion(uint32_t now) {
        if (!_ensurePresent()) {
            _lastPollMs = now;
            _conversionPending = false;
            return false;
        }
        _dt.requestTemperatures();
        _conversionPending = true;
        _conversionStartedMs = now;
        return true;
    }

    bool readConversion(uint32_t now) {
        if (!_conversionPending) return false;
        if ((uint32_t)(now - _conversionStartedMs) < _conversionWaitMs) return false;

        _conversionPending = false;
        _lastPollMs = now;

        if (!_ensurePresent()) {
            return true;
        }

        float t = _dt.getTempCByIndex(0);
        if (t == DEVICE_DISCONNECTED_C) {
            present = false;
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NO_RESPONSE, now, false);
            return true;
        }

        if (isnan(t)) {
            present = true;
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NAN, now, true);
            return true;
        }

        if (t < -55.0f || t > 125.0f) {
            present = true;
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_OUT_OF_RANGE, now, true);
            return true;
        }

        present = true;
        value = t;
        lastValidMs = now;
        hwLimited = false;
        markSensorHealthy(now, true);
        return true;
    }

    void poll() override {
        const uint32_t now = millis();
        if (isReadyToRead(now)) {
            (void)readConversion(now);
        } else if (isDueToStart(now)) {
            (void)startConversion(now);
        }
    }

private:
    bool _ensurePresent() {
        if (_dt.getDeviceCount() == 0) _dt.begin();
        const bool ok = (_dt.getDeviceCount() > 0);
        present = ok;
        if (!ok) {
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NO_RESPONSE, millis(), false);
        }
        return ok;
    }

    OneWire           _ow;
    DallasTemperature _dt;
    bool              _conversionPending = false;
    uint32_t          _conversionStartedMs = 0;
    uint32_t          _conversionWaitMs = 375UL;
};

class VirtualSensor : public SensorBase {
public:
    VirtualSensor(SensorBase* t1, SensorBase* t2)
        : SensorBase("dT"), _t1(t1), _t2(t2)
    {
        periodMs = DEF_T_PERIOD_MS;
        present  = true;
        lastValidMs = 0;
    }

    void begin() override { present = true; }

    void poll() override {
        _lastPollMs = millis();
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;

        if (!enabled) {
            present = true;
            value = NAN;
            error = false;
            resetAlarmRuntime();
            resetAllControlRuntime();
            return;
        }

        present = true;

        if (!isnan(_t1->value) && !isnan(_t2->value) &&
            !_t1->error && !_t2->error &&
            !_t1->sensorErrorLatched && !_t2->sensorErrorLatched &&
            _t1->present && _t2->present &&
            _t1->enabled && _t2->enabled) {
            value = _t2->value - _t1->value;
            lastValidMs = _lastPollMs;
            error = false;
        } else {
            value = NAN;
            error = true;
        }
    }

private:
    SensorBase* _t1;
    SensorBase* _t2;
};

#include <Wire.h>
#include <Adafruit_BMP085.h>

class PressureSensor : public SensorBase {
public:
    PressureSensor() : SensorBase("P", true) {
        periodMs = DEF_P_PERIOD_MS;
    }

    void begin() override {
        Wire.begin(PIN_BMP_SDA, PIN_BMP_SCL);
        present = _bmp.begin(BMP085_ULTRALOWPOWER);
        error   = !present;
        sensorErrorReason = present ? SENSOR_ERR_NONE : SENSOR_ERR_NO_RESPONSE;
        sensorErrorLatched = !present;
        lastValidMs = 0;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
        _healthySinceMs = 0;

        // FIX: markSensorFault must be called so hasSensorLostAlarm() returns
        // true immediately when sensor is physically absent at boot.
        if (!present) {
            markSensorFault(SENSOR_ERR_NO_RESPONSE, millis(), false);
        }
    }

    bool isDueToPoll(uint32_t now) const {
        const uint32_t cappedHealthyMs =
            (periodMs < SENSOR_LOST_TIMEOUT_MS) ? periodMs : SENSOR_LOST_TIMEOUT_MS;
        const uint32_t retryMs = (error || !present) ? 1000UL : cappedHealthyMs;
        return enabled && ((uint32_t)(now - _lastPollMs) >= retryMs);
    }

    void poll() override {
        const uint32_t now = millis();
        _lastPollMs = now;

        if (!present) present = _bmp.begin(BMP085_ULTRALOWPOWER);
        if (!present) {
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NO_RESPONSE, now, false);
            return;
        }

        value = _bmp.readPressure() / 100.0f;
        if (isnan(value)) {
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NAN, now, true);
            return;
        }

        if (value < PRESSURE_SANITY_MIN_HPA || value > PRESSURE_SANITY_MAX_HPA) {
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_OUT_OF_RANGE, now, true);
            return;
        }

        lastValidMs = _lastPollMs;
        hwLimited = false;
        markSensorHealthy(now, true);
    }

private:
    Adafruit_BMP085 _bmp;
};

class DigitalSensor : public SensorBase {
public:
    DigitalSensor(const String& n, uint8_t pin, bool trackSensorLoss = false)
        : SensorBase(n, trackSensorLoss), _pin(pin), _trackSensorLoss(trackSensorLoss)
    {
        periodMs = DEF_FAST_PERIOD_MS;
    }

    void begin() override {
        // L/F работают от pull-down логики: HIGH = норма, LOW = авария.
        // Для GPIO19 и GPIO23 используем внутреннюю подтяжку к GND.
        pinMode(_pin, INPUT_PULLDOWN);
        _lastPollMs = millis();
        present = true;
        error   = false;
        sensorErrorReason = SENSOR_ERR_NONE;
        sensorErrorLatched = false;
        value   = (digitalRead(_pin) == HIGH) ? 1.0f : 0.0f;
        lastValidMs = _lastPollMs;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

    bool isDueToPoll(uint32_t now) const {
        if (!_trackSensorLoss) return enabled && ((uint32_t)(now - _lastPollMs) >= periodMs);
        const uint32_t cappedHealthyMs =
            (periodMs < SENSOR_LOST_TIMEOUT_MS) ? periodMs : SENSOR_LOST_TIMEOUT_MS;
        const uint32_t retryMs = error ? 1000UL : cappedHealthyMs;
        return enabled && ((uint32_t)(now - _lastPollMs) >= retryMs);
    }

    void poll() override {
        const uint32_t now = millis();
        _lastPollMs = now;
        const bool high = (digitalRead(_pin) == HIGH);
        if (_trackSensorLoss && !high) {
            present = false;
            value = NAN;
            hwLimited = false;
            markSensorFault(SENSOR_ERR_NO_RESPONSE, now, false);
            return;
        }

        present = true;
        value = high ? 1.0f : 0.0f;
        lastValidMs = _lastPollMs;
        hwLimited = false;
        markSensorHealthy(now, true);
    }

    bool isCircuitClosed() const { return value > 0.5f; }
    bool isCircuitOpen() const { return !isCircuitClosed(); }
    bool isActive() const { return isCircuitClosed(); }

private:
    uint8_t _pin;
    bool    _trackSensorLoss = false;
};

class AnalogSensor : public SensorBase {
public:
    AnalogSensor(const String& n, uint8_t pin, bool adc2WifiSensitive, bool disabledByConfig)
        : SensorBase(n),
          _pin(pin),
          _adc2WifiSensitive(adc2WifiSensitive),
          _disabledByConfig(disabledByConfig)
    {
        periodMs = DEF_FAST_PERIOD_MS;
    }

    void begin() override {
        present = !_disabledByConfig;
        error   = _disabledByConfig;
        hwLimited = _disabledByConfig;
        diagCode  = _disabledByConfig ? SENSOR_DIAG_GPIO35_RESERVED_FOR_WER_CH2 : SENSOR_DIAG_NONE;
        lastValidMs = 0;
        if (_disabledByConfig) value = NAN;
    }

    void poll() override {
        _lastPollMs = millis();

        if (_disabledByConfig) {
            present = false;
            error   = true;
            hwLimited = true;
            diagCode = SENSOR_DIAG_GPIO35_RESERVED_FOR_WER_CH2;
            value = NAN;
            return;
        }

        if (_adc2WifiSensitive && WiFi.getMode() != WIFI_OFF) {
            present = true;
            error   = true;
            hwLimited = true;
            diagCode = SENSOR_DIAG_ADC2_WIFI_CONFLICT;
            value = NAN;
            return;
        }

        int sum = 0;
        for (int i = 0; i < 4; i++) sum += analogRead(_pin);
        value = sum / 4.0f;
        lastValidMs = _lastPollMs;
        present = true;
        error = false;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

private:
    uint8_t _pin;
    bool    _adc2WifiSensitive = false;
    bool    _disabledByConfig  = false;
};
