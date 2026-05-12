#!/usr/bin/env python3
"""
RectColumn host-side logic tests.

Цель: зафиксировать ожидаемую логику по блок-схеме и по целевой
FSM-архитектуре без ESP32-железа и без изменения Web UI.

Запуск из корня проекта:
    python3 -m unittest -v tests/test_logic_scheme.py

Файл содержит два типа проверок:
1) model/spec tests — проверяют ожидаемую семантику автоматики;
2) source guard tests — читают config.h / *.h и ловят известные
   расхождения исходников с целевой логикой.

Если source guard падает на старом коде — это нормально: тест показывает,
какой участок нужно исправить в C++.
"""

from __future__ import annotations

import math
import re
import unittest
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from typing import Dict, Optional, Tuple


# ---------------------------------------------------------------------------
# Constants from config.h / flowchart model
# ---------------------------------------------------------------------------

LOGIC_HEAT = 0
LOGIC_COOL = 1

OUT_CH1 = 0
OUT_CH2 = 1
OUT_CH3 = 2
OUT_CH4 = 3
OUT_CH5 = 4
OUT_COUNT = 5

SEN_T1 = 0
SEN_T2 = 1
SEN_T3 = 2
SEN_DT = 3
SEN_P = 4
SEN_L = 5
SEN_F = 6
SEN_C = 7
SEN_V = 8
SEN_COUNT = 9

SAFETY_LEVEL = 30
SAFETY_FLOW = 31
SAFETY_PRESSURE = 32
SAFETY_WER = 33
SAFETY_STOP = 34
SAFETY_SOUND = 35

GPIO35_MODE_V_SENSOR = 0
GPIO35_MODE_WER_CH2 = 1


# ---------------------------------------------------------------------------
# Pure Python model of expected target behavior
# ---------------------------------------------------------------------------

class FailSafeMode(Enum):
    NEUTRAL = auto()
    FORCE_OFF = auto()
    FORCE_ON = auto()
    LATCH_FAULT = auto()


class RelayState(Enum):
    OFF = auto()
    ON = auto()
    SWITCHING_ON = auto()
    SWITCHING_OFF = auto()
    FAULT_NO_CONFIRM = auto()
    FAULT_STUCK_ON = auto()
    FAULT_STUCK_HIGH_BEFORE_ON = auto()
    LOCKED_OUT = auto()


@dataclass
class CtrlRule:
    enabled: bool = False
    out_idx: int = 0
    logic: int = LOGIC_HEAT
    min_val: float = 0.0
    max_val: float = 100.0
    fail_safe: FailSafeMode = FailSafeMode.FORCE_OFF
    on_delay_ms: int = 0
    off_delay_ms: int = 0

    def validate(self) -> None:
        if self.out_idx < 0 or self.out_idx >= OUT_COUNT:
            raise ValueError("out_idx out of range")
        if self.logic not in (LOGIC_HEAT, LOGIC_COOL):
            raise ValueError("logic must be LOGIC_HEAT or LOGIC_COOL")
        if not self.min_val < self.max_val:
            raise ValueError("min_val must be lower than max_val")


