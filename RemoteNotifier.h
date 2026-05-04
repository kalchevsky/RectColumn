#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <new>
#include <math.h>

#include "WiFiMgr.h"
#include "EventLog.h"
#include "SensorManager.h"
#include "OutputManager.h"
#include "Storage.h"

class RemoteNotifier {
public:
    void begin(WiFiMgr& wifi, EventLog& log, SensorManager& sm, OutputManager& om, Storage& stor) {
        _wifi = &wifi;
        _log  = &log;
        _sm   = &sm;
        _om   = &om;
        _stor = &stor;

        loadConfig();
        _snapshotCurrentAlarms();
    }

    void loadConfig() {
        if (!_stor) return;
        _stor->loadNotifyConfig(_enabled, _publishUrl, _accessToken);
        _publishUrl.trim();
        _accessToken.trim();
    }

    void setConfig(bool enabled, const String& publishUrl, const String& token) {
        _enabled = enabled;
        _publishUrl = publishUrl;
        _accessToken = token;
        _publishUrl.trim();
        _accessToken.trim();
        if (_stor) _stor->saveNotifyConfig(_enabled, _publishUrl, _accessToken);
    }

    bool enabled() const { return _enabled; }
    String publishUrl() const { return _publishUrl; }
    bool hasToken() const { return _accessToken.length() > 0; }
    String accessToken() const { return _accessToken; }

    bool validatePublishUrl(const String& publishUrl, String& errOut) const {
        return _validatePublishUrl(publishUrl, errOut);
    }

    bool sendTest(String& errOut) {
        return _sendNtfy("Система управления", "Тестовое уведомление", "3", "test_tube", errOut);
    }

    bool sendTestTo(const String& publishUrl, const String& token, String& errOut) {
        return _sendNtfyWithTarget(publishUrl, token, "Система управления", "Тестовое уведомление", "3", "test_tube", errOut);
    }

    void loop() {
        if (!_wifi || !_sm || !_om) return;

        for (int si = 0; si < SEN_COUNT; si++) {
            const uint8_t curMask = _audibleAlarmMask(si);
            const uint8_t newBits = (uint8_t)(curMask & (uint8_t)(~_lastAlarmMask[si]));
            if (newBits != 0) {
                String err;
                const uint8_t alarmBit = _selectPrimaryAlarmBit(newBits);
                const String message = _alarmText(si, alarmBit);
                if (!_scheduleNotify("Система управления", message, "4", "warning,rotating_light", err)) {
                    _logSendFailure(err);
                }
            }
            _lastAlarmMask[si] = curMask;
        }
    }

private:
    WiFiMgr*       _wifi = nullptr;
    EventLog*      _log  = nullptr;
    SensorManager* _sm   = nullptr;
    OutputManager* _om   = nullptr;
    Storage*       _stor = nullptr;

    bool    _enabled = false;
    String  _publishUrl;
    String  _accessToken;
    uint8_t _lastAlarmMask[SEN_COUNT] = {};
    uint32_t _lastFailLogMs = 0;

    struct NotifyTaskPayload {
        RemoteNotifier* self = nullptr;
        String publishUrl;
        String token;
        String title;
        String body;
        String priority;
        String tags;
    };

    void _snapshotCurrentAlarms() {
        for (int si = 0; si < SEN_COUNT; si++) {
            _lastAlarmMask[si] = _audibleAlarmMask(si);
        }
    }

    uint8_t _audibleAlarmMask(int si) const {
        if (!_sm || !_om) return 0;
        if (!_enabled || _publishUrl.length() == 0) return 0;
        if (_om->soundMuted || !_om->ch5Enabled) return 0;

        SensorBase* s = _sm->s[si];
        if (!s || !s->enabled) return 0;
        return s->alarmMask();
    }

    static uint8_t _selectPrimaryAlarmBit(uint8_t bits) {
        if (bits & (1u << 3)) return 3; // max2
        if (bits & (1u << 2)) return 2; // max1
        if (bits & (1u << 1)) return 1; // min2
        return 0;                       // min1 / toggle alarm
    }

    static String _formatRuNumber(float value, uint8_t decimals = 1) {
        if (!isfinite(value)) return String("—");
        char buf[32];
        dtostrf(value, 0, decimals, buf);
        String out(buf);
        out.trim();
        while (out.endsWith("0")) out.remove(out.length() - 1);
        if (out.endsWith(".")) out.remove(out.length() - 1);
        out.replace('.', ',');
        return out;
    }

    static String _formatPercentFromRaw(float raw) {
        if (!isfinite(raw)) return String("—");
        float pct = raw * 100.0f / 4095.0f;
        return _formatRuNumber(pct, 1);
    }

    static String _formatPressureMmHg(float hpa) {
        if (!isfinite(hpa)) return String("—");
        float mmhg = hpa * 0.75006156f;
        return _formatRuNumber(mmhg, 1);
    }

    static String _alarmToken(uint8_t bitIdx) {
        switch (bitIdx) {
            case 0: return "min1";
            case 1: return "min2";
            case 2: return "max1";
            case 3: return "max2";
            default: return String("level") + String(bitIdx + 1);
        }
    }

