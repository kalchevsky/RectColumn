"""
Extended host-side coverage for RectColumn channel/sensor logic.

Run from the project root:
    python -m unittest -v tools.test_logic_scheme
    python -m unittest -v tools.test_logic_full_matrix

Live EMU API tests:
    set RECTCOLUMN_BASE_URL=http://192.168.4.1
    python -m unittest -v tools.test_api_emu_channel_sensor_matrix

Linux/macOS:
    RECTCOLUMN_BASE_URL=http://192.168.4.1 python -m unittest -v tools.test_api_emu_channel_sensor_matrix
"""

from __future__ import annotations

import math
import unittest
from dataclasses import dataclass
from pathlib import Path

try:  # pragma: no cover - import style depends on how unittest is launched
    from .human_report import human_case, record_human_detail
    from . import test_logic_scheme as scheme
except ImportError:  # pragma: no cover
    from human_report import human_case, record_human_detail  # type: ignore
    import test_logic_scheme as scheme


ANALOG_SENSOR_INDICES = (scheme.SEN_T1, scheme.SEN_T2, scheme.SEN_T3, scheme.SEN_P)
DIGITAL_SENSOR_INDICES = (scheme.SEN_L, scheme.SEN_F)
MAIN_CHANNELS = scheme.MAIN_CHANNELS
CONTROL_SENSOR_INDICES = scheme.CONTROL_SENSOR_INDICES
NON_CONTROL_SENSOR_INDICES = scheme.NON_CONTROL_SENSOR_INDICES


def analog_rule(out_idx: int, *, logic: int, enabled: bool = True) -> scheme.CtrlRule:
    return scheme.CtrlRule(
        enabled=enabled,
        out_idx=out_idx,
        logic=logic,
        min_val=70.0,
        max_val=80.0,
        fail_safe=scheme.FailSafeMode.FORCE_OFF,
    )


def analog_sensor(value: float,
                  *,
                  enabled: bool = True,
                  present: bool = True,
                  error: bool = False) -> scheme.Sensor:
    return scheme.Sensor(enabled=enabled, present=present, error=error, value=value)


@dataclass
class HoldOutputModel:
    enabled: bool = True
    actual_on: bool = False
    requested_on: bool = False
    manual_want: bool = False
    operator_hold_off: bool = False
    forbid_mask: int = 0
    want_on_mask: int = 0

    def apply_resolved_hold(self, forbid_mask: int, want_on_mask: int) -> bool:
        self.forbid_mask = forbid_mask
        self.want_on_mask = want_on_mask
        if not self.enabled:
            self.manual_want = False
            self.operator_hold_off = False
            self.requested_on = False
        elif self.forbid_mask != 0:
            self.manual_want = False
            self.operator_hold_off = False
            self.requested_on = False
        elif self.want_on_mask != 0:
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

    def set_manual_hold(self, on: bool) -> bool:
        if on and (self.forbid_mask != 0 or not self.enabled):
            return False
        self.manual_want = bool(on)
        self.operator_hold_off = not on
        self.apply_resolved_hold(self.forbid_mask, self.want_on_mask)
        return (not on) or (self.forbid_mask == 0 and self.enabled)


@dataclass
class LevelOutputModel:
    enabled: bool = True
    actual_on: bool = False
    requested_on: bool = False
    manual_want: bool = False
    forbid_mask: int = 0
    want_on_mask: int = 0

    def apply_resolved(self, forbid_mask: int, want_on_mask: int) -> bool:
        self.forbid_mask = forbid_mask
        self.want_on_mask = want_on_mask
        can_be_on = self.enabled and self.forbid_mask == 0
        wants_on = (self.want_on_mask != 0) or self.manual_want
        self.requested_on = can_be_on and wants_on
        self.actual_on = self.requested_on
        return self.actual_on

    def set_manual(self, on: bool) -> bool:
        if on and ((self.forbid_mask != 0) or not self.enabled) and not self.manual_want:
            return False
        self.manual_want = on
        return self.apply_resolved(self.forbid_mask, self.want_on_mask)


class StopControllerModel:
    def __init__(self) -> None:
        self.main_stop_latched = False
        self.main_outputs = {out_idx: HoldOutputModel(actual_on=True, requested_on=True, manual_want=True)
                             for out_idx in MAIN_CHANNELS}
        self.aux_outputs = {
            scheme.OUT_CH4: LevelOutputModel(actual_on=True, requested_on=True, manual_want=True),
            scheme.OUT_CH5: LevelOutputModel(actual_on=True, requested_on=True, manual_want=True),
        }
        self.last_forbid = {out_idx: 1 for out_idx in range(scheme.OUT_COUNT)}
        self.last_want = {out_idx: 1 for out_idx in range(scheme.OUT_COUNT)}

    def apply_global_stop(self) -> None:
        self.main_stop_latched = True
        for out_idx, output in self.main_outputs.items():
            self.last_forbid[out_idx] = 0
            self.last_want[out_idx] = 0
            output.manual_want = False
            output.operator_hold_off = False
            output.apply_resolved_hold(1 << scheme.SAFETY_STOP, 0)

    def release_stop(self) -> None:
        self.main_stop_latched = False
        for output in self.main_outputs.values():
            output.apply_resolved_hold(0, 0)

    def manual_on(self, out_idx: int) -> bool:
        if out_idx in self.main_outputs and self.main_stop_latched:
            return False
        if out_idx in self.main_outputs:
            return self.main_outputs[out_idx].set_manual_hold(True)
        return self.aux_outputs[out_idx].set_manual(True)


