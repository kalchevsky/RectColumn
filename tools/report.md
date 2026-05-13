# RectColumn API Audit Report

- Generated: 2026-05-13 17:12:14
- Base URL: `https://controlsystem.dmitrystrootz.crazedns.ru`
- Firmware: `1.6.24`
- API version: `v1`
- EMU mode: `False`
- Active tests enabled: `False`
- EMU scenarios enabled: `False`

## Summary

- PASS: 3
- WARN: 10
- FAIL: 0
- SKIP: 1

## Results

| Status | Test | Detail |
| --- | --- | --- |
| PASS | `health` | Health endpoint returned ok=true |
| PASS | `schema` | Schema lists 5 outputs and 9 sensors |
| WARN | `sensor:T1` | Sensor is disabled in configuration |
| WARN | `sensor:T2` | Sensor is disabled in configuration |
| WARN | `sensor:T3` | Sensor is disabled in configuration |
| WARN | `sensor:dT` | Sensor is disabled in configuration |
| WARN | `sensor:P` | Sensor is disabled in configuration |
| WARN | `sensor:L` | Sensor is disabled in configuration |
| WARN | `sensor:F` | Sensor is disabled in configuration |
| WARN | `sensor:C` | Sensor is disabled in configuration |
| WARN | `sensor:V` | GPIO35 зарезервирован под WER_CH2 в этой сборке |
| WARN | `sensor-summary` | Sensors with issues: fail=0, warn=9 |
| PASS | `wifi-scan` | WiFi scan returned 13 visible network(s) |
| SKIP | `active-checks` | Run again with --allow-write to test STOP and manual output control |

## Device Snapshot

