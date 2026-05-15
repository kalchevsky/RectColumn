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
from itertools import combinations
import unittest
from dataclasses import dataclass, field
from enum import Enum, auto
from pathlib import Path
from typing import Dict, Optional, Tuple

try:  # pragma: no cover - depends on unittest invocation style
    from .human_report import human_case, record_human_detail
except ImportError:  # pragma: no cover
    from human_report import human_case, record_human_detail  # type: ignore


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

MAIN_CHANNELS = (OUT_CH1, OUT_CH2, OUT_CH3)
CONTROL_SENSOR_INDICES = (SEN_T1, SEN_T2, SEN_T3, SEN_P, SEN_L, SEN_F)
NON_CONTROL_SENSOR_INDICES = (SEN_DT, SEN_C, SEN_V)

CHANNEL_NAMES = {
    OUT_CH1: "CH1",
    OUT_CH2: "CH2",
    OUT_CH3: "CH3",
}

SENSOR_NAMES = {
    SEN_T1: "T1",
    SEN_T2: "T2",
    SEN_T3: "T3",
    SEN_DT: "dT",
    SEN_P: "P",
    SEN_L: "L",
    SEN_F: "F",
    SEN_C: "C",
    SEN_V: "V",
}

CONTROL_DELAY_MS = {
    SEN_T1: 0,
    SEN_T2: 0,
    SEN_T3: 0,
    SEN_P: 0,
    SEN_L: 5 * 60 * 1000,
    SEN_F: 5000,
}


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

        if not self.enabled:
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
        fail_safe=FailSafeMode.FORCE_OFF,
        on_delay_ms=0,
        off_delay_ms=rule.off_delay_ms,
    )


@dataclass
class ArbiterOutput:
    actual_on: bool = False
    requested_on: bool = False
    manual_want: bool = False
    operator_hold_off: bool = False

    def apply(self, forbid_mask: int, want_mask: int) -> bool:
        # Flowchart priority: OFF > ON > manual OFF > manual ON > hold.
        if forbid_mask:
            self.manual_want = False
            self.operator_hold_off = False
            self.requested_on = False
        elif want_mask:
            self.manual_want = False
            self.operator_hold_off = False
            self.requested_on = True
        elif self.operator_hold_off:
            self.manual_want = False
            self.operator_hold_off = False
            self.requested_on = False
        elif self.manual_want:
            self.manual_want = False
            self.requested_on = True
        else:
            self.requested_on = self.actual_on
        self.actual_on = self.requested_on
        return self.actual_on

    def manual(self, on: bool, forbid_mask: int = 0, want_mask: int = 0) -> bool:
        if on and forbid_mask:
            return False
        self.manual_want = bool(on)
        self.operator_hold_off = not on
        self.apply(forbid_mask, want_mask)
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


def control_delay_ms(sensor_idx: int) -> int:
    return CONTROL_DELAY_MS[sensor_idx]


def requires_wer_confirmation(out_idx: int) -> bool:
    return out_idx in MAIN_CHANNELS


def make_channel_rule(sensor_idx: int, out_idx: int, enabled: bool) -> CtrlRule:
    delay_ms = control_delay_ms(sensor_idx)
    if sensor_idx in (SEN_L, SEN_F):
        return normalize_lf_off_only(
            CtrlRule(enabled=enabled, out_idx=out_idx, off_delay_ms=delay_ms),
            out_idx,
        )
    return CtrlRule(
        enabled=enabled,
        out_idx=out_idx,
        logic=LOGIC_HEAT,
        min_val=20.0,
        max_val=80.0,
        fail_safe=FailSafeMode.FORCE_OFF,
        off_delay_ms=delay_ms,
    )