class AnalogSchemeMatrixTests(unittest.TestCase):
    def test_heat_mode_matrix_for_t1_t2_t3_p(self):
        for sensor_idx in ANALOG_SENSOR_INDICES:
            for out_idx in MAIN_CHANNELS:
                with self.subTest(sensor=scheme.SENSOR_NAMES[sensor_idx],
                                  channel=scheme.CHANNEL_NAMES[out_idx],
                                  logic="heat"):
                    low = analog_sensor(69.0)
                    low.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(low.eval_ctrl(out_idx), 1)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: low}, out_idx), (0, 1 << sensor_idx))

                    neutral = analog_sensor(75.0)
                    neutral.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(neutral.eval_ctrl(out_idx), 0)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: neutral}, out_idx), (0, 0))

                    high = analog_sensor(81.0)
                    high.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(high.eval_ctrl(out_idx), -1)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: high}, out_idx), (1 << sensor_idx, 0))

    def test_cool_mode_matrix_for_t1_t2_t3_p(self):
        for sensor_idx in ANALOG_SENSOR_INDICES:
            for out_idx in MAIN_CHANNELS:
                with self.subTest(sensor=scheme.SENSOR_NAMES[sensor_idx],
                                  channel=scheme.CHANNEL_NAMES[out_idx],
                                  logic="cool"):
                    high = analog_sensor(81.0)
                    high.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_COOL)
                    self.assertEqual(high.eval_ctrl(out_idx), 1)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: high}, out_idx), (0, 1 << sensor_idx))

                    neutral = analog_sensor(75.0)
                    neutral.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_COOL)
                    self.assertEqual(neutral.eval_ctrl(out_idx), 0)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: neutral}, out_idx), (0, 0))

                    low = analog_sensor(69.0)
                    low.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_COOL)
                    self.assertEqual(low.eval_ctrl(out_idx), -1)
                    self.assertEqual(scheme.aggregate_rules({sensor_idx: low}, out_idx), (1 << sensor_idx, 0))

    def test_disabled_analog_sensor_is_neutral_but_enabled_invalid_states_force_off_for_main_channels(self):
        for sensor_idx in ANALOG_SENSOR_INDICES:
            for out_idx in MAIN_CHANNELS:
                with self.subTest(sensor=scheme.SENSOR_NAMES[sensor_idx],
                                  channel=scheme.CHANNEL_NAMES[out_idx]):
                    disabled_rule_sensor = analog_sensor(85.0)
                    disabled_rule_sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT, enabled=False)
                    self.assertEqual(disabled_rule_sensor.eval_ctrl(out_idx), 0)

                    disabled_sensor = analog_sensor(85.0, enabled=False)
                    disabled_sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(disabled_sensor.eval_ctrl(out_idx), 0)

                    missing_sensor = analog_sensor(75.0, present=False)
                    missing_sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(missing_sensor.eval_ctrl(out_idx), -1)

                    error_sensor = analog_sensor(75.0, error=True)
                    error_sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(error_sensor.eval_ctrl(out_idx), -1)

                    nan_sensor = analog_sensor(math.nan)
                    nan_sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
                    self.assertEqual(nan_sensor.eval_ctrl(out_idx), -1)


class DigitalSchemeMatrixTests(unittest.TestCase):
    def test_l_matrix_for_main_channels(self):
        for out_idx in MAIN_CHANNELS:
            with self.subTest(channel=scheme.CHANNEL_NAMES[out_idx]):
                disabled_rule = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_L, disabled_rule, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                    0,
                )

                neutral = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(out_idx,), fault=False)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_L, neutral, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                    0,
                )

                before_delay = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(out_idx,), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_L, before_delay, out_idx, scheme.control_delay_ms(scheme.SEN_L) - 1),
                    0,
                )

                after_delay = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(out_idx,), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_L, after_delay, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                    -1,
                )
                self.assertEqual(
                    scheme.aggregate_rules_timed({scheme.SEN_L: after_delay}, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                    (1 << scheme.SEN_L, 0),
                )

                disabled_sensor = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(out_idx,), fault=True)
                disabled_sensor.enabled = False
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_L, disabled_sensor, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                    0,
                )

                for present, error, value in ((False, False, 0.0), (True, True, 0.0), (True, False, math.nan)):
                    invalid = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(out_idx,), fault=False)
                    invalid.present = present
                    invalid.error = error
                    invalid.value = value
                    self.assertEqual(
                        scheme.eval_ctrl_timed(scheme.SEN_L, invalid, out_idx, scheme.control_delay_ms(scheme.SEN_L)),
                        -1,
                    )

    def test_f_matrix_for_main_channels(self):
        for out_idx in MAIN_CHANNELS:
            with self.subTest(channel=scheme.CHANNEL_NAMES[out_idx]):
                disabled_rule = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, disabled_rule, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=True),
                    0,
                )

                neutral = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=False)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, neutral, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=True),
                    0,
                )

                gated_off = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, gated_off, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=False),
                    0,
                )

                before_delay = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, before_delay, out_idx, scheme.control_delay_ms(scheme.SEN_F) - 1, prev_output_on=True),
                    0,
                )

                after_delay = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=True)
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, after_delay, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=True),
                    -1,
                )

                disabled_sensor = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=True)
                disabled_sensor.enabled = False
                self.assertEqual(
                    scheme.eval_ctrl_timed(scheme.SEN_F, disabled_sensor, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=True),
                    0,
                )

                for present, error, value in ((False, False, 0.0), (True, True, 0.0), (True, False, math.nan)):
                    invalid = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(out_idx,), fault=False)
                    invalid.present = present
                    invalid.error = error
                    invalid.value = value
                    self.assertEqual(
                        scheme.eval_ctrl_timed(scheme.SEN_F, invalid, out_idx, scheme.control_delay_ms(scheme.SEN_F), prev_output_on=True),
                        -1,
                    )

    @human_case(
        title="Нет протока отключает CH1 только когда CH2 уже включён",
        situation="Для CH1 включено правило по F, проток отсутствует, а gate для CH1 зависит от состояния CH2.",
        steps=[
            "Подготовить датчик F с enabled rule на CH1.",
            "Проверить результат после полной задержки при linked_output_on=True.",
            "Проверить forbid mask для CH1.",
        ],
        expected="После истечения задержки F формирует auto-off для CH1, если CH2 считается включённым.",
    )
    def test_f_loss_with_ch2_on_after_delay_turns_off_ch1(self):
        flow = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(scheme.OUT_CH1,), fault=True)
        delay = scheme.control_delay_ms(scheme.SEN_F)
        record_human_detail(self, "flow_rule", flow.ctrl[scheme.OUT_CH1].__dict__)
        record_human_detail(self, "delay_ms", delay)
        self.assertEqual(
            scheme.eval_ctrl_timed(scheme.SEN_F, flow, scheme.OUT_CH1, delay, linked_output_on=True),
            -1,
        )
        self.assertEqual(
            scheme.aggregate_rules_timed({scheme.SEN_F: flow}, scheme.OUT_CH1, delay, linked_output_on=True),
            (1 << scheme.SEN_F, 0),
        )

    @human_case(
        title="Нет протока не влияет на CH1, пока CH2 выключен",
        situation="Для CH1 включено правило по F, но проток должен блокировать канал только при включённом CH2.",
        steps=[
            "Подготовить датчик F с enabled rule на CH1.",
            "Оставить linked_output_on=False.",
            "Проверить eval_ctrl_timed и aggregate_rules_timed.",
        ],
        expected="F остаётся нейтральным для CH1, если CH2 выключен.",
    )
    def test_f_loss_with_ch2_off_is_neutral_for_ch1(self):
        flow = scheme.build_control_sensor(scheme.SEN_F, enabled_channels=(scheme.OUT_CH1,), fault=True)
        delay = scheme.control_delay_ms(scheme.SEN_F)
        record_human_detail(self, "flow_rule", flow.ctrl[scheme.OUT_CH1].__dict__)
        record_human_detail(self, "delay_ms", delay)
        self.assertEqual(
            scheme.eval_ctrl_timed(scheme.SEN_F, flow, scheme.OUT_CH1, delay, linked_output_on=False),
            0,
        )
        self.assertEqual(
            scheme.aggregate_rules_timed({scheme.SEN_F: flow}, scheme.OUT_CH1, delay, linked_output_on=False),
            (0, 0),
        )

    def test_f_isolation_combinations(self):
        combinations = (
            (),
            (scheme.OUT_CH1,),
            (scheme.OUT_CH2,),
            (scheme.OUT_CH3,),
            (scheme.OUT_CH1, scheme.OUT_CH2),
            (scheme.OUT_CH1, scheme.OUT_CH3),
            (scheme.OUT_CH2, scheme.OUT_CH3),
            MAIN_CHANNELS,
        )
        for enabled_channels in combinations:
            with self.subTest(enabled=tuple(scheme.CHANNEL_NAMES[idx] for idx in enabled_channels)):
                sensors = {
                    scheme.SEN_F: scheme.build_control_sensor(scheme.SEN_F, enabled_channels=enabled_channels, fault=True),
                }
                states = scheme.simulate_main_channel_states(
                    sensors,
                    elapsed_ms=scheme.control_delay_ms(scheme.SEN_F),
                    initial_on=True,
                )
                expected = tuple(out_idx not in enabled_channels for out_idx in MAIN_CHANNELS)
                self.assertEqual(scheme.state_tuple(states), expected)