@dataclass
class Sensor:
    enabled: bool = True
    present: bool = True
    error: bool = False
    value: float = math.nan
    threshold_percent_input: bool = False
    ctrl: Dict[int, CtrlRule] = field(default_factory=dict)
    alarm_enabled: Tuple[bool, bool, bool, bool] = (False, False, False, False)
    alarm_threshold: Tuple[float, float, float, float] = (0.0, 0.0, 0.0, 0.0)
    alarm_is_max: Tuple[bool, bool, bool, bool] = (True, True, True, True)

    def usable(self) -> bool:
        return self.enabled and self.present and not self.error and not math.isnan(self.value)

    def effective_threshold(self, raw: float) -> float:
        if self.threshold_percent_input and 0.0 <= raw <= 100.0:
            return raw * 4095.0 / 100.0
        return raw

    def alarm_mask(self) -> int:
        # Target behavior: sensor loss/error/NAN triggers the first enabled alarm slot.
        if not self.enabled:
            return 0
        if self.error or not self.present or math.isnan(self.value):
            for i, enabled in enumerate(self.alarm_enabled):
                if enabled:
                    return 1 << i
            return 0
        mask = 0
        for i, enabled in enumerate(self.alarm_enabled):
            if not enabled:
                continue
            threshold = self.effective_threshold(self.alarm_threshold[i])
            active = self.value > threshold if self.alarm_is_max[i] else self.value < threshold
            if active:
                mask |= 1 << i
        return mask

    def eval_ctrl(self, out_idx: int, control_gate: bool = True) -> int:
        rule = self.ctrl.get(out_idx)
        if not rule or not rule.enabled or rule.out_idx != out_idx:
            return 0
        rule.validate()

        if not control_gate:
            return 0

        if not self.usable():
            if rule.fail_safe in (FailSafeMode.FORCE_OFF, FailSafeMode.LATCH_FAULT):
                return -1
            if rule.fail_safe is FailSafeMode.FORCE_ON:
                return 1
            return 0

        if rule.logic == LOGIC_HEAT:
            if self.value < rule.min_val:
                return 1
            if self.value > rule.max_val:
                return -1
        else:
            if self.value > rule.max_val:
                return 1
            if self.value < rule.min_val:
                return -1
        return 0


def normalize_lf_off_only(rule: CtrlRule, out_idx: int) -> CtrlRule:
    # L/F by scheme are OFF-forbid protection signals:
    # input HIGH/1 = normal => neutral; LOW/0 = emergency => OFF.
    return CtrlRule(
        enabled=rule.enabled,
        out_idx=out_idx,
        logic=LOGIC_COOL,
        min_val=0.5,
        max_val=2.0,
        fail_safe=FailSafeMode.NEUTRAL,
        on_delay_ms=0,
        off_delay_ms=rule.off_delay_ms,
    )


@dataclass
class ArbiterOutput:
    actual_on: bool = False
    requested_on: bool = False
    manual_want: bool = False

    def apply(self, forbid_mask: int, want_mask: int) -> bool:
        # Flowchart priority: OFF > ON > manual/hold.
        if forbid_mask:
            self.manual_want = False
            self.requested_on = False
        elif want_mask:
            self.requested_on = True
        else:
            self.requested_on = self.actual_on
        self.actual_on = self.requested_on
        return self.actual_on

    def manual(self, on: bool, forbid_mask: int = 0) -> bool:
        if on and forbid_mask:
            return False
        self.manual_want = on
        self.requested_on = bool(on and not forbid_mask)
        self.actual_on = self.requested_on
        return True


@dataclass
class ConfirmationFSM:
    state: RelayState = RelayState.OFF
    command_on: bool = False
    fault_latched: Optional[RelayState] = None
    t0_ms: int = 0
    timeout_ms: int = 1000

    def begin_on(self, now_ms: int, feedback_on: bool) -> None:
        if self.fault_latched:
            self.state = RelayState.LOCKED_OUT
            return
        if feedback_on:
            self.state = RelayState.FAULT_STUCK_HIGH_BEFORE_ON
            self.fault_latched = self.state
            return
        self.command_on = True
        self.state = RelayState.SWITCHING_ON
        self.t0_ms = now_ms

    def begin_off(self, now_ms: int) -> None:
        if self.fault_latched:
            self.state = RelayState.LOCKED_OUT
            return
        self.command_on = False
        self.state = RelayState.SWITCHING_OFF
        self.t0_ms = now_ms

    def loop(self, now_ms: int, feedback_on: bool) -> RelayState:
        if self.fault_latched:
            self.state = self.fault_latched
            return self.state
        if self.state is RelayState.SWITCHING_ON:
            if feedback_on:
                self.state = RelayState.ON
            elif now_ms - self.t0_ms >= self.timeout_ms:
                self.state = RelayState.FAULT_NO_CONFIRM
                self.fault_latched = self.state
        elif self.state is RelayState.SWITCHING_OFF:
            if not feedback_on:
                self.state = RelayState.OFF
            elif now_ms - self.t0_ms >= self.timeout_ms:
                self.state = RelayState.FAULT_STUCK_ON
                self.fault_latched = self.state
        elif self.state is RelayState.OFF and feedback_on:
            self.state = RelayState.FAULT_STUCK_ON
            self.fault_latched = self.state
        return self.state

    def reset(self, feedback_on: bool) -> bool:
        if feedback_on:
            return False
        self.fault_latched = None
        self.state = RelayState.OFF
        self.command_on = False
        return True


