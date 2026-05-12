// ================================================================
// WebAPI.h 
// ================================================================
#pragma once

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <string.h>

#include "config.h"
#include "TimeBase.h"
#include "EventLog.h"
#include "SensorManager.h"
#include "OutputManager.h"
#include "ConfirmationManager.h"
#include "WiFiMgr.h"
#include "Storage.h"
#include "Emulator.h"
#include "RemoteNotifier.h"
#include "ProcessSafety.h"
#include "WebPageRoot.h"
#include "WebPageWifi.h"
#include "WebPageApp.h"
#include "WebPageAppCss.h"
#include "WebPageAppJs.h"

class WebAPI {
public:
    explicit WebAPI(uint16_t port = 80) : _server(port) {}

    void begin(TimeBase& tb, EventLog& log,
               SensorManager& sm, OutputManager& om, ConfirmationManager& cm,
               WiFiMgr& wifi, Storage& stor, Emulator& emu, RemoteNotifier& notifier,
               ProcessSafety& processSafety)
    {
        _tb   = &tb;
        _log  = &log;
        _sm   = &sm;
        _om   = &om;
        _cm   = &cm;
        _wifi = &wifi;
        _stor = &stor;
        _emu  = &emu;
        _notifier = &notifier;
        _processSafety = &processSafety;

        _installCors();
        _registerServicePages();
        _registerFrontendRoutes();
        _registerInfoRoutes();
        _registerCoreRoutes();
        _registerSensorRoutes();
        _registerOutputRoutes();
        _registerWifiRoutes();
        _registerNotifyRoutes();
        _registerEmuRoutes();
        _registerFallbacks();

        _server.begin();
    }

private:
    AsyncWebServer   _server;
    TimeBase*        _tb   = nullptr;
    EventLog*        _log  = nullptr;
    SensorManager*   _sm   = nullptr;
    OutputManager*   _om   = nullptr;
    ConfirmationManager* _cm = nullptr;
    WiFiMgr*         _wifi = nullptr;
    Storage*         _stor = nullptr;
    Emulator*        _emu  = nullptr;
    RemoteNotifier*  _notifier = nullptr;
    ProcessSafety*   _processSafety = nullptr;

