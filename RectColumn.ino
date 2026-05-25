// ================================================================
// RectColumn.ino – Контроллер ESP32 для колонны
// ================================================================

#include <Arduino.h>
#include <esp_core_dump.h>
#include <esp_system.h>
#include "config.h"
#include "TimeBase.h"
#include "EventLog.h"
#include "Sensors.h"
#include "SensorManager.h"
#include "Output.h"
#include "OutputManager.h"
#include "ConfirmationManager.h"
#include "Storage.h"
#include "WiFiMgr.h"
#include "WebAPI.h"
#include "Emulator.h"
#include "SerialDebugReporter.h"
#include "ProcessSafety.h"
#include "RemoteNotifier.h"
#include "AckButton.h"

// ── Глобальные объекты ───────────────────────────────────────────
TimeBase            timeBase;
EventLog            eventLog;
SensorManager       sensorMgr;
OutputManager       outputMgr;
ConfirmationManager confirmMgr;
Storage             storage;
WiFiMgr             wifiMgr;
WebAPI              webAPI(80);
Emulator            emulator;
SerialDebugReporter debugReporter;
ProcessSafety       processSafety;
RemoteNotifier      remoteNotifier;
AckButton           ackButton;

// ── Отслеживание состояния для журнала событий ──────────────────
bool    prevOutState[OUT_COUNT]    = {};
bool    prevSenError[SEN_COUNT]    = {};
bool    prevSenPresent[SEN_COUNT]  = {};
bool    prevSenLatched[SEN_COUNT]  = {};
uint8_t prevAlarmMask[SEN_COUNT]   = {};
uint32_t lastHeartbeatMs = 0;
uint32_t lastWifiLedBlinkMs = 0;
bool wifiLedBlinkState = false;

static const char* resetReasonText(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_UNKNOWN:   return "неизвестно (UNKNOWN)";
        case ESP_RST_POWERON:   return "включение питания (POWERON)";
        case ESP_RST_EXT:       return "внешний сброс (EXT)";
        case ESP_RST_SW:        return "программный сброс (SW)";
        case ESP_RST_PANIC:     return "паника ядра (PANIC)";
        case ESP_RST_INT_WDT:   return "сброс по внутреннему watchdog (INT_WDT)";
        case ESP_RST_TASK_WDT:  return "сброс по task watchdog (TASK_WDT)";
        case ESP_RST_WDT:       return "сброс по watchdog (WDT)";
        case ESP_RST_DEEPSLEEP: return "выход из deep sleep (DEEPSLEEP)";
        case ESP_RST_BROWNOUT:  return "просадка питания (BROWNOUT)";
        case ESP_RST_SDIO:      return "сброс SDIO";
        default:                return "прочая причина (OTHER)";
    }
}

static void printBootResetDiagnostics(esp_reset_reason_t reason) {
    Serial.printf("[BOOT] reset reason = %d (%s)\n", (int)reason, resetReasonText(reason));

#if !CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH
    Serial.println("[BOOT] TODO: текущий профиль Arduino-ESP32 собран без flash coredump");
#endif

    if (reason == ESP_RST_PANIC || reason == ESP_RST_INT_WDT || reason == ESP_RST_TASK_WDT) {
        size_t addr = 0;
        size_t size = 0;
        if (esp_core_dump_image_get(&addr, &size) == ESP_OK) {
            Serial.printf("[BOOT] coredump available: addr=0x%x size=%u\n",
                          (unsigned)addr,
                          (unsigned)size);
            Serial.println("[BOOT] use 'espcoredump.py info_corefile' to decode");
        } else {
            Serial.println("[BOOT] coredump not found in flash");
        }
    }
}

static void syncCtrlLogicFromOutputModes() {
    sensorMgr.normalizeDigitalOffOnlyRules();
    for (int si = 0; si < SEN_COUNT; si++) {
        if (!SensorManager::isSchemeAnalogControlSensorIndex((uint8_t)si)) continue;
        SensorBase* s = sensorMgr.s[si];
        if (!s) continue;
        for (int ri = 0; ri < N_CTRL_OUT; ri++) {
            const uint8_t outIdx = s->ctrl[ri].outIdx;
            if (SensorManager::isMainOutputIndex(outIdx)) {
                s->ctrl[ri].logic = outputMgr.chMode[outIdx];
            }
        }
    }
    sensorMgr.normalizeDigitalOffOnlyRules();
    sensorMgr.normalizeSchemeControlRules();
}