    String _alarmText(int si, uint8_t bitIdx) const {
        SensorBase* s = (_sm && si >= 0 && si < SEN_COUNT) ? _sm->s[si] : nullptr;
        if (s && (!s->present || s->error)) {
            const char* name = SensorManager::sensorName(si);
            if (!s->present) return String("Нет датчика ") + (name ? name : "—");
            return String("Ошибка датчика ") + (name ? name : "—");
        }
        switch (si) {
            case SEN_C: {
                const float thr = (s && bitIdx < N_ALARMS) ? s->alarm[bitIdx].threshold : NAN;
                return String("Ток нагрузки min (") + _formatPercentFromRaw(thr) + "%)";
            }
            case SEN_F:
                return "Проток min";
            case SEN_L:
                return "Уровень max";
            case SEN_P: {
                const float thr = (s && bitIdx < N_ALARMS) ? s->alarm[bitIdx].threshold : NAN;
                return String("Давление ") + _alarmToken(bitIdx) + " (" + _formatPressureMmHg(thr) + " мм. рт. ст.)";
            }
            case SEN_T1:
            case SEN_T2:
            case SEN_T3:
            case SEN_DT: {
                const char* name = SensorManager::sensorName(si);
                const float thr = (s && bitIdx < N_ALARMS) ? s->alarm[bitIdx].threshold : NAN;
                return String(name) + " " + _alarmToken(bitIdx) + " (" + _formatRuNumber(thr, 1) + "°С)";
            }
            case SEN_V: {
                const float thr = (s && bitIdx < N_ALARMS) ? s->alarm[bitIdx].threshold : NAN;
                return String("Напряжение нагрузки ") + _alarmToken(bitIdx) + " (" + _formatRuNumber(thr, 1) + ")";
            }
            default: {
                const char* name = SensorManager::sensorName(si);
                const float thr = (s && bitIdx < N_ALARMS) ? s->alarm[bitIdx].threshold : NAN;
                return String(name ? name : "Датчик") + " " + _alarmToken(bitIdx) + " (" + _formatRuNumber(thr, 1) + ")";
            }
        }
    }

    bool _sendNtfy(const String& title,
                   const String& body,
                   const char* priority,
                   const char* tags,
                   String& errOut)
    {
        return _sendNtfyWithTarget(_publishUrl, _accessToken, title, body, priority, tags, errOut);
    }

    bool _scheduleNotify(const String& title,
                         const String& body,
                         const char* priority,
                         const char* tags,
                         String& errOut)
    {
        if (!_validatePublishUrl(_publishUrl, errOut)) {
            return false;
        }
        if (!_wifi || !_wifi->staConnected) {
            errOut = "STA is not connected";
            return false;
        }

        NotifyTaskPayload* payload = new (std::nothrow) NotifyTaskPayload();
        if (!payload) {
            errOut = "notify task alloc failed";
            return false;
        }

        payload->self = this;
        payload->publishUrl = _publishUrl;
        payload->token = _accessToken;
        payload->title = title;
        payload->body = body;
        payload->priority = priority ? priority : "3";
        payload->tags = tags ? tags : "information_source";

        const BaseType_t ok = xTaskCreate(
            _notifyTask,
            "notify",
            4096,
            payload,
            1,
            nullptr
        );
        if (ok != pdPASS) {
            delete payload;
            errOut = "notify task create failed";
            return false;
        }

        return true;
    }

    static void _notifyTask(void* p) {
        NotifyTaskPayload* payload = static_cast<NotifyTaskPayload*>(p);
        if (!payload) {
            vTaskDelete(nullptr);
            return;
        }

        RemoteNotifier* self = payload->self;
        if (self) {
            String err;
            if (!self->_sendNtfyWithTarget(payload->publishUrl,
                                           payload->token,
                                           payload->title,
                                           payload->body,
                                           payload->priority.c_str(),
                                           payload->tags.c_str(),
                                           err)) {
                self->_logSendFailure(err);
            }
        }

        delete payload;
        vTaskDelete(nullptr);
    }

    bool _sendNtfyWithTarget(const String& publishUrl,
                             const String& token,
                             const String& title,
                             const String& body,
                             const char* priority,
                             const char* tags,
                             String& errOut)
    {
        if (!_validatePublishUrl(publishUrl, errOut)) {
            return false;
        }
        if (!_wifi || !_wifi->staConnected) {
            errOut = "STA is not connected";
            return false;
        }
        HTTPClient http;
        WiFiClient client;
        int code = -1;
        String resp;

        if (!http.begin(client, publishUrl)) {
            errOut = "http begin failed";
            return false;
        }

        http.setConnectTimeout(4000);
        http.setTimeout(5000);
        http.addHeader("Content-Type", "text/plain; charset=utf-8");
        http.addHeader("Title", title);
        http.addHeader("Priority", priority ? priority : "3");
        http.addHeader("Tags", tags ? tags : "information_source");
        if (token.length() > 0) {
            http.addHeader("Authorization", String("Bearer ") + token);
        }

        code = http.POST((uint8_t*)body.c_str(), body.length());
        resp = http.getString();
        http.end();

        if (code >= 200 && code < 300) return true;

        errOut = String("HTTP ") + code;
        if (resp.length()) {
            errOut += ": ";
            errOut += resp;
        }
        return false;
    }

    static bool _validatePublishUrl(const String& publishUrl, String& errOut) {
        errOut = "";
        String url = publishUrl;
        url.trim();
        if (url.length() == 0) {
            errOut = "notify url is empty";
            return false;
        }
        if (!url.startsWith("http://")) {
            errOut = "Publish URL must start with http:// (HTTPS is not supported here)";
            return false;
        }
        return true;
    }

    void _logSendFailure(const String& err) {
        if (!_log) return;
        const uint32_t now = millis();
        if (now - _lastFailLogMs < 10000UL) return;
        _lastFailLogMs = now;
        _log->add(String("Notify failed: ") + err,
                  _sm ? _sm->getT1() : NAN,
                  _sm ? _sm->getT2() : NAN,
                  _sm ? _sm->getT3() : NAN,
                  _sm ? _sm->getDT() : NAN);
    }
};