    // ------------------------------------------------------------
    // Route registration
    // ------------------------------------------------------------
    void _registerServicePages() {
        _server.on("/", HTTP_GET, [this](AsyncWebServerRequest* req) {
            if (_wifiWizardPending()) {
                req->redirect("/wifi");
                return;
            }
            _sendGzip(req, "text/html; charset=utf-8", PAGE_ROOT_V2_GZ, PAGE_ROOT_V2_GZ_LEN);
        });

        _server.on("/wifi", HTTP_GET, [this](AsyncWebServerRequest* req) {
            _sendGzip(req, "text/html; charset=utf-8", PAGE_WIFI_V2_GZ, PAGE_WIFI_V2_GZ_LEN);
        });

        _server.on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->redirect("/wifi");
        });
        _server.on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->redirect("/wifi");
        });
        _server.on("/connecttest.txt", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->redirect("/wifi");
        });
        _server.on("/ncsi.txt", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->redirect("/wifi");
        });
        _server.on("/fwlink", HTTP_GET, [this](AsyncWebServerRequest* req) {
            req->redirect("/wifi");
        });
    }

    void _registerFrontendRoutes() {
        _server.on("/app", HTTP_GET, [this](AsyncWebServerRequest* req) {
            if (_wifiWizardPending()) {
                req->redirect("/wifi");
                return;
            }
            AsyncWebServerResponse* resp = req->beginResponse_P(200, "text/html; charset=utf-8", PAGE_APP_HTML);
            resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            resp->addHeader("Pragma", "no-cache");
            resp->addHeader("Expires", "0");
            req->send(resp);
        });

        _server.on("/app.css", HTTP_GET, [this](AsyncWebServerRequest* req) {
            _sendGzip(req, "text/css; charset=utf-8", PAGE_APP_CSS_GZ, PAGE_APP_CSS_GZ_LEN, "max-age=86400, immutable");
        });

        _server.on("/app.js", HTTP_GET, [this](AsyncWebServerRequest* req) {
            _sendGzip(req, "application/javascript; charset=utf-8", PAGE_APP_JS_GZ, PAGE_APP_JS_GZ_LEN, "max-age=86400, immutable");
        });
    }

    void _registerInfoRoutes() {
        _server.on("/api/v1/info", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(6144);
            JsonObject root = doc.to<JsonObject>();
            _buildInfo(root);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/version", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(256);
            doc["ok"]         = true;
            doc["name"]       = DEVICE_NAME;
            doc["fw"]         = FW_VERSION;
            doc["apiVersion"] = API_VERSION;
            doc["emu"]        = EMU_MODE;
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/health", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(512);
            doc["ok"]           = true;
            doc["time"]         = _tb->nowStr();
            doc["synced"]       = _tb->isSynced();
            doc["uptimeMs"]     = (uint64_t)millis();
            doc["emu"]          = EMU_MODE;
            doc["staConnected"] = _wifi->staConnected;
            doc["staIP"]        = _wifi->staIP();
            doc["apIP"]         = _wifi->apIP();
            doc["rssi"]         = _wifi->rssi();
            doc["staRssi"]      = _wifi->staRssi();
            doc["apRssi"]       = _wifi->apRssi();
            doc["apClientCount"] = _wifi->apClientCount();
            doc["wifiWizardPending"] = _wifiWizardPending();
            doc["tzOffsetMin"] = _tb->tzOffsetMin();
            doc["localTimeMode"] = _tb->isSynced();
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/schema", HTTP_GET, [this](AsyncWebServerRequest* req) {
            // Compact schema: the UI only needs ids and a positive ok response.
            // Keeping this endpoint small leaves more flash for the application
            // while preserving API compatibility for the web client.
            DynamicJsonDocument doc(2048);
            doc["ok"] = true;
            doc["uiVersion"] = 1;

            JsonArray sensors = doc.createNestedArray("sensorIds");
            for (int i = 0; i < SEN_COUNT; i++) sensors.add(SensorManager::sensorName(i));

            JsonArray outputs = doc.createNestedArray("outputIds");
            for (int i = 0; i < OUT_COUNT; i++) outputs.add(_outputName(i));

            JsonArray confirmations = doc.createNestedArray("confirmationIds");
            confirmations.add("WER_CH1");
            confirmations.add("WER_CH2");
            confirmations.add("WER_CH3");
            confirmations.add("WER_CH4");

            JsonObject timeSync = doc.createNestedObject("timeSync");
            timeSync["supportsMillisAtSend"] = true;
            timeSync["supportsTzOffsetMin"] = true;

            doc["stopEndpoint"] = "/api/v1/stop";
            doc["stopReleaseEndpoint"] = "/api/v1/stop?release=1";
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/diag", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(4096);
            JsonObject root = doc.to<JsonObject>();
            _buildDiag(root);
            _sendDoc(req, 200, doc);
        });
    }

    void _registerCoreRoutes() {
        _server.on("/api/v1/state", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(16384);
            JsonObject root = doc.to<JsonObject>();
            _buildState(root);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/sensor", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(8192);
            JsonArray arr = doc.to<JsonArray>();
            _buildSensors(arr);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/output", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(6144);
            JsonArray arr = doc.to<JsonArray>();
            _buildOutputs(arr);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/time/sync", HTTP_POST,
            [](AsyncWebServerRequest*) {},
            nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(256);
                if (!_parseJson(req, data, len, doc)) return;

                if (!doc.containsKey("unixTimeMs")) {
                    _sendError(req, 400, "bad_params", "unixTimeMs is required");
                    return;
                }

                const uint64_t unixMs = doc["unixTimeMs"].as<uint64_t>();
                const uint32_t msAt = doc["millisAtSend"] | 0U;
                const int32_t tzOffsetMin = doc["tzOffsetMin"] | 0;

                _tb->sync(unixMs, msAt, tzOffsetMin);
                _log->refreshTimeStrings();
                _log->add("Время синхронизировано", _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(320);
                resp["ok"]            = true;
                resp["synced"]        = true;
                resp["time"]          = _tb->nowStr();
                resp["refMsUsed"]     = _tb->lastSyncRefMs();
                resp["tzOffsetMin"]   = _tb->tzOffsetMin();
                resp["localTimeMode"] = true;
                _sendDoc(req, 200, resp);
            });

        _server.on("/api/v1/mute", HTTP_POST,
            [](AsyncWebServerRequest*) {},
            nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(128);
                if (!_parseJson(req, data, len, doc)) return;
                if (!doc.containsKey("muted") && !doc.containsKey("state")) {
                    _sendError(req, 400, "bad_params", "muted is required");
                    return;
                }

                const bool muted = doc.containsKey("muted") ? (bool)doc["muted"] : (bool)doc["state"];
                _om->mute(muted);
                _stor->saveOutputs(*_om);
                _om->beepAcceptedCommand();
                _log->add(String("Звук ") + (muted ? "приглушён" : "включён"),
                          _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(256);
                resp["ok"] = true;
                resp["muted"] = muted;
                _sendDoc(req, 200, resp);
            });

        auto handleAckRequest = [this](AsyncWebServerRequest* req) {
            const bool compatGet = (req->method() == HTTP_GET);

            const uint16_t activeBefore = _om->activeAlarmCount(*_sm);
            _om->acknowledgeCurrentAlarms(*_sm);
            _om->loop(*_sm);
            const uint16_t unackedAfter = _om->unackedAlarmCount(*_sm);
            const uint16_t acknowledgedCount = (activeBefore >= unackedAfter) ? (activeBefore - unackedAfter) : 0;
            if (acknowledgedCount > 0) {
                _log->add("Оператор подтвердил тревоги", _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
            }
            DynamicJsonDocument resp(448);
            resp["ok"] = true;
            resp["activeAlarmCount"] = activeBefore;
            resp["acknowledgedCount"] = acknowledgedCount;
            resp["unackedAlarmCount"] = unackedAfter;
            resp["muted"] = _om->soundMuted;
            resp["ch4Enabled"] = _om->ch4Enabled;
            resp["ch5Enabled"] = _om->ch5Enabled;
            resp["ch4Actual"] = _om->out[OUT_CH4]->actualOn();
            resp["ch5Actual"] = _om->out[OUT_CH5]->actualOn();
            resp["compatFallback"] = compatGet;
            _sendDoc(req, 200, resp);
        };

        _server.on("/api/v1/ack", HTTP_POST, [handleAckRequest](AsyncWebServerRequest* req) {
            handleAckRequest(req);
        });
        _server.on("/api/v1/ack/", HTTP_POST, [handleAckRequest](AsyncWebServerRequest* req) {
            handleAckRequest(req);
        });
        _server.on("/api/v1/ack", HTTP_GET, [handleAckRequest](AsyncWebServerRequest* req) {
            handleAckRequest(req);
        });
        _server.on("/api/v1/ack/", HTTP_GET, [handleAckRequest](AsyncWebServerRequest* req) {
            handleAckRequest(req);
        });

        _server.on("/api/v1/safety/reset", HTTP_POST, [this](AsyncWebServerRequest* req) {
            if (_processSafety) _processSafety->resetLatchedFaults();
            _om->setSafetyAlarmActive(_processSafety ? _processSafety->safetyAlarmActive() : false);
            _stor->saveOutputs(*_om);
            DynamicJsonDocument resp(384);
            resp["ok"] = true;
            resp["safetyAlarmActive"] = _om->safetyAlarmActive();
            resp["stopLatched"] = _om->mainStopLatched();
            _sendDoc(req, 200, resp);
        });

        _server.on("/api/v1/log", HTTP_GET, [this](AsyncWebServerRequest* req) {
            _log->refreshTimeStrings();
            String json = "[";
            const int logCount = _log->count();
            for (int displayIdx = 0; displayIdx < logCount; displayIdx++) {
                if (displayIdx > 0) json += ",";
                const int i = logCount - 1 - displayIdx;
                const LogEntry& e = _log->get(i);
                json += "{\"t\":\"" + _jsonEscape(e.timeStr) + "\",";
                json += "\"ms\":" + String((uint32_t)e.absMs) + ",";
                json += "\"e\":\"" + _jsonEscape(e.event) + "\",";
                json += "\"T1\":" + _fmtVal(e.T1) + ",";
                json += "\"T2\":" + _fmtVal(e.T2) + ",";
                json += "\"T3\":" + _fmtVal(e.T3) + ",";
                json += "\"dT\":" + _fmtVal(e.dT) + "}";
            }
            json += "]";
            AsyncWebServerResponse* resp = req->beginResponse(200, "application/json; charset=utf-8", json);
            resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
            resp->addHeader("Pragma", "no-cache");
            resp->addHeader("Expires", "0");
            req->send(resp);
        });

        _server.on("/api/v1/log", HTTP_DELETE, [this](AsyncWebServerRequest* req) {
            _log->clear();
            _log->add("Журнал очищен", _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
            _sendOk(req);
        });
    }

    void _registerSensorRoutes() {
        for (int si = 0; si < SEN_COUNT; si++) {
            const String sid = SensorManager::sensorName(si);
            const String base = "/api/v1/sensor/" + sid;

            _server.on((base + "/config").c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, si](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleSensorConfig(req, si, data, len);
                });

            _server.on((base + "/alarm").c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, si](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleSensorAlarm(req, si, data, len);
                });

            _server.on((base + "/ctrl").c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, si](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleSensorCtrl(req, si, data, len);
                });
        }
    }

    void _registerOutputRoutes() {
        for (int oi = 0; oi < OUT_COUNT; oi++) {
            const String route = String("/api/v1/output/") + _outputName(oi) + "/manual";
            _server.on(route.c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, oi](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleOutputManual(req, oi, data, len);
                });
        }

        for (int oi = OUT_CH1; oi <= OUT_CH3; oi++) {
            const String id = _outputName(oi);
            const String cmdV1 = String("/api/v1/relay/") + id + "/command";
            const String cmdCompat = String("/api/relay/") + id + "/command";
            const String stateV1 = String("/api/v1/relay/") + id + "/state";
            const String stateCompat = String("/api/relay/") + id + "/state";

            _server.on(cmdV1.c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, oi](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleRelayCommand(req, oi, data, len, false);
                });
            _server.on(cmdCompat.c_str(), HTTP_POST,
                [](AsyncWebServerRequest*) {}, nullptr,
                [this, oi](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                    _handleRelayCommand(req, oi, data, len, false);
                });

            _server.on(stateV1.c_str(), HTTP_GET, [this, oi](AsyncWebServerRequest* req) {
                _handleRelayState(req, oi);
            });
            _server.on(stateCompat.c_str(), HTTP_GET, [this, oi](AsyncWebServerRequest* req) {
                _handleRelayState(req, oi);
            });
        }

        auto handleStopCommandRequest = [this](AsyncWebServerRequest* req) {
            // Use one stable endpoint for both actions.  The main STOP button is
            // known to reach /api/v1/stop on deployed devices; release is now
            // selected by query parameters instead of depending on a separate
            // nested route that some builds/proxies missed.
            if (_requestWantsStopRelease(req)) _handleReleaseStopMainOutputs(req);
            else _handleStopMainOutputs(req);
        };
        auto handleReleaseStopMainRequest = [this](AsyncWebServerRequest* req) {
            _handleReleaseStopMainOutputs(req);
        };

        _server.on("/api/v1/stop", HTTP_POST, handleStopCommandRequest);
        _server.on("/api/v1/stop/", HTTP_POST, handleStopCommandRequest);

        // Compatibility aliases.  The frontend no longer depends on these paths,
        // but direct API clients and older pages may still call them.
        _server.on("/api/v1/stop/release", HTTP_POST, handleReleaseStopMainRequest);
        _server.on("/api/v1/stop/release/", HTTP_POST, handleReleaseStopMainRequest);

        auto fillOutputConfigDoc = [this](DynamicJsonDocument& doc) {
            doc["ok"] = true;

            JsonArray outputs = doc.createNestedArray("outputs");
            for (int i = 0; i < 3; i++) {
                JsonObject o = outputs.createNestedObject();
                o["id"] = _outputName(i);
                o["mode"] = (_om->chMode[i] == LOGIC_COOL) ? "cool" : "heat";
            }

            doc["ch4Enabled"] = _om->ch4Enabled;
            doc["ch5Enabled"] = _om->ch5Enabled;
            doc["soundMuted"] = _om->soundMuted;
        };

        _server.on("/api/v1/output/config", HTTP_GET, [this, fillOutputConfigDoc](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(512);
            fillOutputConfigDoc(doc);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/output/config", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this, fillOutputConfigDoc](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(512);
                if (!_parseJson(req, data, len, doc)) return;

                uint8_t nextMode[3] = { _om->chMode[0], _om->chMode[1], _om->chMode[2] };
                bool nextCh4Enabled = _om->ch4Enabled;
                bool nextCh5Enabled = _om->ch5Enabled;
                bool nextSoundMuted = _om->soundMuted;

                if (doc.containsKey("outputs") && doc["outputs"].is<JsonArray>()) {
                    JsonArray arr = doc["outputs"].as<JsonArray>();
                    for (JsonObject item : arr) {
                        String id = item["id"] | "";
                        int oi = -1;
                        if (id == "CH1") oi = OUT_CH1;
                        else if (id == "CH2") oi = OUT_CH2;
                        else if (id == "CH3") oi = OUT_CH3;

                        if (oi >= 0 && oi <= OUT_CH3 && item.containsKey("mode")) {
                            String mode = item["mode"].as<String>();
                            mode.trim();
                            mode.toLowerCase();
                            if (mode != "heat" && mode != "cool") {
                                _sendError(req, 400, "bad_params", "mode must be 'heat' or 'cool'");
                                return;
                            }
                            nextMode[oi] = (mode == "cool") ? LOGIC_COOL : LOGIC_HEAT;
                        }
                    }
                }

                if (doc.containsKey("ch4Enabled")) nextCh4Enabled = doc["ch4Enabled"].as<bool>();
                if (doc.containsKey("ch5Enabled")) nextCh5Enabled = doc["ch5Enabled"].as<bool>();
                if (doc.containsKey("soundMuted")) nextSoundMuted = doc["soundMuted"].as<bool>();

                for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++) {
                    if (nextMode[oi] != _om->chMode[oi]) _applyOutputLogicMode(oi, nextMode[oi]);
                }
                _om->ch4Enabled = nextCh4Enabled;
                _om->ch5Enabled = nextCh5Enabled;
                _om->mute(nextSoundMuted);

                _om->applyConfig();
                _stor->saveOutputConfig(*_om);
                _stor->saveSensors(*_sm);
                _om->beepAcceptedCommand();

                _log->add("Конфигурация выходов обновлена",
                          _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(512);
                fillOutputConfigDoc(resp);
                _sendDoc(req, 200, resp);
            });
    }

    void _registerWifiRoutes() {
        _server.on("/api/v1/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(4096);
            JsonArray arr = doc.to<JsonArray>();
            _wifi->fillScan(arr);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/wifi/connect", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(384);
                if (!_parseJson(req, data, len, doc)) return;

                String ssid = doc["ssid"] | "";
                String pass = doc["pass"] | "";
                if (ssid.length() == 0) {
                    _sendError(req, 400, "bad_params", "ssid is required");
                    return;
                }

                WifiConnectResult result = _wifi->connectSTA(ssid, pass, *_stor, WIFI_CONNECT_TIMEOUT_MS);
                if (result.ok) {
                    _stor->saveWifiWizardDone(true);
                    _om->beepAcceptedCommand();
                }

                DynamicJsonDocument resp(640);
                resp["ok"]             = result.ok;
                resp["ssid"]           = result.ssid;
                resp["ip"]             = result.ip;
                resp["status"]         = result.status;
                resp["statusText"]     = result.statusText;
                resp["saved"]          = result.saved;
                resp["timedOut"]       = result.timedOut;
                resp["staIP"]          = _wifi->staIP();
                resp["apStillAvailable"] = true;
                resp["wizardPending"]  = _wifiWizardPending();

                if (result.ok) {
                    _log->add("WiFi STA подключён: " + result.ssid + " IP=" + result.ip,
                              _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
                } else {
                    _log->add("WiFi STA: ошибка подключения к " + result.ssid + " status=" + result.statusText,
                              _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
                }

                _sendDoc(req, 200, resp);
            });

        _server.on("/api/v1/wifi/ap", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(256);
                if (!_parseJson(req, data, len, doc)) return;

                String pass = doc["pass"] | "";
                if (pass.length() != 0 && (pass.length() < 8 || pass.length() > 63)) {
                    _sendError(req, 400, "bad_params", "ap password must be empty or 8..63 chars");
                    return;
                }

                _wifi->setAPPassword(pass, *_stor);
                _stor->saveWifiWizardDone(true);
                _om->beepAcceptedCommand();
                _log->add(String("Пароль точки доступа ") + (pass.length() ? "установлен" : "сброшен"),
                          _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(320);
                resp["ok"] = true;
                resp["protected"] = _wifi->apProtected();
                resp["apIP"] = _wifi->apIP();
                resp["wizardPending"] = _wifiWizardPending();
                resp["reconnectHint"] = pass.length()
                    ? "Точке доступа может потребоваться переподключение с новым паролем"
                    : "Точка доступа остаётся открытой";
                _sendDoc(req, 200, resp);
            });

        _server.on("/api/v1/wifi/wizard/complete", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                DynamicJsonDocument doc(128);
                if (len > 0 && !_parseJson(req, data, len, doc)) return;

                const bool done = doc["done"] | true;
                _stor->saveWifiWizardDone(done);
                _om->beepAcceptedCommand();
                _log->add(String("Мастер WiFi ") + (done ? "завершён" : "открыт повторно"),
                          _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(192);
                resp["ok"] = true;
                resp["wizardPending"] = _wifiWizardPending();
                _sendDoc(req, 200, resp);
            });
    }

    void _registerNotifyRoutes() {
        _server.on("/api/v1/notify/config", HTTP_GET, [this](AsyncWebServerRequest* req) {
            DynamicJsonDocument doc(384);
            doc["ok"] = true;
            doc["enabled"] = (_notifier ? _notifier->enabled() : false);
            doc["url"] = (_notifier ? _notifier->publishUrl() : String(""));
            doc["token"] = (_notifier ? _notifier->accessToken() : String(""));
            doc["hasToken"] = (_notifier ? _notifier->hasToken() : false);
            _sendDoc(req, 200, doc);
        });

        _server.on("/api/v1/notify/config", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                if (!_notifier) {
                    _sendError(req, 500, "notifier_missing", "notifier is not initialized");
                    return;
                }

                DynamicJsonDocument doc(512);
                if (!_parseJson(req, data, len, doc)) return;

                const bool enabled = doc["enabled"] | false;
                String url = doc["url"] | "";
                String token = doc.containsKey("token")
                    ? String(doc["token"] | "")
                    : _notifier->accessToken();
                url.trim();
                token.trim();

                if (url.length() > 0) {
                    String err;
                    if (!_notifier->validatePublishUrl(url, err)) {
                        _sendError(req, 400, "bad_params", err.c_str());
                        return;
                    }
                }

                _notifier->setConfig(enabled, url, token);
                _om->beepAcceptedCommand();
                _log->add("Настройки уведомлений обновлены",
                          _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

                DynamicJsonDocument resp(256);
                resp["ok"] = true;
                resp["enabled"] = _notifier->enabled();
                resp["url"] = _notifier->publishUrl();
                resp["hasToken"] = _notifier->hasToken();
                _sendDoc(req, 200, resp);
            });

        _server.on("/api/v1/notify/test", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
                if (!_notifier) {
                    _sendError(req, 500, "notifier_missing", "notifier is not initialized");
                    return;
                }

                DynamicJsonDocument doc(384);
                if (len > 0 && !_parseJson(req, data, len, doc)) return;

                String url = doc["url"] | _notifier->publishUrl();
                String token = doc["token"] | _notifier->accessToken();
                url.trim();
                token.trim();

                if (url.length() > 0) {
                    String err;
                    if (!_notifier->validatePublishUrl(url, err)) {
                        _sendError(req, 400, "bad_params", err.c_str());
                        return;
                    }
                }

                String err;
                const bool ok = _notifier->sendTestTo(url, token, err);

                DynamicJsonDocument resp(256);
                resp["ok"] = ok;
                if (ok) {
                    _om->beepAcceptedCommand();
                    _log->add("Тест уведомления отправлен",
                              _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
                    _sendDoc(req, 200, resp);
                } else {
                    resp["error"] = err;
                    resp["err"] = err;
                    _sendDoc(req, 400, resp);
                }
            });
    }

    void _registerEmuRoutes() {
        _server.on("/api/v1/emu/set", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            #if EMU_MODE
                DynamicJsonDocument doc(512);
                if (!_parseJson(req, data, len, doc)) return;

                if (doc.containsKey("T1"))    _emu->val.T1 = doc["T1"].as<float>();
                if (doc.containsKey("T2"))    _emu->val.T2 = doc["T2"].as<float>();
                if (doc.containsKey("T3"))    _emu->val.T3 = doc["T3"].as<float>();
                if (doc.containsKey("P"))     _emu->val.P  = doc["P"].as<float>();
                if (doc.containsKey("L"))     _emu->val.L  = doc["L"];
                if (doc.containsKey("F"))     _emu->val.F  = doc["F"];
                if (doc.containsKey("C"))     _emu->val.C  = doc["C"].as<float>();
                if (doc.containsKey("V"))     _emu->val.V  = doc["V"].as<float>();
                if (doc.containsKey("T1err")) _emu->val.T1err = doc["T1err"];
                if (doc.containsKey("T2err")) _emu->val.T2err = doc["T2err"];
                if (doc.containsKey("T3err")) _emu->val.T3err = doc["T3err"];

                if (doc.containsKey("WER_CH1")) _cm->setEmuActive(0, doc["WER_CH1"]);
                if (doc.containsKey("WER_CH2")) _cm->setEmuActive(1, doc["WER_CH2"]);
                if (doc.containsKey("WER_CH3")) _cm->setEmuActive(2, doc["WER_CH3"]);
                if (doc.containsKey("WER_CH4")) _cm->setEmuActive(3, doc["WER_CH4"]);
                if (doc.containsKey("WER_CH1_mode")) _cm->setEmuMode(0, _parseEmuConfirmMode(doc["WER_CH1_mode"]));
                if (doc.containsKey("WER_CH2_mode")) _cm->setEmuMode(1, _parseEmuConfirmMode(doc["WER_CH2_mode"]));
                if (doc.containsKey("WER_CH3_mode")) _cm->setEmuMode(2, _parseEmuConfirmMode(doc["WER_CH3_mode"]));
                if (doc.containsKey("WER_CH4_mode")) _cm->setEmuMode(3, _parseEmuConfirmMode(doc["WER_CH4_mode"]));
                if (doc.containsKey("W1")) _cm->setEmuActive(0, doc["W1"]);
                if (doc.containsKey("W2")) _cm->setEmuActive(1, doc["W2"]);
                if (doc.containsKey("W3")) _cm->setEmuActive(2, doc["W3"]);
                if (doc.containsKey("W4")) _cm->setEmuActive(3, doc["W4"]);
                if (doc.containsKey("W1_mode")) _cm->setEmuMode(0, _parseEmuConfirmMode(doc["W1_mode"]));
                if (doc.containsKey("W2_mode")) _cm->setEmuMode(1, _parseEmuConfirmMode(doc["W2_mode"]));
                if (doc.containsKey("W3_mode")) _cm->setEmuMode(2, _parseEmuConfirmMode(doc["W3_mode"]));
                if (doc.containsKey("W4_mode")) _cm->setEmuMode(3, _parseEmuConfirmMode(doc["W4_mode"]));

                _sendOk(req);
            #else
                _sendError(req, 403, "forbidden", "Режим эмуляции отключён");
            #endif
            });

        _server.on("/api/v1/emu/scenario", HTTP_POST,
            [](AsyncWebServerRequest*) {}, nullptr,
            [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t, size_t) {
            #if EMU_MODE
                DynamicJsonDocument doc(128);
                if (!_parseJson(req, data, len, doc)) return;
                String name = doc["name"] | "none";
                _emu->setScenario(name);
                _log->add("Сценарий эмуляции: " + name, _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
                _sendOk(req);
            #else
                _sendError(req, 403, "forbidden", "Режим эмуляции отключён");
            #endif
            });
    }

    void _registerFallbacks() {
        _server.onNotFound([this](AsyncWebServerRequest* req) {
            if (req->method() == HTTP_OPTIONS) {
                req->send(204);
                return;
            }

            const String url = req->url();
            // Defensive compatibility fallback for the critical STOP endpoints.
            // If the normal route table misses the request for any reason, do
            // not return a generic 404 to the operator button.
            if (req->method() == HTTP_POST) {
                const bool stopPath = (url == "/api/v1/stop" || url == "/api/v1/stop/" ||
                                       url.startsWith("/api/v1/stop?"));
                const bool releasePath = (url == "/api/v1/stop/release" || url == "/api/v1/stop/release/" ||
                                          url.startsWith("/api/v1/stop/release?"));
                if (releasePath) {
                    _handleReleaseStopMainOutputs(req);
                    return;
                }
                if (stopPath) {
                    if (_requestWantsStopRelease(req)) _handleReleaseStopMainOutputs(req);
                    else _handleStopMainOutputs(req);
                    return;
                }
            }

            if (url.startsWith("/api/")) {
                DynamicJsonDocument doc(256);
                doc["ok"] = false;
                doc["code"] = 404;
                doc["error"] = "not found";
                doc["err"] = "not found";
                doc["path"] = req->url();
                _sendDoc(req, 404, doc);
                return;
            }

            if ((req->method() == HTTP_GET || req->method() == HTTP_HEAD) && _wifiWizardPending()) {
                req->redirect("/wifi");
                return;
            }

            req->send(404, "text/html; charset=utf-8",
                "<!doctype html><html><body><h1>RectColumn</h1><p>Page not found.</p><p><a href='/'>root</a> | <a href='/wifi'>wifi</a> | <a href='/app'>app</a></p></body></html>");
        });
    }

    void _installCors() {
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");
        DefaultHeaders::Instance().addHeader("Access-Control-Max-Age", "600");
        // Cache-Control is set per response so static assets can be cached
        // without inheriting a global no-store directive.
    }

    // ------------------------------------------------------------
    // Handlers
    // ------------------------------------------------------------
    void _handleSensorConfig(AsyncWebServerRequest* req, int si, uint8_t* data, size_t len) {
        if (si < 0 || si >= SEN_COUNT) {
            _sendError(req, 404, "not_found", "sensor not found");
            return;
        }

        DynamicJsonDocument doc(256);
        if (!_parseJson(req, data, len, doc)) return;

        SensorBase* s = _sm->s[si];

        bool nextEnabled = s->enabled;
        uint32_t nextPeriodMs = s->periodMs;
        uint32_t nextAlarmDelayMs = s->alarmDelayMs;
        uint32_t nextCtrlDelayMs = s->ctrlDelayMs;

        if (doc.containsKey("enabled")) nextEnabled = doc["enabled"];
        if (doc.containsKey("periodMs")) {
            nextPeriodMs = doc["periodMs"].as<uint32_t>();
            if (nextPeriodMs < SENSOR_PERIOD_MIN_MS || nextPeriodMs > SENSOR_PERIOD_MAX_MS) {
                _sendError(req, 400, "bad_params", "periodMs out of range");
                return;
            }
        }
        if (doc.containsKey("alarmDelayMs")) {
            nextAlarmDelayMs = doc["alarmDelayMs"].as<uint32_t>();
            if (nextAlarmDelayMs > SENSOR_ALARM_DELAY_MAX_MS) {
                _sendError(req, 400, "bad_params", "alarmDelayMs out of range");
                return;
            }
        }
        if (doc.containsKey("ctrlDelayMs")) {
            nextCtrlDelayMs = doc["ctrlDelayMs"].as<uint32_t>();
            if (nextCtrlDelayMs > SENSOR_CTRL_DELAY_MAX_MS) {
                _sendError(req, 400, "bad_params", "ctrlDelayMs out of range");
                return;
            }
        }

        s->enabled = nextEnabled;
        s->periodMs = nextPeriodMs;
        s->alarmDelayMs = nextAlarmDelayMs;
        s->ctrlDelayMs = nextCtrlDelayMs;

        _stor->saveSensors(*_sm);
        _om->beepAcceptedCommand();
        _log->add("Настройка датчика: " + s->name + " " + String(s->enabled ? "включён" : "отключён"),
                  _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
        _sendOk(req);
    }

    void _handleSensorAlarm(AsyncWebServerRequest* req, int si, uint8_t* data, size_t len) {
        if (si < 0 || si >= SEN_COUNT) {
            _sendError(req, 404, "not_found", "sensor not found");
            return;
        }

        DynamicJsonDocument doc(256);
        if (!_parseJson(req, data, len, doc)) return;

        const int ai = doc["idx"] | -1;
        if (ai < 0 || ai >= N_ALARMS) {
            _sendError(req, 400, "bad_params", "alarm idx out of range");
            return;
        }

        SensorBase* s = _sm->s[si];
        if (doc.containsKey("enabled"))   s->alarm[ai].enabled = doc["enabled"];
        if (doc.containsKey("threshold")) s->alarm[ai].threshold = doc["threshold"].as<float>();
        if (doc.containsKey("isMax"))     s->alarm[ai].isMax = doc["isMax"];

        _stor->saveSensors(*_sm);
        _om->beepAcceptedCommand();
        _log->add("Настройка тревоги: " + s->name + " #" + String(ai),
                  _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
        _sendOk(req);
    }

    void _handleSensorCtrl(AsyncWebServerRequest* req, int si, uint8_t* data, size_t len) {
        if (si < 0 || si >= SEN_COUNT) {
            _sendError(req, 404, "not_found", "sensor not found");
            return;
        }

        DynamicJsonDocument doc(256);
        if (!_parseJson(req, data, len, doc)) return;

        const int oi = doc["outIdx"] | -1;
        if (oi < 0 || oi >= N_CTRL_OUT) {
            _sendError(req, 400, "bad_params", "outIdx out of range");
            return;
        }

        SensorBase* s = _sm->s[si];
        CtrlRule& r = s->ctrl[oi];
        CtrlRule next = r;
        const bool fixedOffOnly = SensorManager::isDigitalOffOnlyRule((uint8_t)si, (uint8_t)oi);

        if (doc.containsKey("enabled")) next.enabled = doc["enabled"];

        if (!SensorManager::isRuleAllowedForOutput((uint8_t)si, (uint8_t)oi)) {
            if (next.enabled) {
                _sendError(req, 400, "bad_params", "sensor is not allowed to control CH1..CH3 in scheme mode");
                return;
            }
            next.enabled = false;
            next.outIdx = oi;
            r = next;
            _sm->normalizeSchemeControlRules();
            _stor->saveSensors(*_sm);
            _om->beepAcceptedCommand();
            _sendOk(req);
            return;
        }

        if (fixedOffOnly) {
            // L/F are binary OFF-only conditions for CH1..CH3. The UI may send
            // stale heat/cool/min/max values, but firmware keeps the safe rule.
            next.outIdx = oi;
            next.logic  = LOGIC_COOL;
            next.minVal = 0.5f;
            next.maxVal = 2.0f;
            r = next;
            _sm->normalizeDigitalOffOnlyRules();
            _sm->normalizeSchemeControlRules();
            _stor->saveSensors(*_sm);
            _om->beepAcceptedCommand();
            _log->add("Настройка управления: " + s->name + " -> " + _outputName(oi),
                      _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
            _sendOk(req);
            return;
        }

        if (doc.containsKey("logic")) {
            const String lg = doc["logic"].as<String>();
            if (lg == "heat")      next.logic = LOGIC_HEAT;
            else if (lg == "cool") next.logic = LOGIC_COOL;
            else {
                _sendError(req, 400, "bad_params", "logic must be 'heat' or 'cool'");
                return;
            }
        }
        if (doc.containsKey("min")) next.minVal = doc["min"].as<float>();
        if (doc.containsKey("max")) next.maxVal = doc["max"].as<float>();
        if (isnan(next.minVal) || isnan(next.maxVal) ||
            next.minVal + CTRL_MIN_DEADBAND > next.maxVal) {
            _sendError(req, 400, "bad_params", "min must be lower than max");
            return;
        }

        next.outIdx = oi;
        r = next;
        if (SensorManager::isMainOutputIndex((uint8_t)oi) &&
            SensorManager::isSchemeAnalogControlSensorIndex((uint8_t)si)) {
            _applyOutputLogicMode(oi, r.logic);
            _stor->saveOutputConfig(*_om);
        }
        _sm->normalizeDigitalOffOnlyRules();
        _sm->normalizeSchemeControlRules();
        _stor->saveSensors(*_sm);
        _om->beepAcceptedCommand();

        _log->add("Настройка управления: " + s->name + " -> " + _outputName(oi),
                  _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());
        _sendOk(req);
    }

    bool _requestWantsStopRelease(AsyncWebServerRequest* req) const {
        if (!req) return false;

        auto paramMeansTrue = [](String value) -> bool {
            value.trim();
            value.toLowerCase();
            return !(value == "0" || value == "false" || value == "off" || value == "no");
        };
        auto paramMeansFalse = [](String value) -> bool {
            value.trim();
            value.toLowerCase();
            return (value == "0" || value == "false" || value == "off" || value == "no" || value == "released");
        };

        if (req->hasParam("release")) {
            return paramMeansTrue(req->getParam("release")->value());
        }
        if (req->hasParam("action")) {
            String action = req->getParam("action")->value();
            action.trim();
            action.toLowerCase();
            return (action == "release" || action == "cancel" || action == "clear" || action == "off");
        }
        if (req->hasParam("state")) {
            return paramMeansFalse(req->getParam("state")->value());
        }
        if (req->hasParam("active")) {
            return paramMeansFalse(req->getParam("active")->value());
        }
        return false;
    }

    void _handleStopMainOutputs(AsyncWebServerRequest* req) {
        _om->setMainStopLatched(true);
        _stor->saveOutputs(*_om);

        _log->add("STOP активирован: все выходы выключены, логика CH1-CH3 пропущена до снятия STOP",
                  _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

        DynamicJsonDocument resp(448);
        resp["ok"] = true;
        resp["stopLatched"] = _om->mainStopLatched();
        resp["ch1"] = _om->out[OUT_CH1]->actualOn();
        resp["ch2"] = _om->out[OUT_CH2]->actualOn();
        resp["ch3"] = _om->out[OUT_CH3]->actualOn();
        resp["ch4"] = _om->out[OUT_CH4]->actualOn();
        resp["ch5"] = _om->out[OUT_CH5]->actualOn();
        _sendDoc(req, 200, resp);
    }

    void _handleReleaseStopMainOutputs(AsyncWebServerRequest* req) {
        _om->setMainStopLatched(false);
        _stor->saveOutputs(*_om);

        _log->add("STOP снят: логика CH1-CH3 будет заново рассчитана в следующем цикле",
                  _sm->getT1(), _sm->getT2(), _sm->getT3(), _sm->getDT());

        DynamicJsonDocument resp(384);
        resp["ok"] = true;
        resp["stopLatched"] = _om->mainStopLatched();
        resp["ch1"] = _om->out[OUT_CH1]->actualOn();
        resp["ch2"] = _om->out[OUT_CH2]->actualOn();
        resp["ch3"] = _om->out[OUT_CH3]->actualOn();
        _sendDoc(req, 200, resp);
    }

    void _handleOutputManual(AsyncWebServerRequest* req, int oi, uint8_t* data, size_t len) {
        _handleRelayCommand(req, oi, data, len, true);
    }

    bool _parseRelayCommandDoc(AsyncWebServerRequest* req, uint8_t* data, size_t len,
                               RelayCommand& cmdOut, bool& targetOnOut)
    {
        DynamicJsonDocument doc(160);
        if (!_parseJson(req, data, len, doc)) return false;

        if (doc.containsKey("cmd")) {
            String cmd = doc["cmd"] | "";
            cmd.trim();
            cmd.toLowerCase();
            if (cmd == "on") {
                cmdOut = CMD_ON;
                targetOnOut = true;
                return true;
            }
            if (cmd == "off") {
                cmdOut = CMD_OFF;
                targetOnOut = false;
                return true;
            }
            if (cmd == "none") {
                cmdOut = CMD_NONE;
                targetOnOut = false;
                return true;
            }
            _sendError(req, 400, "invalid_command", "invalid_command");
            return false;
        }

        if (doc.containsKey("state") || doc.containsKey("manual")) {
            targetOnOut = doc.containsKey("state") ? (bool)doc["state"] : (bool)doc["manual"];
            cmdOut = targetOnOut ? CMD_ON : CMD_OFF;
            return true;
        }

        _sendError(req, 400, "invalid_command", "invalid_command");
        return false;
    }

    void _handleRelayCommand(AsyncWebServerRequest* req, int oi, uint8_t* data,
                             size_t len, bool legacyManual)
    {
        if (oi < 0 || oi >= OUT_COUNT) {
            _sendError(req, 404, "not_found", "output not found");
            return;
        }

        RelayCommand cmd = CMD_NONE;
        bool targetOn = false;
        if (!_parseRelayCommandDoc(req, data, len, cmd, targetOn)) return;

        if (cmd == CMD_NONE) {
            DynamicJsonDocument resp(640);
            _buildRelayStateObject(resp.to<JsonObject>(), oi);
            resp["ok"] = true;
            resp["accepted"] = true;
            resp["reason"] = "";
            resp["detail"] = "";
            resp["cmd"] = relayCommandName(cmd);
            resp["target"] = _om->out[oi]->requestedOn();
            _sendDoc(req, 200, resp);
            return;
        }

        RelayCommandResult r = _om->handleRelayCommand((uint8_t)oi, cmd, _log, _sm);
        if (r.accepted) {
            _stor->saveOutputs(*_om);
            _om->beepAcceptedCommand();
        }

        DynamicJsonDocument resp(640);
        _buildRelayStateObject(resp.to<JsonObject>(), oi);
        resp["ok"] = legacyManual ? r.accepted : true;
        resp["accepted"] = r.accepted;
        resp["reason"] = r.reason;
        resp["detail"] = r.detail;
        resp["detailText"] = _om->relayBlockDetailText((uint8_t)oi, r.detail);
        resp["cmd"] = relayCommandName(cmd);
        resp["target"] = targetOn;
        if (!r.accepted) resp["userMessage"] = _relayCommandUserMessage(oi, r.detail);

        if (legacyManual && !r.accepted && cmd == CMD_ON) {
            _sendDoc(req, 409, resp);
            return;
        }
        _sendDoc(req, (legacyManual || r.accepted || strcmp(r.reason, "invalid_command") != 0) ? 200 : 400, resp);
    }

    void _handleRelayState(AsyncWebServerRequest* req, int oi) {
        if (oi < 0 || oi >= OUT_COUNT) {
            _sendError(req, 404, "not_found", "output not found");
            return;
        }
        DynamicJsonDocument doc(512);
        JsonObject root = doc.to<JsonObject>();
        _buildRelayStateObject(root, oi);
        _sendDoc(req, 200, doc);
    }

    void _buildRelayStateObject(JsonObject root, int oi) {
        Output* o = _om->out[oi];
        const bool actualOn = o->actualOn();
        const bool hasConfirm = (oi >= 0 && oi < 4 && _cm->get(oi).available);
        const bool feedbackOn = hasConfirm ? _cm->get(oi).actual : actualOn;

        root["id"] = _outputName(oi);
        root["confirmed"] = feedbackOn ? 1 : 0;
        root["confirmedBool"] = feedbackOn;
        root["feedbackOn"] = feedbackOn;
        root["confirmActual"] = feedbackOn;
        root["confirmAvailable"] = hasConfirm;
        root["actual"] = actualOn;
        root["displayOn"] = actualOn;
        root["requested"] = o->requestedOn();
        root["manualWant"] = o->manualWant();
        root["operatorHoldOff"] = _om->operatorHoldOff((uint8_t)oi);
        root["forbidden"] = o->forbidden();
        root["forbidMask"] = o->forbidMask();
        root["effectiveForbidMask"] = _om->effectiveForbidMask((uint8_t)oi);
        root["forbidReasonText"] = _om->formatForbidReasons(o->forbidMask());
        JsonArray reasons = root.createNestedArray("forbidReasons");
        _appendForbidReasons(reasons, o->forbidMask());
        root["wantOnMask"] = o->wantOnMask();
        root["stopLatched"] = _om->mainStopLatched();
        root["pending"] = _om->relayCommandPending((uint8_t)oi);
        root["relayPending"] = _om->relayCommandPending((uint8_t)oi);
        root["pendingCmd"] = _om->relayCommandPending((uint8_t)oi)
            ? relayCommandName(_om->relayCommand((uint8_t)oi))
            : nullptr;

        const char* err = _om->relayCommandErrorName((uint8_t)oi);
        if (err && err[0]) {
            root["relayError"] = err;
            root["relayErrorMs"] = _om->relayCommandErrorMs((uint8_t)oi);
            root["relayErrorText"] = _om->relayErrorText((uint8_t)oi);
        }
        if (hasConfirm) {
            const ConfirmationChannel& c = _cm->get(oi);
            root["confirmMismatch"] = c.mismatch;
            root["confirmTimeout"] = c.timeout;
            root["confirmFault"] = ConfirmationManager::faultName(c.fault);
            root["confirmFaultText"] = ConfirmationManager::faultNameRu(c.fault);
            root["confirmFaultLatched"] = c.faultLatched;
            root["confirmEmuMode"] = ConfirmationManager::emuModeName(_cm->emuMode((uint8_t)oi));
            root["confirmEmuModeText"] = ConfirmationManager::emuModeNameRu(_cm->emuMode((uint8_t)oi));
        }
    }

    String _relayCommandUserMessage(int oi, const char* detail) const {
        if (!detail || !detail[0]) return "";
        const String prefix = String("Команда ") + _outputName(oi) + ": ";
        if (strcmp(detail, "stop_active") == 0) {
            return prefix + "включение запрещено, активен STOP. Сначала снимите STOP.";
        }
        if (strcmp(detail, "forbidden") == 0) {
            const String reasons = _om->formatForbidReasons(_om->effectiveForbidMask((uint8_t)oi));
            if (reasons.length() > 0) {
                return prefix + "включение запрещено текущими условиями автоматики: " + reasons + ".";
            }
            return prefix + "включение запрещено текущими условиями автоматики.";
        }
        if (strcmp(detail, "disabled") == 0) return prefix + "выход отключён в конфигурации.";
        if (strcmp(detail, "duplicate") == 0) return prefix + "такая же команда уже выполняется.";
        if (strcmp(detail, "busy") == 0) return prefix + "предыдущая команда ещё ожидает подтверждения.";
        if (strcmp(detail, "auto_on_active") == 0) return prefix + "выключение запрещено, автоматика требует держать канал включённым.";
        if (strcmp(detail, "invalid_command") == 0) return prefix + "некорректная команда.";
        return prefix + _om->relayBlockDetailText((uint8_t)oi, detail);
    }

    // ------------------------------------------------------------
    // JSON builders
    // ------------------------------------------------------------
    void _buildInfo(JsonObject root) {
        root["name"]       = DEVICE_NAME;
        root["fw"]         = FW_VERSION;
        root["apiVersion"] = API_VERSION;
        root["emu"]        = EMU_MODE;
        root["synced"]     = _tb->isSynced();
        root["time"]       = _tb->nowStr();

        root["apIP"]       = _wifi->apIP();
        root["apRunning"]  = _wifi->apRunning();
        root["apStatusText"] = _wifi->apStatusText();
        root["staIP"]      = _wifi->staIP();
        root["rssi"]       = _wifi->rssi();
        root["staRssi"]    = _wifi->staRssi();
        root["apRssi"]     = _wifi->apRssi();
        root["apClientCount"] = _wifi->apClientCount();
        root["muted"]      = _om->soundMuted;
        root["ch4Enabled"] = _om->ch4Enabled;
        root["ch5Enabled"] = _om->ch5Enabled;
        root["stopLatched"] = _om->mainStopLatched();
        root["wifiWizardPending"] = _wifiWizardPending();
        root["notifyEnabled"] = (_notifier ? _notifier->enabled() : false);

        JsonObject wizard = root.createNestedObject("wifiWizard");
        wizard["pending"]     = _wifiWizardPending();
        wizard["servicePage"] = "/wifi";
        wizard["staConfigured"] = _wifi->staConfigured();
        wizard["apProtected"] = _wifi->apProtected();

        JsonObject ap = root.createNestedObject("ap");
        ap["ssid"]      = _wifi->apSSID;
        ap["ip"]        = _wifi->apIP();
        ap["protected"] = _wifi->apProtected();

        JsonObject sta = root.createNestedObject("sta");
        sta["configured"] = _wifi->staConfigured();
        sta["connected"]  = _wifi->staConnected;
        sta["ssid"]       = _wifi->staSSID;
        sta["ip"]         = _wifi->staIP();
        sta["rssi"]       = _wifi->staRssi();
        sta["statusText"] = _wifi->staStatusText();

        ap["clientCount"] = _wifi->apClientCount();
        ap["rssi"]        = _wifi->apRssi();

        JsonObject notify = root.createNestedObject("notify");
        notify["enabled"] = (_notifier ? _notifier->enabled() : false);
        notify["url"] = (_notifier ? _notifier->publishUrl() : String(""));
        notify["hasToken"] = (_notifier ? _notifier->hasToken() : false);

        JsonArray pages = root.createNestedArray("servicePages");
        pages.add("/");
        pages.add("/wifi");

        JsonArray operatorPages = root.createNestedArray("operatorPages");
        operatorPages.add("/app");

        JsonArray endpoints = root.createNestedArray("endpoints");
        endpoints.add("/api/v1/info");
        endpoints.add("/api/v1/version");
        endpoints.add("/api/v1/health");
        endpoints.add("/api/v1/schema");
        endpoints.add("/api/v1/diag");
        endpoints.add("/api/v1/state");
        endpoints.add("/api/v1/sensor");
        endpoints.add("/api/v1/output");
        endpoints.add("/api/v1/output/config");
        endpoints.add("/api/v1/log");
        endpoints.add("/api/v1/time/sync");
        endpoints.add("/api/v1/stop");
        endpoints.add("/api/v1/stop?release=1");
        endpoints.add("/api/v1/stop/");
        endpoints.add("/api/v1/stop/release");
        endpoints.add("/api/v1/stop/release/");
        endpoints.add("/api/v1/mute");
        endpoints.add("/api/v1/ack");
        endpoints.add("/api/v1/ack/");
        endpoints.add("/api/v1/safety/reset");
        endpoints.add("/api/v1/notify/config");
        endpoints.add("/api/v1/notify/test");
        endpoints.add("/api/v1/wifi/scan");
        endpoints.add("/api/v1/wifi/connect");
        endpoints.add("/api/v1/wifi/ap");
        endpoints.add("/api/v1/wifi/wizard/complete");
        endpoints.add("/api/v1/emu/set");
        endpoints.add("/api/v1/emu/scenario");
        endpoints.add("/app");
        endpoints.add("/app.css");
        endpoints.add("/app.js");

        JsonObject diag = root.createNestedObject("diag");
        _buildDiag(diag);
    }

    void _buildState(JsonObject root) {
        root["time"]   = _tb->nowStr();
        root["timeMs"] = _tb->nowMs();
        root["synced"] = _tb->isSynced();
        root["emu"]    = EMU_MODE;
        root["fw"]     = FW_VERSION;
        root["apiVersion"] = API_VERSION;
        root["apIP"]   = _wifi->apIP();
        root["apRunning"] = _wifi->apRunning();
        root["staIP"]  = _wifi->staIP();
        root["rssi"]   = _wifi->rssi();
        root["staRssi"] = _wifi->staRssi();
        root["apRssi"] = _wifi->apRssi();
        root["apClientCount"] = _wifi->apClientCount();
        root["muted"]  = _om->soundMuted;
        root["activeAlarmCount"] = _om->activeAlarmCount(*_sm);
        root["unackedAlarmCount"] = _om->unackedAlarmCount(*_sm);
        root["safetyAlarmActive"] = _om->safetyAlarmActive();
        root["ch4Enabled"] = _om->ch4Enabled;
        root["ch5Enabled"] = _om->ch5Enabled;
        root["stopLatched"] = _om->mainStopLatched();
        root["wifiWizardPending"] = _wifiWizardPending();
        root["notifyEnabled"] = (_notifier ? _notifier->enabled() : false);

        JsonArray sensors = root.createNestedArray("sensors");
        _buildSensors(sensors);

        JsonArray outputs = root.createNestedArray("outputs");
        _buildOutputs(outputs);

        JsonArray confirmations = root.createNestedArray("confirmations");
        _buildConfirmations(confirmations);

        JsonObject diag = root.createNestedObject("diag");
        _buildDiag(diag);
    }

    void _buildSensors(JsonArray arr) {
        for (int i = 0; i < SEN_COUNT; i++) {
            SensorBase* s = _sm->s[i];
            JsonObject so = arr.createNestedObject();
            so["id"]      = SensorManager::sensorName(i);
            so["enabled"] = s->enabled;
            so["periodMs"] = s->periodMs;
            so["error"]   = s->error;
            so["present"] = s->present;
            so["stale"]   = s->isStale();
            so["lastValidMs"] = s->lastValidMs;
            so["unit"]    = SensorManager::sensorUnit(i);
            so["hwLimited"] = s->hwLimited;
            so["alarmDelayMs"] = s->alarmDelayMs;
            so["ctrlDelayMs"]  = s->ctrlDelayMs;
            if (s->diagCode != SENSOR_DIAG_NONE) so["note"] = s->diagText();

            if (!isnan(s->value) && !s->error) so["value"] = roundf(s->value * 100.0f) / 100.0f;
            else                               so["value"] = nullptr;

            const uint8_t unackedMask = _om->unackedAlarmMaskFor(*_sm, i);
            JsonArray alarms = so.createNestedArray("alarms");
            for (int ai = 0; ai < N_ALARMS; ai++) {
                JsonObject ao = alarms.createNestedObject();
                ao["enabled"]   = s->alarm[ai].enabled;
                ao["threshold"] = s->alarm[ai].threshold;
                ao["isMax"]     = s->alarm[ai].isMax;
                ao["triggered"] = s->alarm[ai].triggered;
                ao["unacked"]   = ((unackedMask & (1u << ai)) != 0);
            }

            JsonArray ctrl = so.createNestedArray("ctrl");
            for (int oi = 0; oi < N_CTRL_OUT; oi++) {
                JsonObject co = ctrl.createNestedObject();
                const bool fixedOffOnly = SensorManager::isDigitalOffOnlyRule((uint8_t)i, (uint8_t)oi);
                co["outIdx"]  = s->ctrl[oi].outIdx;
                co["enabled"] = s->ctrl[oi].enabled;
                co["logic"]   = (s->ctrl[oi].logic == LOGIC_COOL) ? "cool" : "heat";
                co["min"]     = s->ctrl[oi].minVal;
                co["max"]     = s->ctrl[oi].maxVal;
                co["fixedOffOnly"] = fixedOffOnly;
                co["logicLocked"]  = fixedOffOnly;
                co["schemeAllowed"] = SensorManager::isRuleAllowedForOutput((uint8_t)i, (uint8_t)oi);
            }
        }
    }

    void _buildOutputs(JsonArray arr) {
        for (int i = 0; i < OUT_COUNT; i++) {
            Output* o = _om->out[i];
            JsonObject oo = arr.createNestedObject();
            const bool actualOn = o->actualOn();

            oo["id"]         = _outputName(i);
            oo["state"]      = actualOn;
            oo["actual"]     = actualOn;
            oo["displayOn"]  = actualOn;
            oo["requested"]  = o->requestedOn();
            oo["manualWant"] = o->manualWant();
            oo["operatorHoldOff"] = _om->operatorHoldOff((uint8_t)i);
            oo["forbidden"]  = o->forbidden();
            oo["forbidMask"] = o->forbidMask();
            oo["sensorForbidMask"] = _om->lastForbidMask((uint8_t)i);
            oo["safetyForbidMask"] = _om->safetyForbidMask((uint8_t)i);
            oo["effectiveForbidMask"] = _om->effectiveForbidMask((uint8_t)i);
            oo["forbidReasonText"] = _om->formatForbidReasons(o->forbidMask());
            oo["wantOnMask"] = o->wantOnMask();
            JsonArray reasons = oo.createNestedArray("forbidReasons");
            _appendForbidReasons(reasons, o->forbidMask());
            oo["enabled"]    = o->enabled;
            if (i < 3) oo["mode"] = (_om->chMode[i] == LOGIC_COOL) ? "cool" : "heat";

            oo["relayPending"] = _om->relayCommandPending((uint8_t)i);
            oo["pendingCmd"] = _om->relayCommandPending((uint8_t)i)
                ? relayCommandName(_om->relayCommand((uint8_t)i))
                : nullptr;
            const char* relayErr = _om->relayCommandErrorName((uint8_t)i);
            if (relayErr && relayErr[0]) {
                oo["relayError"] = relayErr;
                oo["relayErrorMs"] = _om->relayCommandErrorMs((uint8_t)i);
                oo["relayErrorText"] = _om->relayErrorText((uint8_t)i);
            }

            if (i < 4) {
                const ConfirmationChannel& c = _cm->get(i);
                oo["confirmAvailable"] = c.available;
                if (c.available) {
                    oo["confirmed"] = c.confirmed;
                    oo["mismatch"]  = c.mismatch;
                    oo["timeout"]   = c.timeout;
                    oo["pending"]   = c.pending;
                    oo["confirmActual"]   = c.actual;
                    oo["confirmExpected"] = c.expected;
                    oo["feedbackOn"] = c.actual;
                    oo["confirmedLevel"] = c.actual ? 1 : 0;
                } else {
                    oo["confirmed"] = nullptr;
                    oo["mismatch"]  = false;
                    oo["timeout"]   = false;
                    oo["pending"]   = false;
                    oo["feedbackOn"] = actualOn;
                    oo["confirmedLevel"] = actualOn ? 1 : 0;
                    oo["confirmNote"] = c.note;
                }
            }
        }
    }

    void _buildConfirmations(JsonArray arr) {
        for (int i = 0; i < 4; i++) {
            const ConfirmationChannel& c = _cm->get(i);
            JsonObject co = arr.createNestedObject();

            co["id"]         = c.id;
            co["outputId"]   = c.outputId;
            co["available"]  = c.available;
            co["raw"]        = c.raw;
            co["actual"]     = c.actual;
            co["expected"]   = c.expected;
            co["confirmed"]  = c.confirmed;
            co["pending"]    = c.pending;
            co["mismatch"]   = c.mismatch;
            co["timeout"]    = c.timeout;
            co["faultLatched"] = c.faultLatched;
            co["fault"]      = ConfirmationManager::faultName(c.fault);
            co["faultText"]  = ConfirmationManager::faultNameRu(c.fault);
            co["timeoutMs"]  = c.timeoutMs;
            co["debounceMs"] = c.debounceMs;
            co["emuMode"]    = ConfirmationManager::emuModeName(_cm->emuMode((uint8_t)i));
            co["emuModeText"] = ConfirmationManager::emuModeNameRu(_cm->emuMode((uint8_t)i));
            if (strlen(c.note) > 0) co["note"] = c.note;
        }
    }

    void _buildDiag(JsonObject root) {
        const bool adc2Warning = (PIN_C == 27);
        root["pin35Mode"]   = (GPIO35_MODE == GPIO35_MODE_WER_CH2) ? "WER_CH2" : "V_SENSOR";
        root["adc2Warning"] = adc2Warning;
        root["staConnected"] = _wifi->staConnected;
        root["staConfigured"] = _wifi->staConfigured();
        root["wifiQualityWarning"] = (_wifi->staConnected && _wifi->rssi() <= WIFI_RSSI_WARN_DBM);
        root["apRunning"] = _wifi->apRunning();
        root["apStatusText"] = _wifi->apStatusText();
        root["apClientCount"] = _wifi->apClientCount();
        root["apRssi"] = _wifi->apRssi();
        root["storageReady"] = (_stor ? _stor->ready() : false);
        root["storageRecovered"] = (_stor ? _stor->recovered() : false);
        root["storageStatus"] = (_stor ? _stor->statusText() : String("недоступно"));
        root["stopLatched"] = _om->mainStopLatched();

        JsonArray hw = root.createNestedArray("hardwareLimitations");
        if (adc2Warning) {
            hw.add("GPIO27/ADC2 (датчик C) ненадёжен при активном WiFi в реальном режиме");
        }
        hw.add("GPIO35 общий для V и WER_CH2; в реальном режиме работает только то, что выбрано через GPIO35_MODE");
        hw.add("WER_CH1 (GPIO27) использует внутренний pull-down; для WER_CH2/CH3/CH4 (GPIO35/34/36) нужна внешняя подтяжка к GND");

        JsonArray mism = root.createNestedArray("activeConfirmMismatches");
        for (int i = 0; i < 4; i++) {
            const ConfirmationChannel& c = _cm->get(i);
            if (c.mismatch) mism.add(c.id);
        }
    }

    // ------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------
    void _appendForbidReasons(JsonArray arr, uint32_t mask) {
        for (uint8_t si = 0; si < SEN_COUNT; si++) {
            if (mask & (1u << si)) arr.add(SensorManager::sensorName(si));
        }
        if (mask & (1u << RULEIDX_STOP)) arr.add("STOP");
        if (mask & (1u << RULEIDX_SAFETY_LEVEL)) arr.add("SAFETY_LEVEL");
        if (mask & (1u << RULEIDX_SAFETY_FLOW)) arr.add("SAFETY_FLOW");
        if (mask & (1u << RULEIDX_SAFETY_PRESSURE)) arr.add("SAFETY_PRESSURE");
        if (mask & (1u << RULEIDX_SAFETY_WER)) arr.add("SAFETY_WER");
    }

    uint8_t _parseEmuConfirmMode(const JsonVariantConst& v) const {
        if (v.is<bool>()) {
            return v.as<bool>() ? EMU_CONFIRM_FORCE_ON : EMU_CONFIRM_FORCE_OFF;
        }
        String mode = v.as<String>();
        mode.trim();
        mode.toLowerCase();
        if (mode == "on" || mode == "force_on" || mode == "high" || mode == "1") {
            return EMU_CONFIRM_FORCE_ON;
        }
        if (mode == "off" || mode == "force_off" || mode == "low" || mode == "0") {
            return EMU_CONFIRM_FORCE_OFF;
        }
        return EMU_CONFIRM_AUTO;
    }

    void _applyOutputLogicMode(int outIdx, uint8_t logic) {
        if (!_om || !_sm) return;
        if (!SensorManager::isMainOutputIndex((uint8_t)outIdx)) return;
        _om->chMode[outIdx] = logic;
        for (int si = 0; si < SEN_COUNT; si++) {
            if (!SensorManager::isSchemeAnalogControlSensorIndex((uint8_t)si)) continue;
            SensorBase* s = _sm->s[si];
            if (!s) continue;
            for (int ri = 0; ri < N_CTRL_OUT; ri++) {
                if (s->ctrl[ri].outIdx == outIdx) s->ctrl[ri].logic = logic;
            }
        }
        _sm->normalizeDigitalOffOnlyRules();
    }

    bool _parseJson(AsyncWebServerRequest* req, uint8_t* data, size_t len, DynamicJsonDocument& doc) {
        if (len == 0) {
            _sendError(req, 400, "invalid_json", "Пустое тело запроса");
            return false;
        }
        DeserializationError err = deserializeJson(doc, data, len);
        if (err) {
            _sendError(req, 400, "invalid_json", "Некорректный JSON");
            return false;
        }
        return true;
    }

    void _sendGzip(AsyncWebServerRequest* req, const char* contentType, const uint8_t* data, size_t len,
                   const char* cacheControl = "no-cache, no-store, must-revalidate") {
        AsyncWebServerResponse* resp = req->beginResponse_P(200, contentType, data, len);
        resp->addHeader("Content-Encoding", "gzip");
        resp->addHeader("Cache-Control", cacheControl);
        if (strstr(cacheControl, "no-store") != nullptr) {
            resp->addHeader("Pragma", "no-cache");
            resp->addHeader("Expires", "0");
        }
        req->send(resp);
    }

    void _sendOk(AsyncWebServerRequest* req) {
        AsyncWebServerResponse* resp = req->beginResponse(200, "application/json; charset=utf-8", "{\"ok\":true}");
        resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        resp->addHeader("Pragma", "no-cache");
        resp->addHeader("Expires", "0");
        req->send(resp);
    }

    void _sendError(AsyncWebServerRequest* req, int status, const String& code, const String& message) {
        DynamicJsonDocument doc(256);
        doc["ok"]    = false;
        doc["code"]  = status;
        doc["error"] = message;
        doc["err"]   = message;
        doc["type"]  = code;
        _sendDoc(req, status, doc);
    }

    void _sendDoc(AsyncWebServerRequest* req, int status, DynamicJsonDocument& doc) {
        String json;
        serializeJson(doc, json);
        AsyncWebServerResponse* resp = req->beginResponse(status, "application/json; charset=utf-8", json);
        resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        resp->addHeader("Pragma", "no-cache");
        resp->addHeader("Expires", "0");
        req->send(resp);
    }

    static const char* _outputName(int idx) {
        static const char* names[OUT_COUNT] = {"CH1","CH2","CH3","CH4","CH5"};
        return (idx >= 0 && idx < OUT_COUNT) ? names[idx] : "";
    }

    static String _fmtVal(float v) {
        if (isnan(v)) return "null";
        return String(v, 2);
    }

    static String _jsonEscape(const String& s) {
        String out;
        out.reserve(s.length() + 8);
        for (size_t i = 0; i < s.length(); i++) {
            const char c = s[i];
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '\"': out += "\\\""; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:   out += c; break;
            }
        }
        return out;
    }

    bool _wifiWizardPending() const {
        return !_stor || !_stor->loadWifiWizardDone();
    }
};