def aggregate_rules(sensors: Dict[int, Sensor], out_idx: int) -> Tuple[int, int]:
    forbid = 0
    want = 0
    for sensor_idx, sensor in sensors.items():
        cmd = sensor.eval_ctrl(out_idx)
        if cmd == -1:
            forbid |= 1 << sensor_idx
        elif cmd == 1:
            want |= 1 << sensor_idx
    return forbid, want


def sound_required(sensor_alarm_active: bool, safety_fault_active: bool, muted: bool = False) -> bool:
    return (sensor_alarm_active or safety_fault_active) and not muted


def validate_sensor_config_update(current: Sensor, patch: Dict[str, object]) -> Sensor:
    # Model of desired API behavior: validate a copy, then commit atomically.
    candidate = Sensor(**{field.name: getattr(current, field.name) for field in current.__dataclass_fields__.values()})
    if "enabled" in patch:
        candidate.enabled = bool(patch["enabled"])
    if "period_ms" in patch:
        period = int(patch["period_ms"])
        if period < 100 or period > 60_000:
            raise ValueError("period_ms out of safe range")
    return candidate


def validate_ctrl_update(current: CtrlRule, patch: Dict[str, object]) -> CtrlRule:
    candidate = CtrlRule(**current.__dict__)
    if "enabled" in patch:
        candidate.enabled = bool(patch["enabled"])
    if "logic" in patch:
        candidate.logic = int(patch["logic"])
    if "min_val" in patch:
        candidate.min_val = float(patch["min_val"])
    if "max_val" in patch:
        candidate.max_val = float(patch["max_val"])
    candidate.validate()
    return candidate


# ---------------------------------------------------------------------------
# Spec/model tests
# ---------------------------------------------------------------------------

class SchemeHysteresisTests(unittest.TestCase):
    def drive(self, logic: int, values) -> list[bool]:
        out = ArbiterOutput()
        states = []
        for value in values:
            s = Sensor(value=value)
            s.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, logic, 70, 80)
            forbid, want = aggregate_rules({SEN_T1: s}, OUT_CH1)
            states.append(out.apply(forbid, want))
        return states

    def test_heat_matches_flowchart(self):
        self.assertEqual(self.drive(LOGIC_HEAT, [69, 75, 81, 75, 69]),
                         [True, True, False, False, True])

    def test_cool_matches_flowchart(self):
        self.assertEqual(self.drive(LOGIC_COOL, [81, 75, 69, 75, 81]),
                         [True, True, False, False, True])

    def test_off_priority_over_on_when_two_sensors_conflict(self):
        t1 = Sensor(value=69)
        t1.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 70, 80)  # wants ON
        p = Sensor(value=101)
        p.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 20, 100)   # forbids/OFF
        forbid, want = aggregate_rules({SEN_T1: t1, SEN_P: p}, OUT_CH1)
        self.assertNotEqual(want, 0)
        self.assertNotEqual(forbid, 0)
        self.assertFalse(ArbiterOutput(actual_on=True).apply(forbid, want))

    def test_neutral_zone_holds_previous_state(self):
        out = ArbiterOutput(actual_on=True)
        self.assertTrue(out.apply(0, 0))
        out.actual_on = False
        self.assertFalse(out.apply(0, 0))