class MainOutputPriorityTests(unittest.TestCase):
    def _assert_enabled_sensor_error_blocks_manual_on(self, sensor_idx: int, out_idx: int) -> None:
        sensor = analog_sensor(75.0, error=True)
        sensor.ctrl[out_idx] = analog_rule(out_idx, logic=scheme.LOGIC_HEAT)
        forbid, want = scheme.aggregate_rules({sensor_idx: sensor}, out_idx)
        self.assertNotEqual(forbid, 0)
        self.assertEqual(want, 0)

        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.apply_resolved_hold(forbid, want)
        self.assertFalse(out.set_manual_hold(True))
        self.assertFalse(out.actual_on)
        self.assertFalse(out.manual_want)

    def test_auto_off_priority_over_auto_on(self):
        out = HoldOutputModel(actual_on=True, requested_on=True)
        self.assertFalse(out.apply_resolved_hold(1, 1))
        self.assertFalse(out.manual_want)
        self.assertFalse(out.operator_hold_off)

    def test_auto_off_priority_over_manual_on(self):
        out = HoldOutputModel(actual_on=True, requested_on=True, manual_want=True)
        self.assertFalse(out.apply_resolved_hold(1, 0))
        self.assertFalse(out.manual_want)

    def test_auto_on_priority_over_manual_off(self):
        out = HoldOutputModel(actual_on=True, requested_on=True, operator_hold_off=True)
        self.assertTrue(out.apply_resolved_hold(0, 1))
        self.assertFalse(out.operator_hold_off)

    def test_manual_off_when_auto_neutral(self):
        out = HoldOutputModel(actual_on=True, requested_on=True)
        self.assertTrue(out.set_manual_hold(False))
        self.assertFalse(out.actual_on)
        self.assertFalse(out.operator_hold_off)

    def test_manual_on_is_applied_in_neutral_zone(self):
        out = HoldOutputModel(actual_on=False, requested_on=False)
        self.assertTrue(out.set_manual_hold(True))
        self.assertTrue(out.actual_on)
        self.assertFalse(out.manual_want)

    def test_manual_hold_when_command_is_neutral(self):
        for initial in (False, True):
            with self.subTest(initial=initial):
                out = HoldOutputModel(actual_on=initial, requested_on=initial)
                self.assertEqual(out.apply_resolved_hold(0, 0), initial)

    def test_auto_off_clears_manual_state(self):
        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.manual_want = True
        out.operator_hold_off = True
        out.apply_resolved_hold(1, 0)
        self.assertFalse(out.actual_on)
        self.assertFalse(out.manual_want)
        self.assertFalse(out.operator_hold_off)

    def test_auto_on_clears_manual_state(self):
        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.manual_want = True
        out.operator_hold_off = True
        out.apply_resolved_hold(0, 1)
        self.assertTrue(out.actual_on)
        self.assertFalse(out.manual_want)
        self.assertFalse(out.operator_hold_off)

    def test_clearing_forbid_does_not_replay_old_manual_on(self):
        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.manual_want = True
        out.apply_resolved_hold(1, 0)
        out.apply_resolved_hold(0, 0)
        self.assertFalse(out.actual_on)
        self.assertFalse(out.manual_want)

    @human_case(
        title="Отключённый датчик не блокирует ручное включение канала",
        situation="Правило защиты по уровню включено для CH1, но сам датчик L выключен и формально находится в аварийном состоянии.",
        steps=[
            "Создать датчик L с ruleEnabled=true для CH1.",
            "Выключить sensor.enabled.",
            "Пересчитать forbid/want masks.",
            "Попробовать ручное включение CH1 через hold-resolver.",
        ],
        expected="forbid=0, want=0, manual ON для CH1 разрешён и канал включается.",
    )
    def test_disabled_sensor_does_not_block_manual_on(self):
        sensor = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(scheme.OUT_CH1,), fault=True)
        sensor.enabled = False
        forbid, want = scheme.aggregate_rules_timed(
            {scheme.SEN_L: sensor},
            scheme.OUT_CH1,
            scheme.control_delay_ms(scheme.SEN_L),
        )
        record_human_detail(self, "runtime_masks", {"forbid": forbid, "want": want})
        self.assertEqual((forbid, want), (0, 0))

        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.apply_resolved_hold(forbid, want)
        record_human_detail(self, "output_before_manual", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
            "manual_want": out.manual_want,
            "operator_hold_off": out.operator_hold_off,
        })
        self.assertTrue(out.set_manual_hold(True))
        record_human_detail(self, "output_after_manual", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
            "manual_want": out.manual_want,
            "operator_hold_off": out.operator_hold_off,
        })
        self.assertTrue(out.actual_on)

    @human_case(
        title="Активный auto-off запрещает manual ON",
        situation="Датчик L включён, ruleEnabled=true для CH1 и вход уже находится в аварийном состоянии.",
        steps=[
            "Создать аварийный датчик L для CH1.",
            "Собрать forbid mask после задержки.",
            "Передать маски в hold-resolver.",
            "Попробовать manual ON для CH1.",
        ],
        expected="manual ON отклоняется, CH1 остаётся выключенным, manual_want не сохраняется.",
    )
    def test_enabled_sensor_with_active_auto_off_blocks_manual_on(self):
        sensor = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(scheme.OUT_CH1,), fault=True)
        forbid, want = scheme.aggregate_rules_timed(
            {scheme.SEN_L: sensor},
            scheme.OUT_CH1,
            scheme.control_delay_ms(scheme.SEN_L),
        )
        record_human_detail(self, "runtime_masks", {"forbid": forbid, "want": want})
        self.assertNotEqual(forbid, 0)
        self.assertEqual(want, 0)

        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.apply_resolved_hold(forbid, want)
        record_human_detail(self, "output_after_manual_attempt", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
            "manual_want": out.manual_want,
            "operator_hold_off": out.operator_hold_off,
        })
        self.assertFalse(out.set_manual_hold(True))
        self.assertFalse(out.actual_on)
        self.assertFalse(out.manual_want)

    @human_case(
        title="Ошибка активного датчика выключает уже включённый канал",
        situation="T1 управляет CH1, канал уже включён, затем датчик T1 уходит в error.",
        steps=[
            "Создать sensor error для T1 при ruleEnabled=true.",
            "Собрать forbid mask для CH1.",
            "Применить маски к уже включённому hold-resolver.",
        ],
        expected="CH1 выключается, actual_on=false и причина автоотключения исходит от T1.",
    )
    def test_active_sensor_error_turns_already_enabled_channel_off(self):
        sensor = analog_sensor(75.0, error=True)
        sensor.ctrl[scheme.OUT_CH1] = analog_rule(scheme.OUT_CH1, logic=scheme.LOGIC_HEAT)
        forbid, want = scheme.aggregate_rules({scheme.SEN_T1: sensor}, scheme.OUT_CH1)
        record_human_detail(self, "runtime_masks", {"forbid": forbid, "want": want})
        self.assertNotEqual(forbid, 0)
        self.assertEqual(want, 0)

        out = HoldOutputModel(actual_on=True, requested_on=True)
        record_human_detail(self, "output_before_apply", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
        })
        self.assertFalse(out.apply_resolved_hold(forbid, want))
        record_human_detail(self, "output_after_apply", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
        })
        self.assertFalse(out.actual_on)

    @human_case(
        title="Manual ON во время auto-off поглощается и не воспроизводится позже",
        situation="CH1 уже запрещён активной защитой по уровню, оператор пытается включить его вручную, а затем запрет снимается.",
        steps=[
            "Создать активный auto-off по L для CH1.",
            "Попробовать manual ON и получить отказ.",
            "Снять forbid mask и пересчитать hold-resolver.",
        ],
        expected="После снятия запрета CH1 не включается сам, manual_want остаётся очищенным.",
    )
    def test_manual_on_during_auto_off_is_consumed_and_not_replayed(self):
        sensor = scheme.build_control_sensor(scheme.SEN_L, enabled_channels=(scheme.OUT_CH1,), fault=True)
        forbid, want = scheme.aggregate_rules_timed(
            {scheme.SEN_L: sensor},
            scheme.OUT_CH1,
            scheme.control_delay_ms(scheme.SEN_L),
        )
        record_human_detail(self, "runtime_masks_before_clear", {"forbid": forbid, "want": want})

        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.apply_resolved_hold(forbid, want)
        self.assertFalse(out.set_manual_hold(True))
        out.apply_resolved_hold(0, 0)
        record_human_detail(self, "output_after_clear", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
            "manual_want": out.manual_want,
            "operator_hold_off": out.operator_hold_off,
        })
        self.assertFalse(out.actual_on)
        self.assertFalse(out.manual_want)

    @human_case(
        title="Manual OFF остаётся разрешённым во время auto-off",
        situation="T1 держит CH1 в auto-off из-за ошибки датчика, но оператор всё равно может отправить команду выключения.",
        steps=[
            "Создать sensor error для T1 при активном правиле CH1 <- T1.",
            "Применить forbid mask к hold-resolver.",
            "Отправить manual OFF в модели.",
        ],
        expected="Команда manual OFF принимается и CH1 остаётся выключенным без дополнительных побочных эффектов.",
    )
    def test_manual_off_remains_allowed_during_auto_off(self):
        sensor = analog_sensor(75.0, error=True)
        sensor.ctrl[scheme.OUT_CH1] = analog_rule(scheme.OUT_CH1, logic=scheme.LOGIC_HEAT)
        forbid, want = scheme.aggregate_rules({scheme.SEN_T1: sensor}, scheme.OUT_CH1)
        record_human_detail(self, "runtime_masks", {"forbid": forbid, "want": want})

        out = HoldOutputModel(actual_on=False, requested_on=False)
        out.apply_resolved_hold(forbid, want)
        record_human_detail(self, "output_after_manual_off", {
            "actual_on": out.actual_on,
            "requested_on": out.requested_on,
            "manual_want": out.manual_want,
            "operator_hold_off": out.operator_hold_off,
        })
        self.assertTrue(out.set_manual_hold(False))
        self.assertFalse(out.actual_on)

    def test_enabled_t1_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on(scheme.SEN_T1, scheme.OUT_CH1)

    def test_enabled_t2_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on(scheme.SEN_T2, scheme.OUT_CH2)

    def test_enabled_t3_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on(scheme.SEN_T3, scheme.OUT_CH3)


