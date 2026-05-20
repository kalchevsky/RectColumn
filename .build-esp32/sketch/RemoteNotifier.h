#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\RemoteNotifier.h"
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <new>
#include <math.h>
#include <string.h>

#include "WiFiMgr.h"
#include "EventLog.h"
#include "SensorManager.h"
#include "OutputManager.h"
#include "Storage.h"

#ifndef APP_CPU_NUM
#define APP_CPU_NUM 1
#endif

class RemoteNotifier {
public:
    void begin(WiFiMgr& wifi, EventLog& log, SensorManager& sm, OutputManager& om, Storage& stor) {
        _wifi = &wifi;
        _log  = &log;
        _sm   = &sm;
        _om   = &om;
        _stor = &stor;

        loadConfig();
        if (!_queue) {
            _queue = xQueueCreate(QUEUE_SIZE, sizeof(QueueItem));
        }
        if (_queue && !_worker) {
            const BaseType_t ok = xTaskCreatePinnedToCore(
                _workerTask,
                "notify-worker",
                16384,
                this,
                tskIDLE_PRIORITY + 1,
                &_worker,
                APP_CPU_NUM
            );
            if (ok != pdPASS) {
                _worker = nullptr;
                _queueSendFailure("notify worker create failed");
            }
        }
        _snapshotCurrentAlarms();
    }

    void loadConfig() {
        if (!_stor) return;
        _stor->loadNotifyConfig(_enabled, _publishUrl, _accessToken);
        _publishUrl.trim();
        _accessToken.trim();
        // === PATCH NTFY BEGIN ===
        _setRedirectBlocked(false);
        // === PATCH NTFY END ===
        _refreshConfigSnapshot();
    }

    void setConfig(bool enabled, const String& publishUrl, const String& token) {
        // === PATCH NTFY BEGIN ===
        portENTER_CRITICAL(&_cfgMux);
        _enabled = enabled;
        _publishUrl = publishUrl;
        _accessToken = token;
        _redirectBlocked = false;
        _publishUrl.trim();
        _accessToken.trim();
        portEXIT_CRITICAL(&_cfgMux);
        // === PATCH NTFY END ===
        _refreshConfigSnapshot();
        if (_stor) _stor->saveNotifyConfig(_enabled, _publishUrl, _accessToken);
        // Re-baseline current alarms so already active/acknowledged alarms do
        // not get replayed as "new" just because notification settings changed.
        _snapshotCurrentAlarms();
    }

    bool enabled() const {
        bool enabledSnap = false;
        _copyConfigSnapshot(&enabledSnap, nullptr, 0, nullptr, 0);
        return enabledSnap;
    }
    String publishUrl() const {
        char url[sizeof(_publishUrlSnap)] = {};
        _copyConfigSnapshot(nullptr, url, sizeof(url), nullptr, 0);
        return String(url);
    }
    bool hasToken() const {
        char token[sizeof(_accessTokenSnap)] = {};
        _copyConfigSnapshot(nullptr, nullptr, 0, token, sizeof(token));
        return token[0] != '\0';
    }
    String accessToken() const {
        char token[sizeof(_accessTokenSnap)] = {};
        _copyConfigSnapshot(nullptr, nullptr, 0, token, sizeof(token));
        return String(token);
    }
    uint32_t droppedCount() const { return _droppedCount; }
    size_t queueDepth() const { return _queue ? (size_t)uxQueueMessagesWaiting(_queue) : 0; }
    size_t queueCapacity() const { return QUEUE_SIZE; }
    bool workerReady() const { return _queue != nullptr && _worker != nullptr; }

    bool validatePublishUrl(const String& publishUrl, String& errOut) const {
        return _validatePublishUrl(publishUrl, errOut);
    }

    bool sendTest(String& errOut) {
        char url[sizeof(_publishUrlSnap)] = {};
        char token[sizeof(_accessTokenSnap)] = {};
        _copyConfigSnapshot(nullptr, url, sizeof(url), token, sizeof(token));
        return _sendNtfyWithTarget(String(url), String(token),
                                   "Система управления", "Тестовое уведомление",
                                   "3", "test_tube", errOut);
    }

    bool sendTestTo(const String& publishUrl, const String& token, String& errOut) {
        return _sendNtfyWithTarget(publishUrl, token, "Система управления", "Тестовое уведомление", "3", "test_tube", errOut);
    }