def build_control_sensor(sensor_idx: int,
                         enabled_channels: Tuple[int, ...] = (),
                         fault: bool = False) -> Sensor:
    if sensor_idx in (SEN_L, SEN_F):
        sensor = Sensor(value=0.0 if fault else 1.0)
    else:
        sensor = Sensor(value=90.0 if fault else 50.0)
    for out_idx in MAIN_CHANNELS:
        sensor.ctrl[out_idx] = make_channel_rule(sensor_idx, out_idx, out_idx in enabled_channels)
    return sensor


def eval_ctrl_timed(sensor_idx: int, sensor: Sensor, out_idx: int,
                    elapsed_ms: int, prev_output_on: bool = True,
                    linked_output_on: Optional[bool] = None) -> int:
    gate = True
    if sensor_idx == SEN_F:
        gate = linked_output_on if linked_output_on is not None else prev_output_on
    cmd = sensor.eval_ctrl(out_idx, control_gate=gate)
    rule = sensor.ctrl.get(out_idx)
    if not rule:
        return cmd
    if cmd == 1 and elapsed_ms < rule.on_delay_ms:
        return 0
    if cmd == -1 and elapsed_ms < rule.off_delay_ms:
        return 0
    return cmd


def aggregate_rules_timed(sensors: Dict[int, Sensor], out_idx: int,
                          elapsed_ms, prev_output_on: bool = True,
                          linked_output_on: Optional[bool] = None) -> Tuple[int, int]:
    forbid = 0
    want = 0
    for sensor_idx, sensor in sensors.items():
        sensor_elapsed = elapsed_ms.get(sensor_idx, 0) if isinstance(elapsed_ms, dict) else elapsed_ms
        cmd = eval_ctrl_timed(sensor_idx, sensor, out_idx, sensor_elapsed,
                              prev_output_on=prev_output_on,
                              linked_output_on=linked_output_on)
        if cmd == -1:
            forbid |= 1 << sensor_idx
        elif cmd == 1:
            want |= 1 << sensor_idx
    return forbid, want


def simulate_main_channel_states(sensors: Dict[int, Sensor], elapsed_ms,
                                 initial_on: bool = True,
                                 extra_forbid_masks: Optional[Dict[int, int]] = None) -> Dict[int, bool]:
    extra_forbid_masks = extra_forbid_masks or {}
    states: Dict[int, bool] = {}
    for out_idx in MAIN_CHANNELS:
        out = ArbiterOutput(actual_on=initial_on, requested_on=initial_on)
        forbid, want = aggregate_rules_timed(sensors, out_idx, elapsed_ms, prev_output_on=initial_on)
        forbid |= extra_forbid_masks.get(out_idx, 0)
        out.apply(forbid, want)
        states[out_idx] = out.actual_on
    return states


def state_tuple(states: Dict[int, bool]) -> Tuple[bool, bool, bool]:
    return tuple(states[out_idx] for out_idx in MAIN_CHANNELS)


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

    def test_lf_missing_sensor_forms_auto_off(self):
        rule = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        lost = Sensor(present=False, value=math.nan)
        lost.ctrl[OUT_CH1] = rule
        self.assertEqual(lost.eval_ctrl(OUT_CH1), -1)

    def test_flow_rule_is_ignored_while_output_is_off(self):
        rule = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        flow = Sensor(value=0.0)
        flow.ctrl[OUT_CH1] = rule
        self.assertEqual(flow.eval_ctrl(OUT_CH1, control_gate=False), 0)
        self.assertEqual(flow.eval_ctrl(OUT_CH1, control_gate=True), -1)

    @human_case(
        title="Проток F влияет на CH1 только через состояние CH2",
        situation="Для CH1 включено правило по протоку F, но gate для этого правила должен зависеть от текущего состояния CH2.",
        steps=[
            "Создать аварийный датчик F с enabled rule только для CH1.",
            "Проверить поведение при linked_output_on=False.",
            "Проверить поведение при linked_output_on=True.",
        ],
        expected="При CH2 OFF датчик F нейтрален для CH1, а при CH2 ON формирует auto-off.",
    )
    def test_flow_rule_for_ch1_depends_on_ch2_state_in_scheme(self):
        flow = build_control_sensor(SEN_F, enabled_channels=(OUT_CH1,), fault=True)
        delay = control_delay_ms(SEN_F)
        record_human_detail(self, "flow_rule", flow.ctrl[OUT_CH1].__dict__)
        record_human_detail(self, "delay_ms", delay)
        self.assertEqual(
            eval_ctrl_timed(SEN_F, flow, OUT_CH1, delay, linked_output_on=False),
            0,
        )
        self.assertEqual(
            eval_ctrl_timed(SEN_F, flow, OUT_CH1, delay, linked_output_on=True),
            -1,
        )

    def test_flow_rule_is_channel_local(self):
        flow = Sensor(value=0.0)
        flow.ctrl[OUT_CH1] = normalize_lf_off_only(CtrlRule(enabled=True), OUT_CH1)
        flow.ctrl[OUT_CH2] = normalize_lf_off_only(CtrlRule(enabled=False), OUT_CH2)
        flow.ctrl[OUT_CH3] = normalize_lf_off_only(CtrlRule(enabled=False), OUT_CH3)

        self.assertEqual(aggregate_rules({SEN_F: flow}, OUT_CH1), (1 << SEN_F, 0))
        self.assertEqual(aggregate_rules({SEN_F: flow}, OUT_CH2), (0, 0))
        self.assertEqual(aggregate_rules({SEN_F: flow}, OUT_CH3), (0, 0))


