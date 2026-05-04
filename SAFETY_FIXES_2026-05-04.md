# RectColumn Safety and EMU Updates - 2026-05-04

## Firmware logic

- Fixed sensor alarm behavior so `error` / `!present` raise the primary enabled alarm slot before generic `NAN` handling.
- Added fail-safe OFF behavior for enabled control rules when the controlling sensor is invalid, stale, absent, or in error.
- Added stale-data support with `lastValidMs` and age-based invalidation.
- Unified threshold conversion for ADC percent thresholds in both alarms and control rules.
- Restricted CH1..CH3 scheme control to `T1/T2/T3/P/L/F`; `dT/C/V` are no longer allowed to drive the main outputs.
- Corrected the GPIO35 hardware model: `WER_CH2=IO35`, `WER_CH3=IO34`, `WER_CH1=IO27`, `WER_CH4=IO36`.
- Updated `V` availability logic so it is disabled only when IO35 is compiled for `WER_CH2`.
- Added latched WER fault handling for:
  - no ON confirmation,
  - stuck ON while output is expected OFF,
  - confirmation HIGH before ON command.
- Added explicit operator reset endpoint for latched safety/WER faults: `POST /api/v1/safety/reset`.
- Split safety timing from user-editable control delays and moved safety timers to firmware constants.
- Improved level, flow, and pressure safety handling to match the intended fail-safe behavior more closely.
- Storage load now normalizes unsafe persisted values for periods, delays, logic modes, rule ranges, and forbidden scheme rules.

## EMU mode

- Changed emulator defaults to start with `L=true` and `F=true`, so EMU starts in a normal process state instead of an immediate level/flow fault state.
- This reduces false manual-control blocking right after boot in `EMU_MODE`.

## emupanel-v3

- Updated the panel to reflect the current API contract without changing its overall visual design.
- Added visibility for:
  - `effectiveForbidMask`,
  - `forbidReasons`,
  - relay pending/error state,
  - confirmation `faultLatched` / `fault`,
  - `safetyAlarmActive`.
- Added `safety reset` action wired to `POST /api/v1/safety/reset`.
- Added `alarmDelayMs` and `ctrlDelayMs` fields to sensor config editing.
- Changed panel reset/default EMU preset so `L` and `F` are ON by default.

## API tests

- Added shared live-test helper: `tools/api_testlib.py`
- Added EMU smoke tests: `tools/test_api_emu_smoke.py`
- Added EMU manual-control logic tests: `tools/test_api_emu_manual_logic.py`
- Added EMU panel/API contract tests: `tools/test_api_emupanel_contract.py`

## Verification

- `arduino-cli compile --fqbn esp32:esp32:esp32 .` passed.
- `python tools/auto_logic_host_test.py` passed.
- `python tools/test_api_emupanel_contract.py` passed for static checks.
- Live HTTP API tests were created and are ready, but could not be executed from this sandbox because the device at `http://192.168.4.1` was unreachable here.
