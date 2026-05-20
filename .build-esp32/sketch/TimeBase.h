#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\TimeBase.h"
// ================================================================
// TimeBase.h
// ================================================================
#pragma once
#include <Arduino.h>
#include <time.h>

class TimeBase {
public:
    TimeBase()
        : _synced(false),
          _bootUnixMs(0),
          _lastSyncRefMs(0),
          _tzOffsetMin(0) {}

    // additive extension:
    // - old clients may still call sync(unixTimeMs, millisAtClientSend)
    // - new clients may also pass tzOffsetMin (minutes east of UTC, same sign as JS Date#getTimezoneOffset() inverted)
    void sync(uint64_t unixTimeMs, uint32_t millisAtClientSend = 0, int32_t tzOffsetMin = 0) {
        const uint32_t refMs = millisAtClientSend ? millisAtClientSend : millis();

        _bootUnixMs    = (unixTimeMs >= (uint64_t)refMs) ? (unixTimeMs - (uint64_t)refMs) : 0ULL;
        _lastSyncRefMs = refMs;
        _tzOffsetMin   = tzOffsetMin;
        _synced        = true;
    }

    uint64_t nowMs() const {
        return _synced ? (_bootUnixMs + (uint64_t)millis()) : (uint64_t)millis();
    }

    String nowStr() const {
        return millisToStr((uint64_t)millis());
    }

    bool isSynced() const { return _synced; }
    uint32_t lastSyncRefMs() const { return _lastSyncRefMs; }
    int32_t tzOffsetMin() const { return _tzOffsetMin; }

    uint64_t nowLocalUnixMs() const {
        if (!_synced) return (uint64_t)millis();
        const int64_t shifted = (int64_t)nowMs() + (int64_t)_tzOffsetMin * 60000LL;
        return shifted > 0 ? (uint64_t)shifted : 0ULL;
    }

    String millisToStr(uint64_t ms) const {
        if (!_synced) return String("T+") + (ms / 1000ULL) + "s";

        const int64_t shiftedMs = (int64_t)(_bootUnixMs + ms) + (int64_t)_tzOffsetMin * 60000LL;
        const time_t sec = (time_t)((shiftedMs > 0 ? shiftedMs : 0LL) / 1000LL);
        struct tm t;
        gmtime_r(&sec, &t);
        char buf[24];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                 t.tm_hour, t.tm_min, t.tm_sec);
        return String(buf);
    }

private:
    bool     _synced;
    uint64_t _bootUnixMs;
    uint32_t _lastSyncRefMs;
    int32_t  _tzOffsetMin;
};