class ChannelSensorMatrixTests(unittest.TestCase):
    def assertChannelStateTuple(self, states: Dict[int, bool], expected: Tuple[bool, bool, bool]) -> None:
        self.assertEqual(state_tuple(states), expected)

    @human_case(
        title="Аварийный датчик не отключает каналы, если ruleEnabled=false",
        situation="Контролируемый датчик находится в fault/error состоянии, но для канала правило управления этим датчиком выключено.",
        steps=[
            "Для каждого control sensor подготовить аварийное состояние.",
            "Оставить enabled_channels пустым.",
            "Пересчитать состояния CH1/CH2/CH3.",
        ],
        expected="Все основные каналы остаются включёнными, потому что выключенное правило не должно создавать auto-off.",
    )
    def test_faulty_sensor_does_not_turn_off_channels_when_rule_disabled(self):
        for out_idx in MAIN_CHANNELS:
            for sensor_idx in CONTROL_SENSOR_INDICES:
                with self.subTest(channel=CHANNEL_NAMES[out_idx], sensor=SENSOR_NAMES[sensor_idx]):
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=(), fault=True),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(sensor_idx))
                    self.assertChannelStateTuple(states, (True, True, True))

    @human_case(
        title="Включённый датчик в нормальном диапазоне не выключает целевой канал",
        situation="Для канала активировано правило управления по датчику, но сам датчик находится в нормальном, неаварийном диапазоне.",
        steps=[
            "Для каждого control sensor включить правило на целевой канал.",
            "Подать нормальное значение без fault.",
            "Пересчитать состояния основных каналов.",
        ],
        expected="Целевой канал остаётся включённым, auto-off не формируется.",
    )
    def test_enabled_sensor_in_normal_state_keeps_target_channel_on(self):
        for out_idx in MAIN_CHANNELS:
            for sensor_idx in CONTROL_SENSOR_INDICES:
                with self.subTest(channel=CHANNEL_NAMES[out_idx], sensor=SENSOR_NAMES[sensor_idx]):
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=(out_idx,), fault=False),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(sensor_idx))
                    self.assertChannelStateTuple(states, (True, True, True))

    def test_fault_before_timeout_keeps_enabled_channel_on_when_sensor_has_delay(self):
        for out_idx in MAIN_CHANNELS:
            for sensor_idx in CONTROL_SENSOR_INDICES:
                delay_ms = control_delay_ms(sensor_idx)
                with self.subTest(channel=CHANNEL_NAMES[out_idx], sensor=SENSOR_NAMES[sensor_idx]):
                    if delay_ms == 0:
                        self.assertEqual(delay_ms, 0)
                        continue
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=(out_idx,), fault=True),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=delay_ms - 1)
                    self.assertChannelStateTuple(states, (True, True, True))

    def test_fault_after_timeout_turns_off_only_the_enabled_channel(self):
        for out_idx in MAIN_CHANNELS:
            for sensor_idx in CONTROL_SENSOR_INDICES:
                with self.subTest(channel=CHANNEL_NAMES[out_idx], sensor=SENSOR_NAMES[sensor_idx]):
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=(out_idx,), fault=True),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(sensor_idx))
                    expected = []
                    for current_idx in MAIN_CHANNELS:
                        expected.append(current_idx != out_idx)
                    self.assertChannelStateTuple(states, tuple(expected))

    def test_fault_turns_off_exactly_channels_where_same_sensor_is_enabled(self):
        channel_groups = tuple(combinations(MAIN_CHANNELS, 2)) + (MAIN_CHANNELS,)
        for sensor_idx in CONTROL_SENSOR_INDICES:
            for enabled_channels in channel_groups:
                with self.subTest(sensor=SENSOR_NAMES[sensor_idx],
                                  enabled=[CHANNEL_NAMES[idx] for idx in enabled_channels]):
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=enabled_channels, fault=True),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(sensor_idx))
                    expected = tuple(out_idx not in enabled_channels for out_idx in MAIN_CHANNELS)
                    self.assertChannelStateTuple(states, expected)

    def test_fault_on_one_enabled_channel_does_not_turn_off_other_enabled_channels(self):
        for sensor_idx in CONTROL_SENSOR_INDICES:
            for protected_idx in MAIN_CHANNELS:
                with self.subTest(sensor=SENSOR_NAMES[sensor_idx], enabled=CHANNEL_NAMES[protected_idx]):
                    sensors = {
                        sensor_idx: build_control_sensor(sensor_idx, enabled_channels=(protected_idx,), fault=True),
                    }
                    states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(sensor_idx))
                    expected = tuple(out_idx != protected_idx for out_idx in MAIN_CHANNELS)
                    self.assertChannelStateTuple(states, expected)

    def test_faulty_disabled_sensor_does_not_override_other_sensor_configuration(self):
        for out_idx in MAIN_CHANNELS:
            for enabled_sensor_idx in CONTROL_SENSOR_INDICES:
                for disabled_sensor_idx in CONTROL_SENSOR_INDICES:
                    if enabled_sensor_idx == disabled_sensor_idx:
                        continue
                    with self.subTest(channel=CHANNEL_NAMES[out_idx],
                                      enabled_sensor=SENSOR_NAMES[enabled_sensor_idx],
                                      disabled_sensor=SENSOR_NAMES[disabled_sensor_idx]):
                        sensors = {
                            enabled_sensor_idx: build_control_sensor(enabled_sensor_idx,
                                                                     enabled_channels=(out_idx,),
                                                                     fault=False),
                            disabled_sensor_idx: build_control_sensor(disabled_sensor_idx,
                                                                      enabled_channels=(),
                                                                      fault=True),
                        }
                        elapsed_ms = {
                            enabled_sensor_idx: control_delay_ms(enabled_sensor_idx),
                            disabled_sensor_idx: control_delay_ms(disabled_sensor_idx),
                        }
                        states = simulate_main_channel_states(sensors, elapsed_ms=elapsed_ms)
                        self.assertChannelStateTuple(states, (True, True, True))

    def test_global_stop_turns_off_all_main_channels(self):
        states = simulate_main_channel_states(
            sensors={},
            elapsed_ms=0,
            extra_forbid_masks={out_idx: 1 << SAFETY_STOP for out_idx in MAIN_CHANNELS},
        )
        self.assertChannelStateTuple(states, (False, False, False))

    def test_all_channels_stay_on_when_all_sensors_are_normal(self):
        sensors = {
            sensor_idx: build_control_sensor(sensor_idx, enabled_channels=MAIN_CHANNELS, fault=False)
            for sensor_idx in CONTROL_SENSOR_INDICES
        }
        elapsed_ms = {sensor_idx: control_delay_ms(sensor_idx) for sensor_idx in CONTROL_SENSOR_INDICES}
        states = simulate_main_channel_states(sensors, elapsed_ms=elapsed_ms)
        self.assertChannelStateTuple(states, (True, True, True))

    def test_multiple_faults_turn_off_only_channels_with_matching_enabled_rules(self):
        for first_sensor_idx, second_sensor_idx in combinations(CONTROL_SENSOR_INDICES, 2):
            with self.subTest(first=SENSOR_NAMES[first_sensor_idx], second=SENSOR_NAMES[second_sensor_idx]):
                sensors = {
                    first_sensor_idx: build_control_sensor(first_sensor_idx, enabled_channels=(OUT_CH1,), fault=True),
                    second_sensor_idx: build_control_sensor(second_sensor_idx, enabled_channels=(OUT_CH2,), fault=True),
                }
                elapsed_ms = {
                    first_sensor_idx: control_delay_ms(first_sensor_idx),
                    second_sensor_idx: control_delay_ms(second_sensor_idx),
                }
                states = simulate_main_channel_states(sensors, elapsed_ms=elapsed_ms)
                self.assertChannelStateTuple(states, (False, False, True))

    @human_case(
        title="Потеря протока отключает CH1, но не CH2, если у CH2 flow-rule disabled",
        situation="Датчик F аварийный, правило F -> CH1 включено, а для CH2 и CH3 правила отключены.",
        steps=[
            "Создать аварийный датчик F с enabled rule только для CH1.",
            "Пересчитать состояния основных каналов после задержки.",
            "Сравнить итог для CH1, CH2 и CH3.",
        ],
        expected="Выключается только CH1, а CH2 и CH3 продолжают работать.",
    )
    def test_regression_flow_fault_turns_off_ch1_but_not_ch2_when_ch2_flow_control_is_disabled(self):
        sensors = {
            SEN_F: build_control_sensor(SEN_F, enabled_channels=(OUT_CH1,), fault=True),
        }
        states = simulate_main_channel_states(sensors, elapsed_ms=control_delay_ms(SEN_F))
        record_human_detail(self, "states", states)
        self.assertChannelStateTuple(states, (False, True, True))


