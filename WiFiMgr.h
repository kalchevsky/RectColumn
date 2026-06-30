// ================================================================
// WiFiMgr оптимизмрован по v006
// ================================================================
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#if __has_include(<esp_idf_version.h>)
#include <esp_idf_version.h>
#endif

#if defined(ESP_IDF_VERSION_MAJOR) && (ESP_IDF_VERSION_MAJOR >= 4)
#define RC_HAS_SOFTAP_CLIENT_RSSI 1
#else
#define RC_HAS_SOFTAP_CLIENT_RSSI 0
#endif

#include "Storage.h"
#include "EventLog.h"
#include "config.h"

class WiFiMgr {
public:
    String apSSID = AP_SSID_DEF;
    String apPass = "";
    String staSSID = "";
    String staPass = "";
    bool   staConnected = false;

    enum class StaTry : uint8_t {
        Idle,
        InProgress,
        Success,
        Failed
    };

    void begin(Storage& stor, EventLog& log) {
        _log = &log;
        _stor = &stor;

        stor.loadWifiSTA(staSSID, staPass);
        _apOnly = stor.loadWifiApOnly();
        if (staSSID.length() == 0) _apOnly = true;   // нет STA-сети => только AP
        // ─── Локальный WiFi для отладки  ─────────────────────────────────
        // if (staSSID.length() == 0) {
        //     staSSID = STA_SSID_DEF;
        //     staPass = STA_PASS_DEF;
        // }
        apPass = stor.loadAPPassword();
        if (apPass.length() > 0 && (apPass.length() < 8 || apPass.length() > 63)) {
            apPass = "";
            stor.saveAPPassword("");
            log.add("WiFi: invalid AP password in storage, AP reverted to open");
        }

        WiFi.persistent(false);
        WiFi.setSleep(false);         // AP-режим стабильнее без modem sleep
        WiFi.setHostname(DEVICE_NAME);
        _applyConfiguredMode(!_apOnly);

        if (_apRunning) {
            log.add(String("WiFi: AP started - ") + apSSID + " / " + (apPass.length() ? "protected" : "open")
                    + " IP=" + WiFi.softAPIP().toString());
        } else {
            log.add("WiFi: AP start FAILED");
        }
    }

    void loop() {
        if (_modeApplyPending) {
            _modeApplyPending = false;
            _applyConfiguredMode(!_apOnly);
        }

        _dns.processNextRequest();

        if (_apRunning && (millis() - _lastApSignalPollMs >= 1500UL)) {
            _lastApSignalPollMs = millis();
            _refreshApClientSignal();
        }

        const wl_status_t curEnum = WiFi.status();
        const int cur = (int)curEnum;
        const bool isConn = (curEnum == WL_CONNECTED);

        if (cur != _lastStatus) {
            _lastStatus = cur;
            _lastStatusText = statusText(cur);
        }

        if (isConn != staConnected) {
            staConnected = isConn;
            if (_log) {
                if (isConn) {
                    _log->add("WiFi: STA connected - " + staSSID + " IP=" + WiFi.localIP().toString());
                } else {
                    _log->add("WiFi: STA disconnected");
                }
            }
            _lastReconnectMs = millis();
        }

        if (isConn) {
            _ensureMDNSStarted();
        }

        _advanceStaTry(curEnum);

        if (_staTry != StaTry::InProgress &&
            !_apOnly &&
            !isConn && staSSID.length() > 0 &&
            !_reconnectPaused() &&
            (millis() - _lastReconnectMs >= WIFI_RECONNECT_COOLDOWN_MS)) {
            _lastReconnectMs = millis();
            _connectSTA();
        }
    }