class StopAndIsolationTests(unittest.TestCase):
    def test_global_stop_turns_off_only_main_outputs_and_clears_main_masks(self):
        ctrl = StopControllerModel()
        ctrl.apply_global_stop()

        self.assertTrue(ctrl.main_stop_latched)
        for out in ctrl.main_outputs.values():
            self.assertFalse(out.actual_on)
            self.assertFalse(out.manual_want)
            self.assertFalse(out.operator_hold_off)
        self.assertTrue(ctrl.aux_outputs[scheme.OUT_CH4].actual_on)
        self.assertTrue(ctrl.aux_outputs[scheme.OUT_CH5].actual_on)
        for out_idx in MAIN_CHANNELS:
            self.assertEqual(ctrl.last_forbid[out_idx], 0)
            self.assertEqual(ctrl.last_want[out_idx], 0)

    def test_stop_clears_pending_manual_for_main_outputs(self):
        ctrl = StopControllerModel()
        for out in ctrl.main_outputs.values():
            out.manual_want = True
            out.operator_hold_off = True
        ctrl.apply_global_stop()
        for out in ctrl.main_outputs.values():
            self.assertFalse(out.manual_want)
            self.assertFalse(out.operator_hold_off)

    def test_manual_on_for_main_outputs_is_blocked_while_stop_is_active(self):
        ctrl = StopControllerModel()
        ctrl.apply_global_stop()
        for out_idx in MAIN_CHANNELS:
            self.assertFalse(ctrl.manual_on(out_idx))

    def test_aux_outputs_are_not_implicitly_blocked_by_main_stop(self):
        ctrl = StopControllerModel()
        ctrl.apply_global_stop()
        self.assertTrue(ctrl.manual_on(scheme.OUT_CH4))
        self.assertTrue(ctrl.manual_on(scheme.OUT_CH5))

    def test_release_stop_does_not_restore_old_manual_main_requests(self):
        ctrl = StopControllerModel()
        ctrl.apply_global_stop()
        ctrl.release_stop()
        self.assertFalse(ctrl.main_stop_latched)
        for out in ctrl.main_outputs.values():
            self.assertFalse(out.actual_on)
            self.assertFalse(out.manual_want)

    def test_sensor_isolation_for_all_control_sensors(self):
        combinations = (
            (),
            (scheme.OUT_CH1,),
            (scheme.OUT_CH2,),
            (scheme.OUT_CH3,),
            (scheme.OUT_CH1, scheme.OUT_CH2),
            (scheme.OUT_CH1, scheme.OUT_CH3),
            (scheme.OUT_CH2, scheme.OUT_CH3),
            MAIN_CHANNELS,
        )
        for sensor_idx in CONTROL_SENSOR_INDICES:
            for enabled_channels in combinations:
                with self.subTest(sensor=scheme.SENSOR_NAMES[sensor_idx],
                                  enabled=tuple(scheme.CHANNEL_NAMES[idx] for idx in enabled_channels)):
                    sensors = {
                        sensor_idx: scheme.build_control_sensor(sensor_idx, enabled_channels=enabled_channels, fault=True),
                    }
                    states = scheme.simulate_main_channel_states(
                        sensors,
                        elapsed_ms=scheme.control_delay_ms(sensor_idx),
                        initial_on=True,
                    )
                    expected = tuple(out_idx not in enabled_channels for out_idx in MAIN_CHANNELS)
                    self.assertEqual(scheme.state_tuple(states), expected)


