#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\EventLog.h"
// ================================================================
// EventLog.h  
// ================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "TimeBase.h"

struct LogEntry {
    uint64_t absMs;
    String   timeStr;
    String   event;
    float    T1, T2, T3, dT;
};

class EventLog {
public:
    EventLog() : _tb(nullptr), _head(0), _count(0) {}

    void begin(TimeBase* tb) { _tb = tb; }

    void add(const String& event,
             float t1 = NAN, float t2 = NAN,
             float t3 = NAN, float dt = NAN)
    {
        const uint64_t absMs = (uint64_t)millis();
        const String timeStr = _tb ? _tb->millisToStr(absMs) : (String("T+") + (absMs / 1000) + "s");
        int traceIdx = 0;

#if STABILITY_BOOT_DIAG
        const bool traceEvent =
            event.startsWith("RELAY_OFF") ||
            event.startsWith("Notify failed") ||
            event.startsWith("Настройка датчика:") ||
            event.indexOf("тревога") >= 0 ||
            event.indexOf("Авария") >= 0;
        const unsigned traceLen = (unsigned)event.length();
#endif

        portENTER_CRITICAL(&_mux);
        traceIdx = _head;
        LogEntry& e  = _buf[_head];
        e.absMs      = absMs;
        e.timeStr    = timeStr;
        e.event      = event;
        e.T1 = t1;
        e.T2 = t2;
        e.T3 = t3;
        e.dT = dt;
        _head = (_head + 1) % LOG_MAX_ENTRIES;
        if (_count < LOG_MAX_ENTRIES) _count++;
        portEXIT_CRITICAL(&_mux);

#if STABILITY_BOOT_DIAG
        if (traceEvent) {
            Serial.printf("[LOG] ms=%lu idx=%d len=%u %.*s\n",
                          (unsigned long)absMs,
                          traceIdx,
                          traceLen,
                          96,
                          event.c_str());
        }
#endif
    }

    void refreshTimeStrings() {
        if (!_tb) return;
        const int snapshotCount = count();
        for (int i = 0; i < snapshotCount; i++) {
            uint64_t absMs = 0;
            int idx = -1;

            portENTER_CRITICAL(&_mux);
            if (i < _count) {
                idx = _entryIndexNoLock(i);
                absMs = _buf[idx].absMs;
            }
            portEXIT_CRITICAL(&_mux);

            if (idx < 0) break;

            const String freshTime = _tb->millisToStr(absMs);

            portENTER_CRITICAL(&_mux);
            if (i < _count) {
                const int currentIdx = _entryIndexNoLock(i);
                if (currentIdx == idx && _buf[idx].absMs == absMs) {
                    _buf[idx].timeStr = freshTime;
                }
            }
            portEXIT_CRITICAL(&_mux);
        }
    }

    int count() const {
        portENTER_CRITICAL(&_mux);
        const int currentCount = _count;
        portEXIT_CRITICAL(&_mux);
        return currentCount;
    }

    int size() const { return count(); }

    const LogEntry& get(int i) const {
        portENTER_CRITICAL(&_mux);
        LogEntry& snapshot = _readSnapshot[_readSnapshotHead];
        _readSnapshotHead = (uint8_t)((_readSnapshotHead + 1) % READ_SNAPSHOT_COUNT);
        if (i < 0 || i >= _count) {
            snapshot = LogEntry{};
        } else {
            snapshot = _buf[_entryIndexNoLock(i)];
        }
        portEXIT_CRITICAL(&_mux);
        return snapshot;
    }

    String toPlainText(bool newestFirst = true) const {
        String out;
        const int snapshotCount = count();
        for (int displayIdx = 0; displayIdx < snapshotCount; displayIdx++) {
            const int i = newestFirst ? (snapshotCount - 1 - displayIdx) : displayIdx;
            LogEntry e;
            if (!_copyEntry(i, e)) break;
            if (out.length() > 0) out += "\r\n\r\n";
            out += e.timeStr;
            out += "\r\n";
            out += e.event;
            out += "\r\n";
            out += "T1=";
            out += _fmtPlainValue(e.T1);
            out += " | T2=";
            out += _fmtPlainValue(e.T2);
            out += " | T3=";
            out += _fmtPlainValue(e.T3);
            out += " | dT=";
            out += _fmtPlainValue(e.dT);
        }
        return out;
    }

    void clear() {
        portENTER_CRITICAL(&_mux);
        _head = 0;
        _count = 0;
        portEXIT_CRITICAL(&_mux);
    }

private:
    static constexpr uint8_t READ_SNAPSHOT_COUNT = 4;

    TimeBase* _tb;
    LogEntry  _buf[LOG_MAX_ENTRIES];
    int       _head;
    int       _count;
    mutable portMUX_TYPE _mux = portMUX_INITIALIZER_UNLOCKED;
    mutable LogEntry _readSnapshot[READ_SNAPSHOT_COUNT];
    mutable uint8_t _readSnapshotHead = 0;

    int _entryIndexNoLock(int i) const {
        return (_count < LOG_MAX_ENTRIES) ? i : (_head + i) % LOG_MAX_ENTRIES;
    }

    bool _copyEntry(int i, LogEntry& outEntry) const {
        portENTER_CRITICAL(&_mux);
        if (i < 0 || i >= _count) {
            portEXIT_CRITICAL(&_mux);
            outEntry = LogEntry{};
            return false;
        }
        outEntry = _buf[_entryIndexNoLock(i)];
        portEXIT_CRITICAL(&_mux);
        return true;
    }

    static String _fmtPlainValue(float v) {
        if (isnan(v)) return "null";
        return String(v, 2);
    }
};