    void fillScan(JsonArray arr) {
        const bool hadReconnect = WiFi.getAutoReconnect();
        const bool reconnectingToStoredAp = !_apOnly && (staSSID.length() > 0) && !staConnected;

        _pauseStaReconnect(WIFI_SCAN_RECONNECT_PAUSE_MS);
        WiFi.setAutoReconnect(false);
        if (reconnectingToStoredAp) {
            WiFi.disconnect(false, false);
            delay(150);
        }

        WiFi.scanDelete();
        delay(50);

        int n = WiFi.scanNetworks(false, true);
        if (n == WIFI_SCAN_FAILED && reconnectingToStoredAp) {
            delay(250);
            WiFi.scanDelete();
            n = WiFi.scanNetworks(false, true);
        }

        _scanEverRun = true;
        _lastScanStatus = n;
        _lastScanCount = 0;
        for (int i = 0; i < n; i++) {
            const String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;

            JsonObject it = arr.createNestedObject();
            it["ssid"]    = ssid;
            it["rssi"]    = WiFi.RSSI(i);
            it["enc"]     = (int)WiFi.encryptionType(i);
            it["channel"] = WiFi.channel(i);
            _lastScanCount++;
        }
        WiFi.scanDelete();
        WiFi.setAutoReconnect(hadReconnect);
    }

    bool beginConnectSTA(const String& ssid, const String& pass, Storage& stor) {
        if (ssid.length() == 0) return false;
        if (_staTry == StaTry::InProgress) return false;

        resetStaTryState();
        _stor = &stor;
        _staTrySsid = ssid;
        _staTryPass = pass;
        _staTryPrevSsid = staSSID;
        _staTryPrevPass = staPass;
        _staTryPrevApOnly = _apOnly;
        _staTryPrevModeApplyPend = _modeApplyPending;
        _clearStaTryResult();

        if (_apOnly || _modeApplyPending) {
            _apOnly = false;
            _modeApplyPending = false;
            _applyConfiguredMode(false);
        }

        _connectSTA(ssid, pass);
        _lastReconnectMs = millis();
        _staTryStartMs = millis();
        _staTry = StaTry::InProgress;
        return true;
    }

    void setAPPassword(const String& pass, Storage& stor) {
        apPass = pass;
        stor.saveAPPassword(pass);
        _startAP();
        if (_apFallbackToOpen) {
            stor.saveAPPassword("");
        }
    }

    bool apOnly() const { return _apOnly; }
    const char* wifiMode() const { return _apOnly ? "ap_only" : "sta_ap"; }

    void setApOnly(bool enabled) {
        if (_stor) {
            _stor->saveWifiApOnly(enabled);
        }
        if (_apOnly == enabled && !_modeApplyPending) return;
        _apOnly = enabled;
        _modeApplyPending = true;
    }

    // Алиас для совместимости с другими ветками
    void updateAPPassword(const String& pass, Storage& stor) {
        setAPPassword(pass, stor);
    }

    String apIP() const { return WiFi.softAPIP().toString(); }
    String staIP() const { return staConnected ? WiFi.localIP().toString() : String(""); }
    int    staRssi() const { return staConnected ? WiFi.RSSI() : -127; }
    int    apRssi() const { return (_apClientCount > 0) ? _apClientRssi : -127; }
    int    rssi() const { return staConnected ? staRssi() : apRssi(); }
    int    apClientCount() const { return _apClientCount; }
    int    lastScanStatus() const { return _lastScanStatus; }
    int    lastScanCount() const { return _lastScanCount; }
    bool   staTryInProgress() const { return _staTry == StaTry::InProgress; }
    bool   staTryFinished() const { return _staTry == StaTry::Success || _staTry == StaTry::Failed; }
    StaTry staTryState() const { return _staTry; }
    const char* staTryStateText() const {
        switch (_staTry) {
            case StaTry::Idle:       return "idle";
            case StaTry::InProgress: return "in_progress";
            case StaTry::Success:    return "success";
            case StaTry::Failed:     return "failed";
        }
        return "idle";
    }
    bool   staTryResultOk() const { return _staTryResultOk; }
    const String& staTryResultSsid() const { return _staTryResultSsid; }
    const String& staTryAttemptedSsid() const { return _staTrySsid; }
    const String& staTryResultIp() const { return _staTryResultIp; }
    const String& staTryResultStatusText() const { return _staTryResultStatusText; }
    int    staTryResultStatus() const { return _staTryResultStatus; }
    bool   staTryTimedOut() const { return _staTryTimedOut; }
    bool   staTrySaved() const { return _staTrySaved; }
    void   resetStaTryState() {
        if (_staTry == StaTry::InProgress) return;
        _staTry = StaTry::Idle;
        _staTrySsid = "";
        _staTryPass = "";
        _staTryPrevSsid = "";
        _staTryPrevPass = "";
        _staTryPrevApOnly = false;
        _staTryPrevModeApplyPend = false;
        _staTryStartMs = 0;
        _clearStaTryResult();
    }
    uint32_t reconnectPauseRemainingMs() const {
        return _reconnectPaused() ? (uint32_t)(_reconnectPausedUntilMs - millis()) : 0UL;
    }