```json
{
  "info": {
    "name": "RectColumn",
    "fw": "1.6.24",
    "apiVersion": "v1",
    "emu": false,
    "synced": true,
    "time": "2026-05-13 17:12:13",
    "apIP": "192.168.4.1",
    "apRunning": true,
    "apStatusText": "open",
    "staIP": "192.168.1.66",
    "rssi": -36,
    "staRssi": -36,
    "apRssi": -127,
    "apClientCount": 0,
    "lastScanStatus": 21,
    "lastScanStatusText": "OK_21",
    "lastScanCount": 13,
    "reconnectPauseMs": 10348,
    "muted": false,
    "ch4Enabled": false,
    "ch5Enabled": false,
    "stopLatched": false,
    "wifiWizardPending": false,
    "notifyEnabled": true,
    "wifiWizard": {
      "pending": false,
      "servicePage": "/wifi",
      "staConfigured": true,
      "apProtected": false
    },
    "ap": {
      "ssid": "Control_System",
      "ip": "192.168.4.1",
      "protected": false,
      "clientCount": 0,
      "rssi": -127
    },
    "sta": {
      "configured": true,
      "connected": true,
      "ssid": "Keenetic-5459",
      "ip": "192.168.1.66",
      "rssi": -36,
      "statusText": "WL_CONNECTED",
      "reconnectPauseMs": 10347
    },
    "notify": {
      "enabled": true,
      "url": "http://ntfy.sh/Str79859996248",
      "hasToken": false
    },
    "servicePages": [
      "/",
      "/wifi"
    ],
    "operatorPages": [
      "/app"
    ],
    "endpoints": [
      "/api/v1/info",
      "/api/v1/version",
      "/api/v1/health",
      "/api/v1/schema",
      "/api/v1/diag",
      "/api/v1/state",
      "/api/v1/sensor",
      "/api/v1/output",
      "/api/v1/output/config",
      "/api/v1/log",
      "/api/v1/time/sync",
      "/api/v1/stop",
      "/api/v1/stop?release=1",
      "/api/v1/stop/",
      "/api/v1/stop/release",
      "/api/v1/stop/release/",
      "/api/v1/mute",
      "/api/v1/ack",
      "/api/v1/ack/",
      "/api/v1/safety/reset",
      "/api/v1/notify/config",
      "/api/v1/notify/test",
      "/api/v1/wifi/scan",
      "/api/v1/wifi/connect",
      "/api/v1/wifi/ap",
      "/api/v1/wifi/wizard/complete",
      "/api/v1/emu/set",
      "/api/v1/emu/scenario",
      "/app",
      "/app.css",
      "/app.js"
    ],
    "diag": {
      "pin35Mode": "WER_CH2",
      "adc2Warning": false,
      "staConnected": true,
      "staConfigured": true,
      "wifiQualityWarning": false,
      "apRunning": true,
      "apStatusText": "open",
      "apClientCount": 0,
      "apRssi": -127,
      "lastScanStatus": 21,
      "lastScanStatusText": "OK_21",
      "lastScanCount": 13,
      "reconnectPauseMs": 10346,
      "storageReady": true,
      "storageRecovered": false,
      "storageStatus": "ok",
      "stopLatched": false,
      "hardwareLimitations": [
        "GPIO35 общий для V и WER_CH2; в реальном режиме работает только то, что выбрано через GPIO35_MODE",
        "WER_CH1 (GPIO27) использует внутренний pull-down; для WER_CH2/CH3/CH4 (GPIO35/34/36) нужна внешняя подтяжка к GND"
      ],
      "activeConfirmMismatches": []
    }
  },
  "diag": {
    "pin35Mode": "WER_CH2",
    "adc2Warning": false,
    "staConnected": true,
    "staConfigured": true,
    "wifiQualityWarning": false,
    "apRunning": true,
    "apStatusText": "open",
    "apClientCount": 0,
    "apRssi": -127,
    "lastScanStatus": 21,
    "lastScanStatusText": "OK_21",
    "lastScanCount": 13,
    "reconnectPauseMs": 9984,
    "storageReady": true,
    "storageRecovered": false,
    "storageStatus": "ok",
    "stopLatched": false,
    "hardwareLimitations": [
      "GPIO35 общий для V и WER_CH2; в реальном режиме работает только то, что выбрано через GPIO35_MODE",
      "WER_CH1 (GPIO27) использует внутренний pull-down; для WER_CH2/CH3/CH4 (GPIO35/34/36) нужна внешняя подтяжка к GND"
    ],
    "activeConfirmMismatches": []
  },
  "outputs": [
    {
      "id": "CH1",
      "state": false,
      "actual": false,
      "displayOn": false,
      "requested": false,
      "manualWant": false,
      "operatorHoldOff": false,
      "forbidden": false,
      "forbidMask": 0,
      "sensorForbidMask": 0,
      "safetyForbidMask": 0,
      "effectiveForbidMask": 0,
      "forbidReasonText": "",
      "wantOnMask": 0,
      "forbidReasons": [],
      "enabled": true,
      "mode": "heat",
      "relayPending": false,
      "pendingCmd": null,
      "confirmAvailable": true,
      "confirmed": true,
      "mismatch": false,
      "timeout": false,
      "pending": false,
      "confirmActual": false,
      "confirmExpected": false,
      "feedbackOn": false,
      "confirmedLevel": 0
    },
    {
      "id": "CH2",
      "state": false,
      "actual": false,
      "displayOn": false,
      "requested": false,
      "manualWant": false,
      "operatorHoldOff": false,
      "forbidden": false,
      "forbidMask": 0,
      "sensorForbidMask": 0,
      "safetyForbidMask": 0,
      "effectiveForbidMask": 0,
      "forbidReasonText": "",
      "wantOnMask": 0,
      "forbidReasons": [],
      "enabled": true,
      "mode": "cool",
      "relayPending": false,
      "pendingCmd": null,
      "confirmAvailable": true,
      "confirmed": true,
      "mismatch": false,
      "timeout": false,
      "pending": false,
      "confirmActual": false,
      "confirmExpected": false,
      "feedbackOn": false,
      "confirmedLevel": 0
    },
    {
      "id": "CH3",
      "state": true,
      "actual": true,
      "displayOn": true,
      "requested": true,
      "manualWant": false,
      "operatorHoldOff": false,
      "forbidden": false,
      "forbidMask": 0,
      "sensorForbidMask": 0,
      "safetyForbidMask": 0,
      "effectiveForbidMask": 0,
      "forbidReasonText": "",
      "wantOnMask": 0,
      "forbidReasons": [],
      "enabled": true,
      "mode": "heat",
      "relayPending": false,
      "pendingCmd": null,
      "confirmAvailable": true,
      "confirmed": true,
      "mismatch": false,
      "timeout": false,
      "pending": false,
      "confirmActual": true,
      "confirmExpected": true,
      "feedbackOn": true,
      "confirmedLevel": 1
    },
    {
      "id": "CH4",
      "state": false,
      "actual": false,
      "displayOn": false,
      "requested": false,
      "manualWant": false,
      "operatorHoldOff": false,
      "forbidden": false,
      "forbidMask": 0,
      "sensorForbidMask": 0,
      "safetyForbidMask": 0,
      "effectiveForbidMask": 0,
      "forbidReasonText": "",
      "wantOnMask": 0,
      "forbidReasons": [],
      "enabled": false,
      "relayPending": false,
      "pendingCmd": null,
      "confirmAvailable": true,
      "confirmed": true,
      "mismatch": false,
      "timeout": false,
      "pending": false,
      "confirmActual": false,
      "confirmExpected": false,
      "feedbackOn": false,
      "confirmedLevel": 0
    },
    {
      "id": "CH5",
      "state": false,
      "actual": false,
      "displayOn": false,
      "requested": false,
      "manualWant": false,
      "operatorHoldOff": false,
      "forbidden": false,
      "forbidMask": 0,
      "sensorForbidMask": 0,
      "safetyForbidMask": 0,
      "effectiveForbidMask": 0,
      "forbidReasonText": "",
      "wantOnMask": 0,
      "forbidReasons": [],
      "enabled": false,
      "relayPending": false,
      "pendingCmd": null
    }
  ],
  "sensors": [
    {
      "id": "T1",
      "enabled": false,
      "periodMs": 1000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "C",
      "hwLimited": false,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 25,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 10,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 98,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 97,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": true,
          "logic": "heat",
          "min": 20,
          "max": 25,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 15,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 25,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "T2",
      "enabled": false,
      "periodMs": 3000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "C",
      "hwLimited": false,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": false,
          "logic": "heat",
          "min": 25,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": true,
          "logic": "cool",
          "min": 10,
          "max": 25,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 30,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "T3",
      "enabled": false,
      "periodMs": 3000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "C",
      "hwLimited": false,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": true,
          "logic": "heat",
          "min": 20,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 25,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "dT",
      "enabled": false,
      "periodMs": 15000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "C",
      "hwLimited": false,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": -2,
          "max": -1,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "P",
      "enabled": false,
      "periodMs": 5000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "hPa",
      "hwLimited": false,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 1000,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 1013,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 1013,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "L",
      "enabled": false,
      "periodMs": 100,
      "error": false,
      "present": true,
      "stale": true,
      "lastValidMs": 697,
      "unit": "",
      "hwLimited": false,
      "alarmDelayMs": 5000,
      "ctrlDelayMs": 300000,
      "value": 0,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": true,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "F",
      "enabled": false,
      "periodMs": 1000,
      "error": false,
      "present": true,
      "stale": true,
      "lastValidMs": 697,
      "unit": "",
      "hwLimited": false,
      "alarmDelayMs": 2000,
      "ctrlDelayMs": 5000,
      "value": 1,
      "alarms": [
        {
          "enabled": true,
          "threshold": 0,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": true,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "cool",
          "min": 0.5,
          "max": 2,
          "fixedOffOnly": true,
          "logicLocked": true,
          "schemeAllowed": true
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "C",
      "enabled": false,
      "periodMs": 1000,
      "error": false,
      "present": true,
      "stale": false,
      "lastValidMs": 0,
      "unit": "",
      "hwLimited": false,
      "alarmDelayMs": 3000,
      "ctrlDelayMs": 0,
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 30,
          "isMax": false,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    },
    {
      "id": "V",
      "enabled": true,
      "periodMs": 1000,
      "error": true,
      "present": false,
      "stale": false,
      "lastValidMs": 0,
      "unit": "",
      "hwLimited": true,
      "alarmDelayMs": 0,
      "ctrlDelayMs": 0,
      "note": "GPIO35 зарезервирован под WER_CH2 в этой сборке",
      "value": null,
      "alarms": [
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        },
        {
          "enabled": false,
          "threshold": 0,
          "isMax": true,
          "triggered": false,
          "unacked": false
        }
      ],
      "ctrl": [
        {
          "outIdx": 0,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 1,
          "enabled": false,
          "logic": "cool",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 2,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": false
        },
        {
          "outIdx": 3,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        },
        {
          "outIdx": 4,
          "enabled": false,
          "logic": "heat",
          "min": 0,
          "max": 100,
          "fixedOffOnly": false,
          "logicLocked": false,
          "schemeAllowed": true
        }
      ]
    }
  ],
  "confirmations": [
    {
      "id": "WER_CH1",
      "outputId": "CH1",
      "available": true,
      "raw": false,
      "actual": false,
      "expected": false,
      "confirmed": true,
      "pending": false,
      "mismatch": false,
      "timeout": false,
      "faultLatched": false,
      "fault": "",
      "faultText": "",
      "timeoutMs": 1000,
      "debounceMs": 80,
      "emuMode": "auto",
      "emuModeText": "авто"
    },
    {
      "id": "WER_CH2",
      "outputId": "CH2",
      "available": true,
      "raw": false,
      "actual": false,
      "expected": false,
      "confirmed": true,
      "pending": false,
      "mismatch": false,
      "timeout": false,
      "faultLatched": false,
      "fault": "",
      "faultText": "",
      "timeoutMs": 5000,
      "debounceMs": 80,
      "emuMode": "auto",
      "emuModeText": "авто"
    },
    {
      "id": "WER_CH3",
      "outputId": "CH3",
      "available": true,
      "raw": true,
      "actual": true,
      "expected": true,
      "confirmed": true,
      "pending": false,
      "mismatch": false,
      "timeout": false,
      "faultLatched": false,
      "fault": "",
      "faultText": "",
      "timeoutMs": 5000,
      "debounceMs": 80,
      "emuMode": "auto",
      "emuModeText": "авто"
    },
    {
      "id": "WER_CH4",
      "outputId": "CH4",
      "available": true,
      "raw": false,
      "actual": false,
      "expected": false,
      "confirmed": true,
      "pending": false,
      "mismatch": false,
      "timeout": false,
      "faultLatched": false,
      "fault": "",
      "faultText": "",
      "timeoutMs": 1000,
      "debounceMs": 80,
      "emuMode": "auto",
      "emuModeText": "авто"
    }
  ]
}
```