class ExtendedSensorsAndAuxOutputsTests(unittest.TestCase):
    def test_aux_outputs_do_not_require_wer_confirmation(self):
        self.assertFalse(scheme.requires_wer_confirmation(scheme.OUT_CH4))
        self.assertFalse(scheme.requires_wer_confirmation(scheme.OUT_CH5))

    def test_dt_c_v_can_drive_ch4_and_ch5_with_level_resolver(self):
        for sensor_idx in NON_CONTROL_SENSOR_INDICES:
            for out_idx in (scheme.OUT_CH4, scheme.OUT_CH5):
                with self.subTest(sensor=scheme.SENSOR_NAMES[sensor_idx], channel=out_idx):
                    sensor = analog_sensor(10.0)
                    sensor.ctrl[out_idx] = scheme.CtrlRule(
                        enabled=True,
                        out_idx=out_idx,
                        logic=scheme.LOGIC_HEAT,
                        min_val=20.0,
                        max_val=80.0,
                        fail_safe=scheme.FailSafeMode.NEUTRAL,
                    )
                    cmd = sensor.eval_ctrl(out_idx)
                    self.assertEqual(cmd, 1)

                    out = LevelOutputModel(actual_on=False, requested_on=False)
                    want = (1 << sensor_idx) if cmd == 1 else 0
                    forbid = (1 << sensor_idx) if cmd == -1 else 0
                    self.assertTrue(out.apply_resolved(forbid, want))

    def test_ch4_ch5_level_resolver_turns_on_and_off_from_masks(self):
        out = LevelOutputModel(actual_on=False, requested_on=False)
        self.assertTrue(out.apply_resolved(0, 1))
        self.assertFalse(out.apply_resolved(0, 0))
        self.assertFalse(out.apply_resolved(1, 1))
        self.assertFalse(out.set_manual(True))

    def test_aux_output_activity_does_not_change_main_channel_state(self):
        ch1 = HoldOutputModel(actual_on=True, requested_on=True)
        ch4 = LevelOutputModel(actual_on=False, requested_on=False)
        ch5 = LevelOutputModel(actual_on=False, requested_on=False)
        ch4.apply_resolved(0, 1)
        ch5.apply_resolved(0, 1)
        self.assertTrue(ch1.actual_on)
        self.assertTrue(ch4.actual_on)
        self.assertTrue(ch5.actual_on)


class ConfirmationAndSafetyModelTests(unittest.TestCase):
    def test_reset_clears_fault_but_persistent_bad_feedback_recreates_it(self):
        fsm = scheme.ConfirmationFSM()
        fsm.begin_on(now_ms=0, feedback_on=True)
        self.assertEqual(fsm.state, scheme.RelayState.FAULT_STUCK_HIGH_BEFORE_ON)
        self.assertTrue(fsm.reset(feedback_on=False))
        fsm.begin_on(now_ms=100, feedback_on=True)
        self.assertEqual(fsm.state, scheme.RelayState.FAULT_STUCK_HIGH_BEFORE_ON)

    def test_no_on_confirm_fault_is_channel_local_for_main_outputs(self):
        ch1 = scheme.ConfirmationFSM(timeout_ms=1000)
        ch2 = scheme.ConfirmationFSM(timeout_ms=1000)
        ch1.begin_on(now_ms=0, feedback_on=False)
        ch2.begin_on(now_ms=0, feedback_on=False)
        self.assertEqual(ch1.loop(now_ms=1000, feedback_on=False), scheme.RelayState.FAULT_NO_CONFIRM)
        self.assertEqual(ch2.loop(now_ms=500, feedback_on=True), scheme.RelayState.ON)


class FullMatrixSourceGuardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = Path(__file__).resolve().parents[1]
        cls.sensor_manager_h = (cls.root / "SensorManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.sensors_h = (cls.root / "Sensors.h").read_text(encoding="utf-8", errors="ignore")
        cls.webapi_h = (cls.root / "WebAPI.h").read_text(encoding="utf-8", errors="ignore")
        cls.storage_h = (cls.root / "Storage.h").read_text(encoding="utf-8", errors="ignore")
        cls.output_h = (cls.root / "Output.h").read_text(encoding="utf-8", errors="ignore")
        cls.output_manager_h = (cls.root / "OutputManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.process_h = (cls.root / "ProcessSafety.h").read_text(encoding="utf-8", errors="ignore")
        cls.remote_notifier_h = (cls.root / "RemoteNotifier.h").read_text(encoding="utf-8", errors="ignore")
        cls.confirm_h = (cls.root / "ConfirmationManager.h").read_text(encoding="utf-8", errors="ignore")
        cls.config_h = (cls.root / "config.h").read_text(encoding="utf-8", errors="ignore")
        cls.rect_column_ino = (cls.root / "RectColumn.ino").read_text(encoding="utf-8", errors="ignore")

    def test_digital_off_only_normalization_is_fixed_for_l_and_f(self):
        self.assertIn("r.logic  = LOGIC_COOL;", self.sensor_manager_h)
        self.assertIn("r.minVal = 0.5f;", self.sensor_manager_h)
        self.assertIn("r.maxVal = 2.0f;", self.sensor_manager_h)
        self.assertIn("const bool keepEnabled = r.enabled;", self.sensor_manager_h)
        self.assertIn("r.enabled = keepEnabled;", self.sensor_manager_h)

    def test_dt_is_allowed_for_main_channels_but_c_and_v_remain_blocked(self):
        self.assertIn("if (!isMainOutputIndex(outIdx)) return true;", self.sensor_manager_h)
        self.assertIn("return isSchemeControlSensorIndex(sensorIdx);", self.sensor_manager_h)
        self.assertIn("sensorIdx == SEN_DT", self.sensor_manager_h)
        self.assertNotIn("sensorIdx == SEN_C", self.sensor_manager_h)
        self.assertNotIn("sensorIdx == SEN_V", self.sensor_manager_h)

    def test_web_api_keeps_main_channel_allowlist(self):
        self.assertIn("sensor is not allowed to control CH1..CH3 in scheme mode", self.webapi_h)
        self.assertIn("SensorManager::isRuleAllowedForOutput((uint8_t)si, (uint8_t)oi)", self.webapi_h)
        self.assertIn("_sm->normalizeSchemeControlRules();", self.webapi_h)

    def test_storage_sanitize_disables_restored_extended_rules_for_main_outputs(self):
        self.assertIn("if (!SensorManager::isRuleAllowedForOutput((uint8_t)si, (uint8_t)oi)) {", self.storage_h)
        self.assertIn("r.enabled = false;", self.storage_h)
        self.assertIn("sm.normalizeDigitalOffOnlyRules();", self.storage_h)
        self.assertIn("sm.normalizeSchemeControlRules();", self.storage_h)

    def test_main_outputs_use_hold_resolver_and_aux_outputs_use_level_resolver(self):
        self.assertIn("void applyResolved(uint32_t forbidMask, uint32_t wantOnMask)", self.output_h)
        self.assertIn("void applyResolvedHold(uint32_t forbidMask, uint32_t wantOnMask)", self.output_h)
        self.assertIn("void setFinalOnAllowed(bool allowed)", self.output_h)
        self.assertIn("bool finalRequestedOn() const { return _resolvePhysicalRequest(_requestedOn); }", self.output_h)
        self.assertIn("_applyPhysical(_resolvePhysicalRequest(_requestedOn));", self.output_h)
        self.assertIn("void _applyCurrentMain(uint8_t outIdx)", self.output_manager_h)
        self.assertIn("out[outIdx]->applyResolvedHold(forbidMask, wantMask);", self.output_manager_h)
        self.assertIn("out[outIdx]->setFinalOnAllowed(forbidMask == 0);", self.output_manager_h)
        self.assertIn("out[outIdx]->setManualHold(true);", self.output_manager_h)
        self.assertIn("out[outIdx]->forceOff(true);", self.output_manager_h)
        self.assertIn("out[outIdx]->applyResolved(_effectiveForbidMask(outIdx), _lastWant[outIdx]);", self.output_manager_h)

    def test_global_stop_loop_is_limited_to_main_channels(self):
        self.assertIn("for (uint8_t oi = OUT_CH1; oi <= OUT_CH3; oi++)", self.output_manager_h)

    def test_flow_gate_uses_ch2_for_ch1_and_keeps_local_gate_for_other_channels(self):
        self.assertIn("controlGate = _flowControlGate(prevState, outIdx);", self.output_manager_h)
        self.assertIn("out[idx]->manualWant()", self.output_manager_h)
        self.assertIn("(_lastWant[idx] != 0)", self.output_manager_h)
        self.assertIn("return wants(OUT_CH2);", self.output_manager_h)
        self.assertIn("return wants(outIdx);", self.output_manager_h)

    def test_mute_clears_sound_outputs_without_touching_main_channels(self):
        self.assertIn("out[OUT_CH4]->setBellPatternActive(false);", self.output_manager_h)
        self.assertIn("out[OUT_CH5]->applyResolved(_effectiveForbidMask(OUT_CH5), want);", self.output_manager_h)

    def test_flow_loss_is_not_a_global_stop_or_global_flow_forbid(self):
        self.assertNotIn("_applyFlowSafetyInterlock", self.process_h)
        self.assertNotIn("_flowEmergencyLatched", self.process_h)
        self.assertNotIn("_flowDemandStartedMs", self.process_h)
        self.assertNotIn("setMainStopLatched(true);", self.process_h[self.process_h.find("void _handleFlowLoss"):self.process_h.find("void _handlePressureHigh")])

    def test_notifier_uses_unacked_alarms_and_resets_baseline_on_config_change(self):
        self.assertIn("_om->unackedAlarmMaskFor(*_sm, (uint8_t)si);", self.remote_notifier_h)
        self.assertIn("_snapshotCurrentAlarms();", self.remote_notifier_h)
        self.assertNotIn("return s->alarmMask();", self.remote_notifier_h)
        self.assertIn("if (bits & SENSOR_LOST_ALARM_MASK) return SENSOR_LOST_ALARM_BIT;", self.remote_notifier_h)
        self.assertIn("return s->sensorLostNotice();", self.remote_notifier_h)

    def test_notifier_defers_failure_logging_from_background_task(self):
        self.assertIn("_flushQueuedFailure();", self.remote_notifier_h)
        self.assertIn("self->_queueSendFailure(err.c_str());", self.remote_notifier_h)
        self.assertIn("void _queueSendFailure(const char* err)", self.remote_notifier_h)
        self.assertIn("void _flushQueuedFailure()", self.remote_notifier_h)
        self.assertNotIn("self->_logSendFailure(err);", self.remote_notifier_h)

    def test_state_api_keeps_active_alarm_reason_list(self):
        self.assertIn("activeAlarmReasons", self.webapi_h)

    def test_level_and_pressure_alarms_do_not_add_channel_safety_forbids(self):
        self.assertNotIn("_om->setSafetyForbid(OUT_CH1, RULEIDX_SAFETY_LEVEL, true);", self.process_h)
        self.assertNotIn("_om->setSafetyForbid(OUT_CH2, RULEIDX_SAFETY_LEVEL, true);", self.process_h)
        self.assertNotIn("_om->setSafetyForbid(OUT_CH1, RULEIDX_SAFETY_PRESSURE, true);", self.process_h)
        self.assertNotIn("_om->setSafetyForbid(OUT_CH2, RULEIDX_SAFETY_PRESSURE, true);", self.process_h)
        self.assertNotIn("_om->setSafetyForbid(OUT_CH3, RULEIDX_SAFETY_PRESSURE, true);", self.process_h)

    def test_wer_fault_is_logged_without_safety_lockout(self):
        self.assertIn("requiresWerConfirmation(c.outputIdx) && c.faultLatched", self.process_h)
        self.assertIn("Только индикация, без отключения канала.", self.process_h)
        self.assertNotIn("_om->setSafetyForbid(c.outputIdx, RULEIDX_SAFETY_WER, true);", self.process_h)
        self.assertNotIn("_om->out[c.outputIdx]->forceOff(true);", self.process_h)
        self.assertIn("if (!requiresWerConfirmation(_ch[idx].outputIdx)) return false;", self.confirm_h)

    def test_notifier_uses_single_worker_queue_and_status_endpoint(self):
        self.assertIn("QueueHandle_t _queue = nullptr;", self.remote_notifier_h)
        self.assertIn("TaskHandle_t _worker = nullptr;", self.remote_notifier_h)
        self.assertIn("xTaskCreatePinnedToCore(", self.remote_notifier_h)
        self.assertIn("16384,", self.remote_notifier_h)
        self.assertIn("xQueueSend(_queue, &item, 0)", self.remote_notifier_h)
        self.assertIn("_server.on(\"/api/v1/notify/status\", HTTP_GET", self.webapi_h)
        self.assertIn("notify[\"droppedCount\"]", self.webapi_h)

    def test_sensor_enable_warmup_and_ntfy_redirect_guard_are_present(self):
        self.assertIn("startEnableWarmup(3000);", self.webapi_h)
        self.assertIn("so[\"warmup\"] = s->isInEnableWarmup();", self.webapi_h)
        self.assertIn("if (isInEnableWarmup()) {", self.sensors_h)
        self.assertIn("sensorErrorLatched", self.webapi_h)
        self.assertIn("sensorLostNotice", self.webapi_h)
        self.assertIn("http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);", self.remote_notifier_h)
        self.assertIn("http.useHTTP10(true);", self.remote_notifier_h)
        self.assertIn("server returned redirect (HTTPS required?); use a publish URL that does not redirect", self.remote_notifier_h)

    def test_aux_outputs_skip_wer_confirmation_and_timeout_path(self):
        self.assertIn("static inline constexpr bool requiresWerConfirmation(uint8_t outIdx)", self.config_h)
        self.assertIn("if (!requiresWerConfirmation(_ch[idx].outputIdx)) {", self.confirm_h)
        self.assertIn("if (!requiresWerConfirmation(oi)) {", self.output_manager_h)
        self.assertIn("return requiresWerConfirmation(outIdx);", self.output_manager_h)

    def test_web_runtime_syncs_emu_inputs_and_manual_commands_before_final_gate(self):
        self.assertIn("_applyEmuInputsToRuntimeNow();", self.webapi_h)
        self.assertIn("_emu->injectAll(*_sm);", self.webapi_h)
        self.assertIn("_om->syncRuntimeState(*_sm);", self.webapi_h)
        self.assertIn("if (sm) {", self.output_manager_h)
        self.assertIn("syncRuntimeState(*sm);", self.output_manager_h)

    def test_relay_off_diagnostics_include_source_and_sensor_context(self):
        self.assertIn("RELAY_OFF source=", self.output_manager_h)
        self.assertIn("_logRelayOff(log, sm, (uint8_t)i);", self.output_manager_h)
        self.assertIn("ctrlDelayMs=", self.output_manager_h)
        self.assertIn("elapsedMs=", self.output_manager_h)
        self.assertIn("ruleState=", self.output_manager_h)
        self.assertIn("_formatSensorState(sm.s[SEN_F])", self.output_manager_h)
        self.assertIn("_formatSensorState(sm.s[SEN_P])", self.output_manager_h)

    def test_l_and_f_default_timeouts_match_current_sources(self):
        self.assertIn("l->ctrlDelayMs  = SAFETY_LEVEL_SHUTDOWN_MS;", self.sensor_manager_h)
        self.assertIn("f->ctrlDelayMs  = 5000UL;", self.sensor_manager_h)
        self.assertIn("#define SAFETY_LEVEL_SHUTDOWN_MS   (5UL * 60UL * 1000UL)", self.config_h)
        self.assertIn("#define SENSOR_LOST_TIMEOUT_MS       3000UL", self.config_h)
        self.assertIn("#define SENSOR_LOST_TIMEOUT_DS_MS    5000UL", self.config_h)
        self.assertIn("#define SENSOR_HEALTHY_HYSTERESIS_MS 5000UL", self.config_h)
        self.assertIn("#define PRESSURE_SANITY_MIN_HPA      300.0f", self.config_h)
        self.assertIn("#define PRESSURE_SANITY_MAX_HPA      1100.0f", self.config_h)

    # === FIX BUG 1: pressure sensor phantom value handled ===
    def test_pressure_sensor_rejects_out_of_range_reading_immediately(self):
        # Out-of-range values (phantom ADC noise when disconnected) must be
        # rejected by setting value=NAN and triggering sensor error on first read.
        self.assertIn("if (value < PRESSURE_SANITY_MIN_HPA || value > PRESSURE_SANITY_MAX_HPA)", self.sensors_h)
        self.assertIn("markSensorFault(SENSOR_ERR_OUT_OF_RANGE, now, true);", self.sensors_h)
        # The value is set to NAN before markSensorFault so the phantom reading
        # (e.g. 1718.79) never enters the sensor value field.
        self.assertIn("value = NAN;\n            hwLimited = false;\n            markSensorFault(SENSOR_ERR_OUT_OF_RANGE, now, true);", self.sensors_h)

    # === FIX BUG 2: temp sensor lost notification uses sensorLostNotice, not T1min ===
    def test_temp_sensor_lost_notification_uses_sensor_lost_notice_not_alarm_token(self):
        # The notifier must return "Потеря датчика T1" from sensorLostNotice()
        # when SENSOR_LOST_ALARM_BIT fires, not the generic alarm token "T1min".
        self.assertIn("if (s && bitIdx == SENSOR_LOST_ALARM_BIT && s->hasSensorLostAlarm()) {", self.remote_notifier_h)
        self.assertIn("return s->sensorLostNotice();", self.remote_notifier_h)
        # FIX: RemoteNotifier now checks sensorLostAlarm BEFORE falling through to
        # _alarmToken() which would produce "T1min" instead of "Потеря датчика T1".
        self.assertIn(
            "if (s && bitIdx == SENSOR_LOST_ALARM_BIT && s->hasSensorLostAlarm()) {\n            return s->sensorLostNotice();\n        }",
            self.remote_notifier_h,
        )
        # The alarmToken must not be called for sensor_lost events.
        self.assertIn("static String _alarmToken(uint8_t bitIdx)", self.remote_notifier_h)
        # sensorLostNotice() must concatenate name properly.
        self.assertIn('return hasSensorLostAlarm() ? (String("Потеря датчика ") + name) : String("");', self.sensors_h)

    # === FIX BUG 3: sticky error — sensorErrorLatched stays after physical restore ===
    def test_sensor_error_latched_stays_true_until_operator_off_on_cycle(self):
        # After a sensor fault, sensorErrorLatched stays true even if the sensor
        # physically reconnects and produces valid readings. Only the operator
        # off->on cycle (applyOperatorResetCycle) can clear it.
        self.assertIn("sensorErrorLatched = !present;", self.sensors_h)
        # FIX: begin() now calls markSensorFault when sensor is absent, so
        # hasSensorLostAlarm() returns true immediately at boot.
        self.assertIn(
            "if (!present) {\n            markSensorFault(SENSOR_ERR_NO_RESPONSE, millis(), false);\n        }",
            self.sensors_h,
        )
        # markSensorHealthy must NOT clear sensorErrorLatched automatically.
        # The condition "if (!_trackSensorLoss || !sensorErrorLatched)" confirms this.
        self.assertIn(
            "if (!_trackSensorLoss || !sensorErrorLatched) sensorErrorReason = SENSOR_ERR_NONE;",
            self.sensors_h,
        )
        # applyOperatorResetCycle must NOT unconditionally reset sensorErrorLatched
        # to false before checking _canClearLatchedErrorNow().
        self.assertIn(
            "if (_canClearLatchedErrorNow()) {\n            sensorErrorLatched = false;",
            self.sensors_h,
        )
        # FIX: after the first healthy reading, sensorErrorLatched stays true
        # (Relatched path does NOT reset it to false).
        self.assertNotIn(
            "sensorErrorLatched = true;\n        return SensorOperatorResetResult::Relatched;",
            self.sensors_h,
        )
        # logSensorTransitions must log "sensor restored" message (not just the
        # latched=true case) so operator knows hardware recovered.
        self.assertIn(
            '"Датчик ") + s->name + " восстановился, сбросьте ошибку в интерфейсе"',
            self.rect_column_ino,
        )
        # Sticky error must block sensor from control even when live value is valid.
        self.assertIn("!sensorErrorLatched && !isnan(value)", self.sensors_h)
        # isInEnableWarmup further delays re-entry after operator off->on.
        self.assertIn("isInEnableWarmup()", self.sensors_h)

    # === FIX BUG 4: relay timeout -> neutral state, manualWant cleared ===
    def test_relay_timeout_clears_manualWant_and_allows_retry_on(self):
        # After WER confirmation timeout:
        # 1. relayError=timeout is set
        # 2. manualWant is cleared (button shows "выключено" not "включено")
        # 3. operatorHoldOff stays false (retry ON is allowed)
        # 4. NO OFF command is sent — neutral zone keeps last confirmed physical state
        self.assertIn(
            "out[oi]->restoreManualWant(false);\n                _manualStateDirty = true;",
            self.output_manager_h,
        )
        # The clearCommand + restoreManualWant sequence must appear after the
        # timeout check in updateRelayCommandFeedback.
        # FIX: relay timeout handler clears manualWant (Bug 4).
        # The timeout branch in updateRelayCommandFeedback must:
        # 1. clearCommand
        # 2. restoreManualWant(false)  <- key fix that clears button state
        # 3. NOT call forceOff
        # We check the two non-algorithmic strings independently to avoid
        # hitting UTF-8 encoding issues in the file read.
        timeout_start = self.output_manager_h.find(
            "if (now - out[oi]->commandSentAt() >= _relayConfirmTimeoutMs(oi))"
        )
        self.assertGreater(timeout_start, -1)
        # RestoreManualWant appears on a later line after clearCommand.
        self.assertIn("restoreManualWant(false);", self.output_manager_h)
        # forceOff must NOT appear in the timeout branch itself.
        block_section = self.output_manager_h[timeout_start:timeout_start + 1200]
        self.assertNotIn("forceOff", block_section)


if __name__ == "__main__":
    unittest.main(verbosity=2)