    bool staConfigured() const { return staSSID.length() > 0; }
    bool apProtected() const { return apPass.length() > 0; }
    bool apRunning() const { return _apRunning; }
    bool apFallbackToOpen() const { return _apFallbackToOpen; }
    String apStatusText() const { return _apStatusText; }
    String staStatusText() const { return _lastStatusText; }
    String lastScanStatusText() const {
        if (!_scanEverRun) return "idle";
        return scanStatusText(_lastScanStatus);
    }
    int lastStatus() const { return _lastStatus; }

    static String statusText(int status) {
        switch (status) {
            case WL_NO_SHIELD:       return "WL_NO_SHIELD";
            case WL_IDLE_STATUS:     return "WL_IDLE_STATUS";
            case WL_NO_SSID_AVAIL:   return "WL_NO_SSID_AVAIL";
            case WL_SCAN_COMPLETED:  return "WL_SCAN_COMPLETED";
            case WL_CONNECTED:       return "WL_CONNECTED";
            case WL_CONNECT_FAILED:  return "WL_CONNECT_FAILED";
            case WL_CONNECTION_LOST: return "WL_CONNECTION_LOST";
            case WL_DISCONNECTED:    return "WL_DISCONNECTED";
            default:                 return String("WL_") + status;
        }
    }

    static String statusText(wl_status_t s) {
        return statusText((int)s);
    }

    static String scanStatusText(int status) {
        if (status == WIFI_SCAN_RUNNING) return "WIFI_SCAN_RUNNING";
        if (status == WIFI_SCAN_FAILED)  return "WIFI_SCAN_FAILED";
        if (status >= 0) return String("OK_") + status;
        return String("SCAN_") + status;
    }

private:
    DNSServer _dns;
    EventLog* _log = nullptr;
    Storage*  _stor = nullptr;
    uint32_t  _lastReconnectMs = 0;
    uint32_t  _reconnectPausedUntilMs = 0;
    int       _lastStatus = WL_IDLE_STATUS;
    String    _lastStatusText = "idle";
    bool      _mdnsStarted = false;
    bool      _apOnly = false;
    bool      _modeApplyPending = false;
    bool      _apRunning = false;
    bool      _apFallbackToOpen = false;
    String    _apStatusText = "idle";
    uint32_t  _lastApSignalPollMs = 0;
    int       _apClientCount = 0;
    int       _apClientRssi = -127;
    bool      _scanEverRun = false;
    int       _lastScanStatus = 0;
    int       _lastScanCount = 0;
    String    _activeApSsid = "";
    String    _activeApPass = "";
    StaTry    _staTry = StaTry::Idle;
    String    _staTrySsid = "";
    String    _staTryPass = "";
    String    _staTryPrevSsid = "";
    String    _staTryPrevPass = "";
    bool      _staTryPrevApOnly = false;
    bool      _staTryPrevModeApplyPend = false;
    uint32_t  _staTryStartMs = 0;
    bool      _staTryResultOk = false;
    String    _staTryResultSsid = "";
    String    _staTryResultIp = "";
    String    _staTryResultStatusText = "";
    int       _staTryResultStatus = 0;
    bool      _staTryTimedOut = false;
    bool      _staTrySaved = false;

