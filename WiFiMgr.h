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

struct WifiConnectResult {
    bool   ok = false;
    int    status = WL_IDLE_STATUS;
    String statusText;
    String ssid;
    String ip;
    bool   saved = false;
    bool   timedOut = false;
};

class WiFiMgr {
public:
    String apSSID = AP_SSID_DEF;
    String apPass = "";
    String staSSID = "";
    String staPass = "";
    bool   staConnected = false;

    void begin(Storage& stor, EventLog& log) {
        _log = &log;

        stor.loadWifiSTA(staSSID, staPass);
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
        WiFi.setAutoReconnect(true);
        WiFi.setSleep(false);         // AP-режим стабильнее без modem sleep
        WiFi.setHostname(DEVICE_NAME);
        WiFi.mode(WIFI_OFF);
        delay(50);
        WiFi.mode(WIFI_AP_STA);
        delay(50);

        _startAP();
        if (_apRunning) {
            _dns.start(53, "*", WiFi.softAPIP());
        }
        _refreshApClientSignal();

        if (staSSID.length() > 0) {
            _connectSTA();
        }

        _lastReconnectMs = millis();
        _lastStatus = (int)WiFi.status();
        staConnected = (_lastStatus == WL_CONNECTED);

        if (staConnected) {
            _lastStatusText = statusText(_lastStatus);
            _ensureMDNSStarted();
        } else {
            _lastStatusText = staSSID.length() > 0 ? "connecting" : "not configured";
        }

        if (_apRunning) {
            log.add(String("WiFi: AP started - ") + apSSID + " / " + (apPass.length() ? "protected" : "open")
                    + " IP=" + WiFi.softAPIP().toString());
        } else {
            log.add("WiFi: AP start FAILED");
        }
    }

    void loop() {
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

        if (!isConn && staSSID.length() > 0 &&
            (millis() - _lastReconnectMs >= WIFI_RECONNECT_COOLDOWN_MS)) {
            _lastReconnectMs = millis();
            _connectSTA();
        }
    }

    void fillScan(JsonArray arr) {
        const int n = WiFi.scanNetworks(false, true);
        for (int i = 0; i < n; i++) {
            const String ssid = WiFi.SSID(i);
            if (ssid.length() == 0) continue;

            JsonObject it = arr.createNestedObject();
            it["ssid"]    = ssid;
            it["rssi"]    = WiFi.RSSI(i);
            it["enc"]     = (int)WiFi.encryptionType(i);
            it["channel"] = WiFi.channel(i);
        }
        WiFi.scanDelete();
    }

    WifiConnectResult connectSTA(const String& ssid,
                                 const String& pass,
                                 Storage& stor,
                                 uint32_t timeoutMs = WIFI_CONNECT_TIMEOUT_MS) {
        WifiConnectResult res;
        res.ssid = ssid;

        const String prevSSID = staSSID;
        const String prevPass = staPass;

        _connectSTA(ssid, pass);

        const uint32_t started = millis();
        wl_status_t cur = WiFi.status();
        while (cur != WL_CONNECTED && (millis() - started) < timeoutMs) {
            delay(200);
            yield();
            cur = WiFi.status();
        }

        res.status = (int)cur;
        res.timedOut = (cur != WL_CONNECTED) && ((millis() - started) >= timeoutMs);
        res.statusText = res.timedOut
            ? (String("timeout/") + statusText((int)cur))
            : statusText((int)cur);

        if (cur == WL_CONNECTED) {
            staSSID = ssid;
            staPass = pass;
            staConnected = true;
            res.ok = true;
            res.ip = WiFi.localIP().toString();
            stor.saveWifiSTA(ssid, pass);
            res.saved = true;
            _lastReconnectMs = millis();
            _lastStatus = (int)cur;
            _lastStatusText = statusText((int)cur);
            _ensureMDNSStarted();
            return res;
        }

        // Не подменяем рабочую конфигурацию неудачной попыткой подключения.
        staConnected = false;
        if (prevSSID.length() > 0) {
            staSSID = prevSSID;
            staPass = prevPass;
            _connectSTA();
            _lastStatus = (int)WiFi.status();
            _lastStatusText = statusText(_lastStatus);
        } else {
            staSSID = "";
            staPass = "";
            _lastStatus = (int)cur;
            _lastStatusText = res.statusText;
        }

        return res;
    }

    void setAPPassword(const String& pass, Storage& stor) {
        apPass = pass;
        stor.saveAPPassword(pass);
        _startAP();
        if (_apFallbackToOpen) {
            stor.saveAPPassword("");
        }
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

    bool staConfigured() const { return staSSID.length() > 0; }
    bool apProtected() const { return apPass.length() > 0; }
    bool apRunning() const { return _apRunning; }
    bool apFallbackToOpen() const { return _apFallbackToOpen; }
    String apStatusText() const { return _apStatusText; }
    String staStatusText() const { return _lastStatusText; }
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

private:
    DNSServer _dns;
    EventLog* _log = nullptr;
    uint32_t  _lastReconnectMs = 0;
    int       _lastStatus = WL_IDLE_STATUS;
    String    _lastStatusText = "idle";
    bool      _mdnsStarted = false;
    bool      _apRunning = false;
    bool      _apFallbackToOpen = false;
    String    _apStatusText = "idle";
    uint32_t  _lastApSignalPollMs = 0;
    int       _apClientCount = 0;
    int       _apClientRssi = -127;

    void _startAP() {
        _apRunning = false;
        _apFallbackToOpen = false;
        _apStatusText = "failed";

        WiFi.softAPdisconnect(true);
        delay(50);

        bool ok = false;
        if (apPass.length() > 0) ok = WiFi.softAP(apSSID.c_str(), apPass.c_str());
        else                     ok = WiFi.softAP(apSSID.c_str());

        if (!ok && apPass.length() > 0) {
            apPass = "";
            ok = WiFi.softAP(apSSID.c_str());
            if (ok) {
                _apFallbackToOpen = true;
            }
        }

        _apRunning = ok;
        if (_apRunning) {
            _apStatusText = _apFallbackToOpen
                ? "fallback_open"
                : (apPass.length() > 0 ? "protected" : "open");
        }

        _refreshApClientSignal();
    }

    void _connectSTA() {
        if (staSSID.length() == 0) return;
        _connectSTA(staSSID, staPass);
    }

    void _connectSTA(const String& ssid, const String& pass) {
        WiFi.disconnect(false, false);
        delay(100);
        WiFi.begin(ssid.c_str(), pass.c_str());
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