class SensorFaultAndAlarmTests(unittest.TestCase):
    def test_enabled_analog_control_sensor_error_forms_auto_off(self):
        t1 = Sensor(present=False, error=True, value=math.nan)
        t1.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 70, 80,
                                    fail_safe=FailSafeMode.FORCE_OFF)
        self.assertEqual(t1.eval_ctrl(OUT_CH1), -1)

    @human_case(
        title="Отключённый аналоговый датчик полностью исключается из блокировки канала",
        situation="T1 выключен на уровне sensor.enabled=false и одновременно находится в error/missing состоянии.",
        steps=[
            "Создать выключенный T1 с error=True и отсутствующим значением.",
            "Оставить правило T1 -> CH1 включённым.",
            "Проверить eval_ctrl для CH1.",
        ],
        expected="Отключённый датчик возвращает 0 и не формирует auto-off для CH1.",
    )
    def test_disabled_analog_control_sensor_error_is_excluded(self):
        t1 = Sensor(enabled=False, present=False, error=True, value=math.nan)
        t1.ctrl[OUT_CH1] = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 70, 80,
                                    fail_safe=FailSafeMode.FORCE_OFF)
        record_human_detail(self, "sensor_state", {
            "enabled": t1.enabled,
            "present": t1.present,
            "error": t1.error,
            "value": t1.value,
        })
        self.assertEqual(t1.eval_ctrl(OUT_CH1), 0)

    def test_nan_error_raises_first_enabled_alarm_slot(self):
        t1 = Sensor(present=False, error=True, value=math.nan,
                    alarm_enabled=(False, True, True, False))
        self.assertEqual(t1.alarm_mask(), 0b0010)

    def test_disabled_sensor_alarm_mask_is_cleared(self):
        t1 = Sensor(enabled=False, present=False, error=True, value=math.nan,
                    alarm_enabled=(True, True, False, False))
        self.assertEqual(t1.alarm_mask(), 0)

    def test_percent_threshold_applies_consistently_to_adc_sensor(self):
        c = Sensor(value=2048, threshold_percent_input=True,
                   alarm_enabled=(True, False, False, False),
                   alarm_threshold=(40, 0, 0, 0),
                   alarm_is_max=(True, True, True, True))
        self.assertEqual(c.alarm_mask(), 0b0001)