    void _startAP() {
        if (_apRunning &&
            _activeApSsid == apSSID &&
            _activeApPass == apPass) {
            _refreshApClientSignal();
            return;
        }

        _apFallbackToOpen = false;
        _apStatusText = "failed";
        bool ok = false;
        if (apPass.length() > 0) ok = WiFi.softAP(apSSID.c_str(), apPass.c_str());
        else                     ok = WiFi.softAP(apSSID.c_str());

        if (!ok && _apRunning) {
            WiFi.softAPdisconnect(false);
            delay(20);
            if (apPass.length() > 0) ok = WiFi.softAP(apSSID.c_str(), apPass.c_str());
            else                     ok = WiFi.softAP(apSSID.c_str());
        }

        if (!ok && apPass.length() > 0) {
            apPass = "";
            if (_apRunning) {
                WiFi.softAPdisconnect(false);
                delay(20);
            }
            ok = WiFi.softAP(apSSID.c_str());
            if (ok) {
                _apFallbackToOpen = true;
            }
        }

        _apRunning = ok;
        if (_apRunning) {
            _activeApSsid = apSSID;
            _activeApPass = apPass;
            _apStatusText = _apFallbackToOpen
                ? "fallback_open"
                : (apPass.length() > 0 ? "protected" : "open");
        } else {
            _activeApSsid = "";
            _activeApPass = "";
        }

        _refreshApClientSignal();
    }

    void _applyConfiguredMode(bool connectStoredSta) {
        WiFi.setAutoReconnect(!_apOnly);
        if (_apOnly) {
            WiFi.disconnect(false, false);
            delay(20);
        }
        WiFi.mode(_apOnly ? WIFI_AP : WIFI_AP_STA);
        delay(20);

        _startAP();
        if (_apRunning) {
            _dns.start(53, "*", WiFi.softAPIP());
        }
        _refreshApClientSignal();

        if (!_apOnly && connectStoredSta && staSSID.length() > 0) {
            _connectSTA();
        }

        _lastReconnectMs = millis();
        _lastStatus = (int)WiFi.status();
        staConnected = (_lastStatus == WL_CONNECTED);

        if (staConnected) {
            _lastStatusText = statusText(_lastStatus);
            _ensureMDNSStarted();
        } else if (_apOnly) {
            _lastStatusText = "AP-only";
        } else {
            _lastStatusText = staSSID.length() > 0 ? "connecting" : "not configured";
        }
    }

    void _connectSTA() {
        if (staSSID.length() == 0) return;
        _connectSTA(staSSID, staPass);
    }

    void _connectSTA(const String& ssid, const String& pass) {
        WiFi.scanDelete();
        WiFi.disconnect(false, false);
        WiFi.begin(ssid.c_str(), pass.c_str());
    }

    bool _staHasIp() const {
        return WiFi.localIP() != IPAddress((uint32_t)0U);
    }

    void _clearStaTryResult() {
        _staTryResultOk = false;
        _staTryResultSsid = "";
        _staTryResultIp = "";
        _staTryResultStatusText = "";
        _staTryResultStatus = WL_IDLE_STATUS;
        _staTryTimedOut = false;
        _staTrySaved = false;
    }