static void initPrevState() {
    for (int i = 0; i < OUT_COUNT; i++) prevOutState[i] = outputMgr.out[i]->isOn();
    for (int i = 0; i < SEN_COUNT; i++) {
        prevSenError[i]   = sensorMgr.s[i]->error;
        prevSenPresent[i] = sensorMgr.s[i]->present;
        prevSenLatched[i] = sensorMgr.s[i]->sensorErrorLatched;
        prevAlarmMask[i]  = sensorMgr.s[i]->userAlarmMask();
    }
}

static void logSensorTransitions() {
    for (int i = 0; i < SEN_COUNT; i++) {
        SensorBase* s = sensorMgr.s[i];

        if (s->tracksSensorLoss()) {
            prevSenPresent[i] = s->present;
            prevSenError[i] = s->error;
            if (s->sensorErrorLatched != prevSenLatched[i]) {
                prevSenLatched[i] = s->sensorErrorLatched;
                if (s->sensorErrorLatched) {
                    eventLog.add(s->sensorLostNotice(),
                                 sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
                }
            }
        } else {
            if (s->present != prevSenPresent[i]) {
                prevSenPresent[i] = s->present;
                eventLog.add(s->name + (s->present ? " подключён" : " отключён"),
                             sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
            }

            if (s->error != prevSenError[i]) {
                prevSenError[i] = s->error;
                eventLog.add(s->name + String(s->error ? " ошибка" : " ошибка снята"),
                             sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
            }
        }

        const uint8_t curMask = s->userAlarmMask();
        if (curMask != prevAlarmMask[i]) {
            if (prevAlarmMask[i] == 0 && curMask != 0) {
                eventLog.add(s->name + " тревога сработала",
                             sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
            } else if (prevAlarmMask[i] != 0 && curMask == 0) {
                eventLog.add(s->name + " тревога снята",
                             sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
            } else {
                eventLog.add(s->name + " состояние тревог изменилось",
                             sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
            }
            prevAlarmMask[i] = curMask;
        }
    }
}

static void logOutputTransitions() {
    for (int i = 0; i < OUT_COUNT; i++) {
        const bool cur = outputMgr.out[i]->isOn();
        if (cur != prevOutState[i]) {
            prevOutState[i] = cur;
            if (i == OUT_CH4 || i == OUT_CH5) continue; // звуковое оформление не засоряет лог
            eventLog.add(String(outputMgr.out[i]->name) + (cur ? " включён" : " выключен"),
                         sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
        }
    }
}

static void printHeartbeat() {
    const uint32_t now = millis();
    if (now - lastHeartbeatMs < 30000UL) return;
    lastHeartbeatMs = now;

    Serial.print("[HB] up=");
    Serial.print(now);
    Serial.print(" freeHeap=");
    Serial.print(ESP.getFreeHeap());
    Serial.print(" minFreeHeap=");
    Serial.print(ESP.getMinFreeHeap());
    Serial.print(" sta=");
    Serial.print(wifiMgr.staConnected ? "ON" : "OFF");
    Serial.print(" staIP=");
    Serial.print(wifiMgr.staIP());
    Serial.print(" apIP=");
    Serial.print(wifiMgr.apIP());
    Serial.print(" rssi=");
    Serial.print(wifiMgr.rssi());
    Serial.print(" synced=");
    Serial.println(timeBase.isSynced() ? "YES" : "NO");
}

static void initWiFiLed() {
    if (PIN_WIFI_LED < 0) return;
    pinMode(PIN_WIFI_LED, OUTPUT);
    digitalWrite(PIN_WIFI_LED, LOW);
}

static void updateWiFiLed() {
    if (PIN_WIFI_LED < 0) return;
    if (wifiMgr.staConnected) {
        digitalWrite(PIN_WIFI_LED, HIGH);
        return;
    }

    const wifi_mode_t mode = WiFi.getMode();
    const bool apActive = (mode == WIFI_AP || mode == WIFI_AP_STA);
    if (!apActive) {
        digitalWrite(PIN_WIFI_LED, LOW);
        return;
    }

    const uint32_t now = millis();
    if (now - lastWifiLedBlinkMs >= WIFI_LED_BLINK_MS) {
        lastWifiLedBlinkMs = now;
        wifiLedBlinkState = !wifiLedBlinkState;
        digitalWrite(PIN_WIFI_LED, wifiLedBlinkState ? HIGH : LOW);
    }
}

void setup() {
    Serial.begin(115200);
    delay(300);
    const esp_reset_reason_t resetReason = esp_reset_reason();
    printBootResetDiagnostics(resetReason);
    Serial.println();
    Serial.println("=== " DEVICE_NAME " " FW_VERSION " ===");
    Serial.println(String("Причина сброса: ") + resetReasonText(resetReason));

    eventLog.begin(&timeBase);
    eventLog.add("Версия прошивки " FW_VERSION);
    eventLog.add(String("Загрузка: причина сброса ") + resetReasonText(resetReason));

    storage.loadSensors(sensorMgr);
    storage.loadOutputs(outputMgr);
    syncCtrlLogicFromOutputModes();

    if (!storage.ready()) {
        eventLog.add(String("Хранилище: недоступно - ") + storage.statusText());
    } else if (storage.recovered()) {
        eventLog.add("Хранилище: NVS восстановлено, сохранённые настройки сброшены");
    }

    if (!EMU_MODE) {
        sensorMgr.begin();
    }

    outputMgr.begin();
    confirmMgr.begin(outputMgr);
    emulator.begin();
    initWiFiLed();

    wifiMgr.begin(storage, eventLog);

    webAPI.begin(timeBase, eventLog, sensorMgr, outputMgr, confirmMgr, wifiMgr, storage, emulator, remoteNotifier, processSafety);
    debugReporter.begin();
    processSafety.begin(timeBase, eventLog, sensorMgr, outputMgr, confirmMgr);
    remoteNotifier.begin(wifiMgr, eventLog, sensorMgr, outputMgr, storage);
    ackButton.begin();

    initPrevState();

    eventLog.add("Веб-сервер запущен", sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());
    eventLog.add(String("Режим прошивки: ") + (EMU_MODE ? "ЭМУЛЯЦИЯ" : "РЕАЛЬНЫЙ"),
                 sensorMgr.getT1(), sensorMgr.getT2(), sensorMgr.getT3(), sensorMgr.getDT());

    Serial.println("AP IP   : " + wifiMgr.apIP());
    Serial.println(String("AP mode : ") + String(wifiMgr.apRunning() ? "ON" : "FAILED") + " / " + wifiMgr.apStatusText());
    Serial.println(String("STA cfg : ") + String(wifiMgr.staConfigured() ? wifiMgr.staSSID : "<none>"));
    Serial.println(String("Storage : ") + storage.statusText());
    Serial.println("Готово.");
}

void loop() {
    if (EMU_MODE) {
        emulator.injectAll(sensorMgr);
    }

    sensorMgr.loop();
    logSensorTransitions();

    outputMgr.loop(sensorMgr, &eventLog);
    logOutputTransitions();

    // Опрос WER-подтверждений должен работать всегда, чтобы API/UI видели
    // реальное состояние реле даже при активном STOP.
    confirmMgr.loop(outputMgr, sensorMgr, &eventLog);

    if (!outputMgr.mainStopLatched()) {
        bool relayFeedbackOn[OUT_COUNT] = {};
        bool relayFeedbackAvailable[OUT_COUNT] = {};
        for (uint8_t i = 0; i < 4; i++) {
            const ConfirmationChannel& c = confirmMgr.get(i);
            if (c.outputIdx < OUT_COUNT && requiresWerConfirmation(c.outputIdx)) {
                relayFeedbackAvailable[c.outputIdx] = c.available;
                relayFeedbackOn[c.outputIdx] = c.actual;
            }
        }
        outputMgr.updateRelayCommandFeedback(relayFeedbackOn, relayFeedbackAvailable,
                                             &eventLog, &sensorMgr);

        processSafety.loop();
        outputMgr.setSafetyAlarmActive(processSafety.safetyAlarmActive());
    } else {
        outputMgr.setSafetyAlarmActive(false);
    }

    ackButton.loop(outputMgr, sensorMgr, &eventLog);
    remoteNotifier.loop();

    if (outputMgr.consumeManualStateDirty()) {
        storage.saveOutputs(outputMgr);
    }

    wifiMgr.loop();
    updateWiFiLed();
    debugReporter.loop(timeBase, sensorMgr, confirmMgr);
    printHeartbeat();

    yield();
}