class DigitalLFTests(unittest.TestCase):
    def test_lf_low_is_off_only_and_high_is_neutral(self):
        rule = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        low = Sensor(value=0.0)
        low.ctrl[OUT_CH1] = rule
        high = Sensor(value=1.0)
        high.ctrl[OUT_CH1] = rule
        self.assertEqual(low.eval_ctrl(OUT_CH1), -1)
        self.assertEqual(high.eval_ctrl(OUT_CH1), 0)

    def test_lf_missing_sensor_is_neutral(self):
        rule = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        lost = Sensor(present=False, value=math.nan)
        lost.ctrl[OUT_CH1] = rule
        self.assertEqual(lost.eval_ctrl(OUT_CH1), 0)

    def test_flow_rule_is_ignored_while_output_is_off(self):
        rule = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        flow = Sensor(value=0.0)
        flow.ctrl[OUT_CH1] = rule
        self.assertEqual(flow.eval_ctrl(OUT_CH1, control_gate=False), 0)
        self.assertEqual(flow.eval_ctrl(OUT_CH1, control_gate=True), -1)


class SensorFaultAndAlarmTests(unittest.TestCase):
    def test_analog_control_sensor_error_is_neutral(self):
        t1 = Sensor(present=False, error=True, value=math.nan)
        t1.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 70, 80,
                                    fail_safe=FailSafeMode.NEUTRAL)
        self.assertEqual(t1.eval_ctrl(OUT_CH1), 0)

    def test_nan_error_raises_first_enabled_alarm_slot(self):
        t1 = Sensor(present=False, error=True, value=math.nan,
                    alarm_enabled=(False, True, True, False))
        self.assertEqual(t1.alarm_mask(), 0b0010)

    def test_percent_threshold_applies_consistently_to_adc_sensor(self):
        c = Sensor(value=2048, threshold_percent_input=True,
                   alarm_enabled=(True, False, False, False),
                   alarm_threshold=(40, 0, 0, 0),
                   alarm_is_max=(True, True, True, True))
        self.assertEqual(c.alarm_mask(), 0b0001)


class ConfirmationFsmTests(unittest.TestCase):
    def test_on_command_requires_feedback_transition_not_stuck_high(self):
        fsm = ConfirmationFSM()
        fsm.begin_on(now_ms=0, feedback_on=True)
        self.assertEqual(fsm.state, RelayState.FAULT_STUCK_HIGH_BEFORE_ON)
        self.assertFalse(fsm.reset(feedback_on=True))
        self.assertTrue(fsm.reset(feedback_on=False))

    def test_on_timeout_latches_fault_until_reset(self):
        fsm = ConfirmationFSM(timeout_ms=1000)
        fsm.begin_on(now_ms=0, feedback_on=False)
        self.assertEqual(fsm.loop(now_ms=999, feedback_on=False), RelayState.SWITCHING_ON)
        self.assertEqual(fsm.loop(now_ms=1000, feedback_on=False), RelayState.FAULT_NO_CONFIRM)
        self.assertEqual(fsm.loop(now_ms=2000, feedback_on=True), RelayState.FAULT_NO_CONFIRM)
        self.assertTrue(fsm.reset(feedback_on=False))
        self.assertEqual(fsm.state, RelayState.OFF)

    def test_off_timeout_detects_stuck_on(self):
        fsm = ConfirmationFSM(state=RelayState.ON, timeout_ms=1000)
        fsm.begin_off(now_ms=0)
        self.assertEqual(fsm.loop(now_ms=1001, feedback_on=True), RelayState.FAULT_STUCK_ON)

    def test_uncommanded_feedback_on_while_off_is_fault(self):
        fsm = ConfirmationFSM(state=RelayState.OFF)
        self.assertEqual(fsm.loop(now_ms=0, feedback_on=True), RelayState.FAULT_STUCK_ON)


class SafetyAndApiTests(unittest.TestCase):
    def test_safety_alarm_drives_sound_even_without_sensor_alarm(self):
        self.assertTrue(sound_required(sensor_alarm_active=False, safety_fault_active=True))
        self.assertFalse(sound_required(sensor_alarm_active=False, safety_fault_active=True, muted=True))

    def test_invalid_ctrl_update_is_atomic(self):
        current = CtrlRule(enabled=False, out_idx=OUT_CH1, logic=LOGIC_HEAT, min_val=70, max_val=80)
        with self.assertRaises(ValueError):
            validate_ctrl_update(current, {"enabled": True, "min_val": 90, "max_val": 80})
        self.assertFalse(current.enabled)
        self.assertEqual((current.min_val, current.max_val), (70, 80))