    void _completeStaTrySuccess() {
        _apOnly = false;
        _modeApplyPending = false;
        if (_stor) {
            _stor->saveWifiApOnly(false);
            _stor->saveWifiSTA(_staTrySsid, _staTryPass);
            _stor->saveWifiWizardDone(true);
            _staTrySaved = true;
        } else {
            _staTrySaved = false;
        }

        staSSID = _staTrySsid;
        staPass = _staTryPass;
        staConnected = true;
        _lastReconnectMs = millis();
        _lastStatus = WL_CONNECTED;
        _lastStatusText = statusText(WL_CONNECTED);
        _ensureMDNSStarted();

        _staTryResultOk = true;
        _staTryResultSsid = _staTrySsid;
        _staTryResultIp = WiFi.localIP().toString();
        _staTryResultStatusText = _lastStatusText;
        _staTryResultStatus = WL_CONNECTED;
        _staTryTimedOut = false;
        _staTry = StaTry::Success;
    }

    void _completeStaTryFailure(wl_status_t curEnum) {
        _staTryResultOk = false;
        _staTryResultStatus = (int)curEnum;
        _staTryTimedOut = true;
        _staTrySaved = false;

        WiFi.disconnect(false, false);
        staConnected = false;

        _apOnly = _staTryPrevApOnly;
        _modeApplyPending = _staTryPrevModeApplyPend;

        if (_staTryPrevApOnly) {
            staSSID = _staTryPrevSsid;
            staPass = _staTryPrevPass;
            _applyConfiguredMode(false);
            _staTryResultSsid = "";
            _staTryResultIp = "";
            _staTryResultStatusText = "Не удалось подключиться к " + _staTrySsid;
        } else if (_staTryPrevSsid.length() > 0) {
            staSSID = _staTryPrevSsid;
            staPass = _staTryPrevPass;
            _connectSTA(_staTryPrevSsid, _staTryPrevPass);
            _lastReconnectMs = millis();
            _staTryResultSsid = "";
            _staTryResultIp = "";
            _staTryResultStatusText = "Не удалось подключиться к " + _staTrySsid + "; переподключаемся к " + _staTryPrevSsid;
        } else {
            staSSID = "";
            staPass = "";
            _lastReconnectMs = millis();
            _staTryResultSsid = "";
            _staTryResultIp = "";
            _staTryResultStatusText = "Не удалось подключиться к " + _staTrySsid;
        }

        _lastStatus = (int)WiFi.status();
        _lastStatusText = _staTryResultStatusText;
        _staTry = StaTry::Failed;
    }

    void _advanceStaTry(wl_status_t curEnum) {
        if (_staTry != StaTry::InProgress) return;

        if (curEnum == WL_CONNECTED && _staHasIp()) {
            _completeStaTrySuccess();
            return;
        }

        if ((millis() - _staTryStartMs) < WIFI_CONNECT_TIMEOUT_MS) return;
        _completeStaTryFailure(curEnum);
    }

    bool _reconnectPaused() const {
        return (int32_t)(millis() - _reconnectPausedUntilMs) < 0;
    }

    void _pauseStaReconnect(uint32_t durationMs) {
        if (durationMs == 0) return;
        _reconnectPausedUntilMs = millis() + durationMs;
    }

    void _ensureMDNSStarted() {
        if (_mdnsStarted) return;
        if (MDNS.begin("rectcolumn")) {
            MDNS.addService("http", "tcp", 80);
            _mdnsStarted = true;
        }
    }

    void _refreshApClientSignal() {
        _apClientCount = 0;
        _apClientRssi = -127;
        if (!_apRunning) return;

        wifi_sta_list_t staList = {};
        if (esp_wifi_ap_get_sta_list(&staList) != ESP_OK) return;

        _apClientCount = staList.num;
        if (_apClientCount <= 0) return;

#if RC_HAS_SOFTAP_CLIENT_RSSI
        int bestRssi = -127;
        for (int i = 0; i < _apClientCount; i++) {
            if (staList.sta[i].rssi > bestRssi) bestRssi = staList.sta[i].rssi;
        }
        _apClientRssi = bestRssi;
#else
        _apClientRssi = -67;
#endif
    }
};
