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
        LogEntry& e  = _buf[_head];
        e.absMs      = (uint64_t)millis();
        e.timeStr    = _tb ? _tb->millisToStr(e.absMs) : (String("T+") + (e.absMs / 1000) + "s");
        e.event      = event;
        e.T1 = t1;
        e.T2 = t2;
        e.T3 = t3;
        e.dT = dt;

        _head = (_head + 1) % LOG_MAX_ENTRIES;
        if (_count < LOG_MAX_ENTRIES) _count++;
    }

    void refreshTimeStrings() {
        if (!_tb) return;
        for (int i = 0; i < _count; i++) {
            _buf[i].timeStr = _tb->millisToStr(_buf[i].absMs);
        }
    }

    int count() const { return _count; }

    const LogEntry& get(int i) const {
        int idx = (_count < LOG_MAX_ENTRIES) ? i : (_head + i) % LOG_MAX_ENTRIES;
        return _buf[idx];
    }

    void clear() {
        _head = 0;
        _count = 0;
    }

private:
    TimeBase* _tb;
    LogEntry  _buf[LOG_MAX_ENTRIES];
    int       _head;
    int       _count;
};