# ---------------------------------------------------------------------------
# Source guard tests. These are intentionally strict and should be updated
# only when the hardware/schematic changes.
# ---------------------------------------------------------------------------

class SourceGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[1]
        cls.config_h = (cls.root / "config.h").read_text(encoding="utf-8", errors="ignore")
        cls.sensors_h = (cls.root / "Sensors.h").read_text(encoding="utf-8", errors="ignore")
        cls.confirm_h = (cls.root / "ConfirmationManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.process_h = (cls.root / "ProcessSafety.h").read_text(encoding="utf-8", errors="ignore")
        cls.output_mgr_h = (cls.root / "OutputManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.main_ino = (cls.root / "RectColumn.ino").read_text(encoding="utf-8", errors="ignore")

    def define_int(self, name: str) -> int:
        m = re.search(rf"^\s*#define\s+{name}\s+(-?\d+)\b", self.config_h, re.MULTILINE)
        self.assertIsNotNone(m, f"missing #define {name}")
        return int(m.group(1))

    def test_feedback_pin_map_matches_hardware_statement(self):
        self.assertEqual(self.define_int("PIN_WER_CH1"), 27)
        self.assertEqual(self.define_int("PIN_WER_CH2"), 35)
        self.assertEqual(self.define_int("PIN_WER_CH3"), 34)
        self.assertEqual(self.define_int("PIN_WER_CH4"), 36)

    def test_gpio35_mode_names_match_actual_shared_pin(self):
        # GPIO35 is shared with WER_CH2, not WER_CH3.
        self.assertIn("GPIO35_MODE_WER_CH2", self.config_h)
        self.assertNotIn("GPIO35_MODE_WER_CH3", self.config_h)

    def test_gpio35_availability_disables_wer_ch2_when_v_sensor_enabled(self):
        # WER array index: 0=CH1, 1=CH2, 2=CH3, 3=CH4.
        self.assertRegex(self.confirm_h, r"idx\s*==\s*1[^\n]+GPIO35_MODE\s*!=\s*GPIO35_MODE_WER_CH2")
        self.assertIn("PIN_V == PIN_WER_CH2", self.config_h)

    def test_alarm_error_branch_is_before_nan_branch(self):
        primary = self.sensors_h.find("primaryErrorAlarmIdx")
        nan_branch = self.sensors_h.find("isnan(value)")
        self.assertGreater(primary, -1)
        self.assertGreater(nan_branch, -1)
        self.assertLess(primary, nan_branch,
                        "sensor error alarm must be handled before generic NAN clearing")

    def test_flow_emergency_timer_starts_from_ch2_demand(self):
        self.assertIn("_flowDemandStartedMs", self.process_h)
        self.assertIn("valveDemandActive", self.process_h)
        self.assertRegex(self.process_h, r"now\s*-\s*_flowDemandStartedMs\s*>=\s*ctrlDelayMs")

    def test_sensor_stop_mode_is_configurable(self):
        self.assertRegex(self.config_h, r"#define\s+SAFETY_MODE_SENSOR_STOP\s+(false|true|0|1)")
        self.assertIn("_applySensorStopIfEnabled", self.process_h)
        self.assertIn("if (!SAFETY_MODE_SENSOR_STOP) return;", self.process_h)

    def test_output_manager_has_global_stop_short_circuit(self):
        self.assertIn("if (_mainStopLatched)", self.output_mgr_h)
        self.assertIn("_applyGlobalStop();", self.output_mgr_h)
        self.assertIn("invalidMeansOff = false;", self.output_mgr_h)

    def test_flow_rule_is_gated_by_previous_output_state(self):
        self.assertIn("controlGate = prevState[outIdx];", self.output_mgr_h)

    def test_main_loop_skips_confirmation_and_safety_while_stop_is_active(self):
        self.assertIn("if (!outputMgr.mainStopLatched())", self.main_ino)
        self.assertIn("outputMgr.setSafetyAlarmActive(false);", self.main_ino)


if __name__ == "__main__":
    unittest.main(verbosity=2)
