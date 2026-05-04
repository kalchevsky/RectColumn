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

class SensorBase {
public:
    String   name;
    bool     enabled  = true;
    uint32_t periodMs = DEF_T_PERIOD_MS;
    bool     error    = false;
    bool     present  = false;
    bool     hwLimited = false;
    uint8_t  diagCode = SENSOR_DIAG_NONE;
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

    explicit SensorBase(const String& n) : name(n) {
        for (uint8_t i = 0; i < N_CTRL_OUT; i++) {
            ctrl[i].outIdx = i;
            _ctrlCandidateCmd[i] = 0;
            _ctrlCandidateSinceMs[i] = 0;
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
        int primaryErrorAlarmIdx = -1;

        // Ошибка/пропадание датчика должны поднимать сигнализацию.
        // Чтобы не зажигать сразу все уровни AL1/AL2, используем только
        // первый включённый alarm-slot как "аварию датчика".
        if (!externalAlarmLogic && (error || !present)) {
            for (uint8_t i = 0; i < N_ALARMS; i++) {
                if (alarm[i].enabled) {
                    primaryErrorAlarmIdx = (int)i;
                    break;
                }
            }
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
            } else if (primaryErrorAlarmIdx >= 0) {
                a.triggered = ((int)i == primaryErrorAlarmIdx);
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

    uint8_t alarmMask() const {
        uint8_t mask = 0;
        for (uint8_t i = 0; i < N_ALARMS; i++) {
            if (alarm[i].enabled && alarm[i].triggered) mask |= (1u << i);
        }
        return mask;
    }

    bool hasUsableValue() const {
        return enabled && present && !error && !isnan(value) && !isStale();
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

    int evalCtrl(uint8_t outIdx) {
        if (outIdx >= N_CTRL_OUT) return 0;

        const CtrlRule& r = ctrl[outIdx];
        if (!r.enabled || r.outIdx != outIdx) {
            _ctrlCandidateCmd[outIdx] = 0;
            _ctrlCandidateSinceMs[outIdx] = 0;
            return 0;
        }

        if (!hasUsableValue()) {
            _ctrlCandidateCmd[outIdx] = 0;
            _ctrlCandidateSinceMs[outIdx] = 0;
            return -1;
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
            _ctrlCandidateCmd[outIdx] = 0;
            _ctrlCandidateSinceMs[outIdx] = 0;
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
            case SENSOR_DIAG_TEMP_RECOVERY_HOLD:         return "Датчик температуры восстановился, удерживается индикация ошибки";
            default:                                     return "";
        }
    }

private:
    uint32_t _alarmCandidateSinceMs[N_ALARMS] = {};
    int8_t   _ctrlCandidateCmd[N_CTRL_OUT] = {};
    uint32_t _ctrlCandidateSinceMs[N_CTRL_OUT] = {};

    uint32_t _defaultMaxAgeMs() const {
        if (periodMs == 0) return 0;
        uint32_t factor = SENSOR_MAX_AGE_FACTOR;
        if (factor < 1) factor = 1;
        return periodMs * factor;
    }
};

#include <OneWire.h>
#include <DallasTemperature.h>

class TempSensor : public SensorBase {
public:
    TempSensor(const String& n, uint8_t pin)
        : SensorBase(n), _ow(pin), _dt(&_ow)
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
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
        lastValidMs = 0;
        _recoveryHoldUntilMs = 0;
        _faultSeen = false;
        _conversionPending = false;
        _conversionStartedMs = 0;
        _lastPollMs = millis() - periodMs; // first sample can start immediately
    }

    bool isConversionPending() const { return _conversionPending; }

    bool isReadyToRead(uint32_t now) const {
        return enabled && _conversionPending && ((uint32_t)(now - _conversionStartedMs) >= _conversionWaitMs);
    }

    bool isDueToStart(uint32_t now) const {
        const uint32_t retryMs = present ? periodMs : 1000UL;
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
            _faultSeen = true;
            _recoveryHoldUntilMs = 0;
            return true;
        }

        float t = _dt.getTempCByIndex(0);
        if (t == DEVICE_DISCONNECTED_C) {
            present = false;
            error = true;
            value = NAN;
            hwLimited = false;
            diagCode = SENSOR_DIAG_NONE;
            _faultSeen = true;
            _recoveryHoldUntilMs = 0;
            return true;
        }

        present = true;
        value = t;
        lastValidMs = now;
        hwLimited = false;
        error = false;
        diagCode = SENSOR_DIAG_NONE;
        _faultSeen = false;
        _recoveryHoldUntilMs = 0;
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
            error = true;
            value = NAN;
            hwLimited = false;
            diagCode = SENSOR_DIAG_NONE;
        }
        return ok;
    }

    OneWire           _ow;
    DallasTemperature _dt;
    bool              _faultSeen = false;
    uint32_t          _recoveryHoldUntilMs = 0;
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
        present = true;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;

        if (!isnan(_t1->value) && !isnan(_t2->value) && !_t1->error && !_t2->error && _t1->present && _t2->present) {
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
    PressureSensor() : SensorBase("P") {
        periodMs = DEF_P_PERIOD_MS;
    }

    void begin() override {
        Wire.begin(PIN_BMP_SDA, PIN_BMP_SCL);
        present = _bmp.begin(BMP085_ULTRALOWPOWER);
        error   = !present;
        lastValidMs = 0;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

    void poll() override {
        _lastPollMs = millis();

        if (!present) present = _bmp.begin(BMP085_ULTRALOWPOWER);
        if (!present) {
            error = true;
            value = NAN;
            return;
        }

        value = _bmp.readPressure() / 100.0f;
        lastValidMs = _lastPollMs;
        error = false;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

private:
    Adafruit_BMP085 _bmp;
};

class DigitalSensor : public SensorBase {
public:
    DigitalSensor(const String& n, uint8_t pin)
        : SensorBase(n), _pin(pin)
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
        value   = (digitalRead(_pin) == HIGH) ? 1.0f : 0.0f;
        lastValidMs = _lastPollMs;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

    void poll() override {
        _lastPollMs = millis();
        present = true;
        error = false;
        value = (digitalRead(_pin) == HIGH) ? 1.0f : 0.0f;
        lastValidMs = _lastPollMs;
        hwLimited = false;
        diagCode = SENSOR_DIAG_NONE;
    }

    bool isCircuitClosed() const { return value > 0.5f; }
    bool isCircuitOpen() const { return !isCircuitClosed(); }
    bool isActive() const { return isCircuitClosed(); }

private:
    uint8_t _pin;
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