    void loop() {
        if (!_wifi || !_sm || !_om) return;
        _flushQueuedFailure();

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
    portMUX_TYPE _failMux = portMUX_INITIALIZER_UNLOCKED;
    bool _queuedFailPending = false;
    char _queuedFailText[192] = {};
    mutable portMUX_TYPE _cfgMux = portMUX_INITIALIZER_UNLOCKED;
    bool _enabledSnap = false;
    char _publishUrlSnap[160] = {};
    char _accessTokenSnap[96] = {};
    // === PATCH NTFY BEGIN ===
    bool _redirectBlocked = false;
    // === PATCH NTFY END ===
    QueueHandle_t _queue = nullptr;
    TaskHandle_t _worker = nullptr;
    volatile uint32_t _droppedCount = 0;

    static constexpr size_t QUEUE_SIZE = 8;

    struct QueueItem {
        char title[96];
        char body[320];
        char priority[8];
        char tags[48];
    };

    void _snapshotCurrentAlarms() {
        for (int si = 0; si < SEN_COUNT; si++) {
            _lastAlarmMask[si] = _audibleAlarmMask(si);
        }
    }

    uint8_t _audibleAlarmMask(int si) const {
        if (!_sm || !_om) return 0;
        bool enabledSnap = false;
        char url[sizeof(_publishUrlSnap)] = {};
        _copyConfigSnapshot(&enabledSnap, url, sizeof(url), nullptr, 0);
        if (!enabledSnap || url[0] == '\0') return 0;
        if (_om->soundMuted || !_om->ch5Enabled) return 0;

        SensorBase* s = _sm->s[si];
        if (!s || !s->enabled) return 0;
        // Notifications follow the same acknowledgement semantics as sound:
        // only unacknowledged active alarms should be considered new.
        return _om->unackedAlarmMaskFor(*_sm, (uint8_t)si);
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
                return String("Давление ") + _alarmToken(bitIdx) + " (" + _formatRuNumber(thr, 1) + " гПа)";
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
        char url[sizeof(_publishUrlSnap)] = {};
        char token[sizeof(_accessTokenSnap)] = {};
        _copyConfigSnapshot(nullptr, url, sizeof(url), token, sizeof(token));
        return _sendNtfyWithTarget(String(url), String(token), title, body, priority, tags, errOut);
    }

    bool _scheduleNotify(const String& title,
                         const String& body,
                         const char* priority,
                         const char* tags,
                         String& errOut)
    {
        if (!_queue || !_worker) {
            errOut = "notify worker is not ready";
            return false;
        }

        bool enabledSnap = false;
        char url[sizeof(_publishUrlSnap)] = {};
        _copyConfigSnapshot(&enabledSnap, url, sizeof(url), nullptr, 0);
        if (!enabledSnap) {
            errOut = "notify is disabled";
            return false;
        }
        if (!_validatePublishUrlSnapshot(url, errOut)) {
            return false;
        }
        // === PATCH NTFY BEGIN ===
        if (_isRedirectBlocked()) {
            errOut = "server returned redirect (HTTPS required?); use a publish URL that does not redirect";
            return false;
        }
        // === PATCH NTFY END ===
        if (!_wifi || !_wifi->staConnected) {
            errOut = "STA is not connected";
            return false;
        }

        QueueItem item = {};
        strlcpy(item.title, title.c_str(), sizeof(item.title));
        strlcpy(item.body, body.c_str(), sizeof(item.body));
        strlcpy(item.priority, priority ? priority : "3", sizeof(item.priority));
        strlcpy(item.tags, tags ? tags : "information_source", sizeof(item.tags));

#if STABILITY_BOOT_DIAG
        Serial.printf("[NTFY] schedule bodyLen=%u urlLen=%u\n",
                      (unsigned)strlen(item.body),
                      (unsigned)strlen(url));
#endif

        if (xQueueSend(_queue, &item, 0) != pdTRUE) {
            _droppedCount++;
            errOut = "notification queue full";
            _queueSendFailure("Notification queue full");
            return false;
        }

        return true;
    }

    static void _workerTask(void* p) {
        RemoteNotifier* self = static_cast<RemoteNotifier*>(p);
        if (!self) {
            vTaskDelete(nullptr);
            return;
        }

        QueueItem item = {};
        for (;;) {
            if (xQueueReceive(self->_queue, &item, portMAX_DELAY) != pdTRUE) {
                continue;
            }

            bool enabledSnap = false;
            char url[sizeof(self->_publishUrlSnap)] = {};
            char token[sizeof(self->_accessTokenSnap)] = {};
            self->_copyConfigSnapshot(&enabledSnap, url, sizeof(url), token, sizeof(token));
            if (!enabledSnap || url[0] == '\0') continue;

            String err;
#if STABILITY_BOOT_DIAG
            Serial.printf("[NTFY] worker bodyLen=%u queueDepth=%u\n",
                          (unsigned)strlen(item.body),
                          (unsigned)(self->_queue ? uxQueueMessagesWaiting(self->_queue) : 0));
#endif
            if (!self->_sendNtfyWithTarget(String(url),
                                           String(token),
                                           String(item.title),
                                           String(item.body),
                                           item.priority,
                                           item.tags,
                                           err)) {
                // === PATCH NTFY BEGIN ===
                if (err.startsWith("server returned redirect")) {
                    self->_setRedirectBlocked(true);
                    if (self->_queue) xQueueReset(self->_queue);
                }
                // === PATCH NTFY END ===
                self->_queueSendFailure(err.c_str());
            }
        }
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
        // === PATCH NTFY BEGIN ===
        http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
        http.useHTTP10(true);
        // === PATCH NTFY END ===

        http.setConnectTimeout(4000);
        http.setTimeout(5000);
        http.addHeader("Content-Type", "text/plain; charset=utf-8");
        // === PATCH NTFY BEGIN ===
        http.addHeader("Connection", "close");
        // === PATCH NTFY END ===
        http.addHeader("Title", title);
        http.addHeader("Priority", priority ? priority : "3");
        http.addHeader("Tags", tags ? tags : "information_source");
        if (token.length() > 0) {
            String authHeader;
            authHeader.reserve(token.length() + 7);
            authHeader = "Bearer ";
            authHeader += token;
            http.addHeader("Authorization", authHeader);
        }

        code = http.POST((uint8_t*)body.c_str(), body.length());
        // === PATCH NTFY BEGIN ===
        if (code == 301 || code == 302 || code == 307 || code == 308) {
            http.end();
            errOut = "server returned redirect (HTTPS required?); use a publish URL that does not redirect";
            return false;
        }
        // === PATCH NTFY END ===
        if (code >= 200 && code < 300) {
#if STABILITY_BOOT_DIAG
            Serial.printf("[NTFY] sent ok code=%d\n", code);
#endif
            http.end();
            return true;
        }

        resp = http.getString();
        http.end();

        errOut = String("HTTP ") + code;
        if (resp.length()) {
            errOut += ": ";
            errOut += resp;
        }
#if STABILITY_BOOT_DIAG
        Serial.printf("[NTFY] send failed code=%d errLen=%u\n", code, (unsigned)errOut.length());
#endif
        return false;
    }

    static bool _validatePublishUrl(const String& publishUrl, String& errOut) {
        errOut = "";
        String url = publishUrl;
        url.trim();
        return _validatePublishUrlSnapshot(url.c_str(), errOut);
    }

    static bool _validatePublishUrlSnapshot(const char* publishUrl, String& errOut) {
        errOut = "";
        if (!publishUrl || !publishUrl[0]) {
            errOut = "notify url is empty";
            return false;
        }
        if (strncmp(publishUrl, "http://", 7) != 0) {
            errOut = "Publish URL must start with http:// (HTTPS is not supported here)";
            return false;
        }
        return true;
    }

    // === PATCH NTFY BEGIN ===
    void _setRedirectBlocked(bool blocked) {
        portENTER_CRITICAL(&_cfgMux);
        _redirectBlocked = blocked;
        portEXIT_CRITICAL(&_cfgMux);
    }

    bool _isRedirectBlocked() const {
        portENTER_CRITICAL(&_cfgMux);
        const bool blocked = _redirectBlocked;
        portEXIT_CRITICAL(&_cfgMux);
        return blocked;
    }
    // === PATCH NTFY END ===

    void _refreshConfigSnapshot() {
        portENTER_CRITICAL(&_cfgMux);
        _enabledSnap = _enabled;
        strlcpy(_publishUrlSnap, _publishUrl.c_str(), sizeof(_publishUrlSnap));
        strlcpy(_accessTokenSnap, _accessToken.c_str(), sizeof(_accessTokenSnap));
        portEXIT_CRITICAL(&_cfgMux);
    }

    void _copyConfigSnapshot(bool* enabledOut,
                             char* urlOut,
                             size_t urlLen,
                             char* tokenOut,
                             size_t tokenLen) const
    {
        portENTER_CRITICAL(&_cfgMux);
        if (enabledOut) *enabledOut = _enabledSnap;
        if (urlOut && urlLen > 0) strlcpy(urlOut, _publishUrlSnap, urlLen);
        if (tokenOut && tokenLen > 0) strlcpy(tokenOut, _accessTokenSnap, tokenLen);
        portEXIT_CRITICAL(&_cfgMux);
    }

    void _queueSendFailure(const char* err) {
        portENTER_CRITICAL(&_failMux);
        _queuedFailPending = true;
        if (!err || !err[0]) err = "unknown";
        strncpy(_queuedFailText, err, sizeof(_queuedFailText) - 1);
        _queuedFailText[sizeof(_queuedFailText) - 1] = '\0';
        portEXIT_CRITICAL(&_failMux);
#if STABILITY_BOOT_DIAG
        Serial.printf("[NTFY] queued failure: %s\n", _queuedFailText);
#endif
    }

    void _flushQueuedFailure() {
        char errBuf[sizeof(_queuedFailText)] = {};
        bool hasQueuedFailure = false;

        portENTER_CRITICAL(&_failMux);
        if (_queuedFailPending) {
            strncpy(errBuf, _queuedFailText, sizeof(errBuf) - 1);
            errBuf[sizeof(errBuf) - 1] = '\0';
            _queuedFailPending = false;
            _queuedFailText[0] = '\0';
            hasQueuedFailure = true;
        }
        portEXIT_CRITICAL(&_failMux);

        if (!hasQueuedFailure) return;
        _logSendFailure(String(errBuf));
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