class ConfirmationFsmTests(unittest.TestCase):
    def test_wer_confirmation_applies_only_to_main_channels(self):
        self.assertTrue(requires_wer_confirmation(OUT_CH1))
        self.assertTrue(requires_wer_confirmation(OUT_CH2))
        self.assertTrue(requires_wer_confirmation(OUT_CH3))
        self.assertFalse(requires_wer_confirmation(OUT_CH4))
        self.assertFalse(requires_wer_confirmation(OUT_CH5))

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
        cls.sensor_manager_h = (cls.root / "SensorManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.sensors_h = (cls.root / "Sensors.h").read_text(encoding="utf-8", errors="ignore")
        cls.confirm_h = (cls.root / "ConfirmationManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.process_h = (cls.root / "ProcessSafety.h").read_text(encoding="utf-8", errors="ignore")
        cls.output_h = (cls.root / "Output.h").read_text(encoding="utf-8", errors="ignore")
        cls.output_mgr_h = (cls.root / "OutputManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.webapi_h = (cls.root / "WebAPI.h").read_text(encoding="utf-8", errors="ignore")
        cls.wifi_mgr_h = (cls.root / "WiFiMgr.h").read_text(encoding="utf-8", errors="ignore")
        cls.main_ino = (cls.root / "RectColumn.ino").read_text(encoding="utf-8", errors="ignore")
        cls.emu_panel = (cls.root / "emupanel-v3.html").read_text(encoding="utf-8", errors="ignore")

    def define_int(self, name: str) -> int:
        m = re.search(rf"^\s*#define\s+{name}\s+(-?\d+)(?:[uUlL]*)\b", self.config_h, re.MULTILINE)
        self.assertIsNotNone(m, f"missing #define {name}")
        return int(m.group(1))

    def test_feedback_pin_map_matches_hardware_statement(self):
        self.assertEqual(self.define_int("PIN_WER_CH1"), 27)
        self.assertEqual(self.define_int("PIN_WER_CH2"), 35)
        self.assertEqual(self.define_int("PIN_WER_CH3"), 34)
        self.assertEqual(self.define_int("PIN_WER_CH4"), 36)

    def test_output_pin_map_matches_current_board_wiring(self):
        self.assertEqual(self.define_int("PIN_CH1"), 26)
        self.assertEqual(self.define_int("PIN_CH2"), 25)
        self.assertEqual(self.define_int("PIN_CH3"), 33)
        self.assertEqual(self.define_int("PIN_CH4"), 32)
        self.assertEqual(self.define_int("PIN_CH5"), 13)

    def test_valve_channels_have_longer_confirmation_timeout_than_ch1(self):
        self.assertEqual(self.define_int("RELAY_CONFIRM_TIMEOUT_CH1_MS"), 1000)
        self.assertGreater(self.define_int("RELAY_CONFIRM_TIMEOUT_CH2_MS"),
                           self.define_int("RELAY_CONFIRM_TIMEOUT_CH1_MS"))
        self.assertGreater(self.define_int("RELAY_CONFIRM_TIMEOUT_CH3_MS"),
                           self.define_int("RELAY_CONFIRM_TIMEOUT_CH1_MS"))

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

    def test_flow_loss_does_not_use_global_ch2_demand_interlock(self):
        self.assertNotIn("_flowDemandStartedMs", self.process_h)
        self.assertNotIn("valveDemandActive", self.process_h)
        self.assertNotIn("_applyFlowSafetyInterlock", self.process_h)
        self.assertNotIn("_flowEmergencyLatched", self.process_h)

    def test_main_channel_control_sensor_set_matches_sensor_manager(self):
        self.assertIn("return sensorIdx == SEN_T1 || sensorIdx == SEN_T2 ||", self.sensor_manager_h)
        self.assertIn("sensorIdx == SEN_T3 || sensorIdx == SEN_P;", self.sensor_manager_h)
        self.assertIn("sensorIdx == SEN_L || sensorIdx == SEN_F;", self.sensor_manager_h)
        self.assertIn("if (!isRuleAllowedForOutput(si, oi)) r.enabled = false;", self.sensor_manager_h)
        self.assertNotIn("sensorIdx == SEN_DT", self.sensor_manager_h)
        self.assertNotIn("sensorIdx == SEN_C", self.sensor_manager_h)
        self.assertNotIn("sensorIdx == SEN_V", self.sensor_manager_h)

    def test_main_channel_control_delays_match_matrix_assumptions(self):
        self.assertIn("l->ctrlDelayMs  = SAFETY_LEVEL_SHUTDOWN_MS;", self.sensor_manager_h)
        self.assertIn("f->ctrlDelayMs  = 5000UL;", self.sensor_manager_h)
        self.assertIn("#define SAFETY_LEVEL_SHUTDOWN_MS   (5UL * 60UL * 1000UL)", self.config_h)

    def test_sensor_stop_mode_is_configurable(self):
        self.assertRegex(self.config_h, r"#define\s+SAFETY_MODE_SENSOR_STOP\s+(false|true|0|1)")
        self.assertNotIn("_applySensorStopIfEnabled", self.process_h)

    def test_output_manager_recomputes_all_sensors_to_clear_disabled_runtime_flags(self):
        self.assertIn("if (!sen) continue;", self.output_mgr_h)
        self.assertNotIn("if (!sen || !sen->enabled) continue;", self.output_mgr_h)

    def test_sensor_runtime_state_is_reset_on_config_and_rule_changes(self):
        self.assertIn("s->resetAllControlRuntime();", self.webapi_h)
        self.assertIn("s->resetAlarmRuntime();", self.webapi_h)
        self.assertIn("s->resetControlRuntime((uint8_t)oi);", self.webapi_h)

    def test_output_manager_has_global_stop_short_circuit(self):
        self.assertIn("if (_mainStopLatched)", self.output_mgr_h)
        self.assertIn("_applyGlobalStop();", self.output_mgr_h)
        self.assertIn("invalidMeansOff = true;", self.output_mgr_h)
        self.assertIn("for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++)", self.output_mgr_h)

    def test_disabled_sensor_is_explicitly_excluded_from_eval_ctrl(self):
        self.assertIn("if (!enabled) {", self.sensors_h)
        self.assertIn("return 0;", self.sensors_h)

    def test_flow_rule_for_ch1_depends_on_ch2_and_other_channels_keep_local_gate(self):
        self.assertIn("controlGate = _flowControlGate(prevState, outIdx);", self.output_mgr_h)
        self.assertIn("return prevState[OUT_CH2];", self.output_mgr_h)
        self.assertIn("return prevState[outIdx];", self.output_mgr_h)

    def test_flow_alarm_is_gated_by_ch2_runtime_and_enabled_flow_control(self):
        self.assertIn("const bool ch2ActualOn = _om->out[OUT_CH2] && _om->out[OUT_CH2]->actualOn();", self.process_h)
        self.assertIn("const bool flowFault = fs->enabled && flowControlEnabled && ch2ActualOn && !_sm->flowActive();", self.process_h)

    def test_main_loop_skips_confirmation_and_safety_while_stop_is_active(self):
        self.assertIn("if (!outputMgr.mainStopLatched())", self.main_ino)
        self.assertIn("outputMgr.setSafetyAlarmActive(false);", self.main_ino)

    def test_wer_confirmation_is_explicitly_limited_to_ch1_ch3(self):
        self.assertIn("static inline constexpr bool requiresWerConfirmation(uint8_t outIdx)", self.config_h)
        self.assertIn("return outIdx == OUT_CH1 || outIdx == OUT_CH2 || outIdx == OUT_CH3;", self.config_h)
        self.assertIn("if (!requiresWerConfirmation(_ch[idx].outputIdx)) return false;", self.confirm_h)
        self.assertIn("if (!requiresWerConfirmation(oi)) {", self.output_mgr_h)
        self.assertIn("requiresWerConfirmation(c.outputIdx)", self.process_h)
        self.assertIn("requiresWerConfirmation(c.outputIdx)", self.main_ino)

    def test_emupanel_error_toggles_use_same_emu_set_fields_as_runtime(self):
        self.assertIn("payload.T1err = $('emu_T1err').checked;", self.emu_panel)
        self.assertIn("payload.T2err = $('emu_T2err').checked;", self.emu_panel)
        self.assertIn("payload.T3err = $('emu_T3err').checked;", self.emu_panel)
        self.assertIn("await requestJSON('/api/v1/emu/set'", self.emu_panel)
        self.assertIn("if (doc.containsKey(\"T1err\")) _emu->val.T1err = doc[\"T1err\"];", self.webapi_h)
        self.assertIn("if (doc.containsKey(\"T2err\")) _emu->val.T2err = doc[\"T2err\"];", self.webapi_h)
        self.assertIn("if (doc.containsKey(\"T3err\")) _emu->val.T3err = doc[\"T3err\"];", self.webapi_h)

    def test_manual_endpoint_uses_output_manager_runtime_path(self):
        self.assertIn("const String route = String(\"/api/v1/output/\") + _outputName(oi) + \"/manual\";", self.webapi_h)
        self.assertIn("RelayCommandResult r = _om->handleRelayCommand((uint8_t)oi, cmd, _log, _sm);", self.webapi_h)

    def test_manual_runtime_recomputes_live_forbid_state_before_accepting_command(self):
        self.assertIn("if (sm) {", self.output_mgr_h)
        self.assertIn("syncRuntimeState(*sm);", self.output_mgr_h)

    def test_emu_set_applies_inputs_to_runtime_immediately(self):
        self.assertIn("_applyEmuInputsToRuntimeNow();", self.webapi_h)
        self.assertIn("_emu->injectAll(*_sm);", self.webapi_h)
        self.assertIn("_om->syncRuntimeState(*_sm);", self.webapi_h)

    def test_final_safety_gate_clamps_physical_on_at_output_layer(self):
        self.assertIn("void setFinalOnAllowed(bool allowed)", self.output_h)
        self.assertIn("bool finalRequestedOn() const { return _resolvePhysicalRequest(_requestedOn); }", self.output_h)
        self.assertIn("_applyPhysical(_resolvePhysicalRequest(_requestedOn));", self.output_h)
        self.assertIn("out[outIdx]->setFinalOnAllowed(forbidMask == 0);", self.output_mgr_h)

    def test_wifi_scan_pauses_reconnect_and_breaks_connect_loop(self):
        self.assertIn("_pauseStaReconnect(WIFI_SCAN_RECONNECT_PAUSE_MS);", self.wifi_mgr_h)
        self.assertIn("WiFi.disconnect(false, false);", self.wifi_mgr_h)
        self.assertIn("WiFi.setAutoReconnect(false);", self.wifi_mgr_h)
        self.assertIn("reconnectPauseRemainingMs", self.wifi_mgr_h)


if __name__ == "__main__":
    unittest.main(verbosity=2)
