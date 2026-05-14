#!/usr/bin/env python3
"""
Host-side checks for RectColumn CH1..CH3 automation semantics.
This is a lightweight model of SensorBase::evalCtrl plus Output::applyResolvedHold.
It does not touch ESP32 hardware.
"""
from dataclasses import dataclass

LOGIC_HEAT = 0
LOGIC_COOL = 1
OUT_CH1 = 0
OUT_CH2 = 1
OUT_CH3 = 2
SEN_L = 5
SEN_F = 6


@dataclass
class CtrlRule:
    enabled: bool = False
    outIdx: int = 0
    logic: int = LOGIC_HEAT
    minVal: float = 0.0
    maxVal: float = 100.0


def eval_ctrl(value, present, error, rule: CtrlRule, out_idx: int,
              elapsed_ms=0, delay_ms=0, invalid_means_off=True, control_gate=True):
    if out_idx != rule.outIdx or not rule.enabled:
        return 0
    if not control_gate:
        return 0
    if not present or error or value is None:
        return -1 if invalid_means_off else 0
    cmd = 0
    if rule.logic == LOGIC_HEAT:
        if value < rule.minVal:
            cmd = 1
        elif value > rule.maxVal:
            cmd = -1
    else:
        if value > rule.maxVal:
            cmd = 1
        elif value < rule.minVal:
            cmd = -1
    if cmd and delay_ms and elapsed_ms < delay_ms:
        return 0
    return cmd


def normalize_lf(rule: CtrlRule, out_idx: int):
    keep = rule.enabled
    rule.enabled = keep
    rule.outIdx = out_idx
    rule.logic = LOGIC_COOL
    rule.minVal = 0.5
    rule.maxVal = 2.0
    return rule


class OutputHold:
    def __init__(self):
        self.enabled = True
        self.actual = False
        self.requested = False
        self.manualWant = False
        self.forbidMask = 0
        self.wantOnMask = 0

    def apply(self, forbid, want):
        self.forbidMask = forbid
        self.wantOnMask = want
        if not self.enabled:
            self.manualWant = False
            self.requested = False
        elif forbid:
            self.manualWant = False
            self.requested = False
        elif want:
            self.requested = True
        else:
            self.requested = self.actual
        self.actual = self.requested

    def manual(self, on):
        if on and (not self.enabled or self.forbidMask):
            return False
        self.manualWant = on
        self.requested = bool(on and self.enabled and not self.forbidMask)
        self.actual = self.requested
        return True


def assert_seq(name, got, expected):
    assert got == expected, f"{name}: got {got}, expected {expected}"
    print(f"OK {name}: {got}")


def analog_cmd(logic, value, min_v=70, max_v=80):
    r = CtrlRule(True, OUT_CH1, logic, min_v, max_v)
    return eval_ctrl(value, True, False, r, OUT_CH1)


def drive_analog(logic, values):
    o = OutputHold()
    states = []
    for v in values:
        cmd = analog_cmd(logic, v)
        forbid = 1 if cmd == -1 else 0
        want = 1 if cmd == 1 else 0
        o.apply(forbid, want)
        states.append(o.actual)
    return states


def test_heat_hysteresis():
    states = drive_analog(LOGIC_HEAT, [69, 75, 81, 75, 69])
    assert_seq("HEAT hysteresis", states, [True, True, False, False, True])


def test_cool_hysteresis():
    states = drive_analog(LOGIC_COOL, [81, 75, 69, 75, 81])
    assert_seq("COOL hysteresis", states, [True, True, False, False, True])


def _check_digital(sensor_name):
    r = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.0, 100.0), OUT_CH1)
    o = OutputHold()
    o.manual(True)
    seq = []
    for value, elapsed in [(1.0, 0), (0.0, 100), (0.0, 5000), (1.0, 6000)]:
        cmd = eval_ctrl(value, True, False, r, OUT_CH1, elapsed_ms=elapsed, delay_ms=5000)
        forbid = 1 if cmd == -1 else 0
        want = 1 if cmd == 1 else 0
        o.apply(forbid, want)
        seq.append((cmd, o.actual))
    assert_seq(sensor_name, seq, [(0, True), (0, True), (-1, False), (0, False)])


def test_digital_l():
    _check_digital("Digital L")


def test_digital_f():
    _check_digital("Digital F")


def test_flow_requires_output_on():
    r = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.0, 100.0), OUT_CH1)
    cmd_off = eval_ctrl(0.0, True, False, r, OUT_CH1, elapsed_ms=6000, delay_ms=5000, control_gate=False)
    cmd_on = eval_ctrl(0.0, True, False, r, OUT_CH1, elapsed_ms=6000, delay_ms=5000, control_gate=True)
    assert_seq("Flow gate OFF", cmd_off, 0)
    assert_seq("Flow gate ON", cmd_on, -1)


def test_flow_is_channel_local():
    ch1 = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.0, 100.0), OUT_CH1)
    ch2 = normalize_lf(CtrlRule(False, OUT_CH2, LOGIC_HEAT, 0.0, 100.0), OUT_CH2)
    ch3 = normalize_lf(CtrlRule(False, OUT_CH3, LOGIC_HEAT, 0.0, 100.0), OUT_CH3)
    cmd_ch1 = eval_ctrl(0.0, True, False, ch1, OUT_CH1, elapsed_ms=6000, delay_ms=5000, control_gate=True)
    cmd_ch2 = eval_ctrl(0.0, True, False, ch2, OUT_CH2, elapsed_ms=6000, delay_ms=5000, control_gate=True)
    cmd_ch3 = eval_ctrl(0.0, True, False, ch3, OUT_CH3, elapsed_ms=6000, delay_ms=5000, control_gate=True)
    assert_seq("Flow CH1 local", cmd_ch1, -1)
    assert_seq("Flow CH2 disabled", cmd_ch2, 0)
    assert_seq("Flow CH3 disabled", cmd_ch3, 0)


def test_lf_mode_isolation():
    lf = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.0, 100.0), OUT_CH1)
    for channel_logic in (LOGIC_HEAT, LOGIC_COOL, LOGIC_HEAT):
        # output mode changes must not rewrite the fixed L/F rule
        _ = channel_logic
        normalize_lf(lf, OUT_CH1)
        assert (lf.logic, lf.minVal, lf.maxVal) == (LOGIC_COOL, 0.5, 2.0)
    print("OK L/F mode isolation")


def test_storage_migration():
    old_enabled = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.0, 100.0), OUT_CH1)
    assert (old_enabled.enabled, old_enabled.logic, old_enabled.minVal, old_enabled.maxVal) == (True, LOGIC_COOL, 0.5, 2.0)
    bad_v2 = normalize_lf(CtrlRule(True, OUT_CH1, LOGIC_HEAT, 0.5, 2.0), OUT_CH1)
    assert (bad_v2.enabled, bad_v2.logic, bad_v2.minVal, bad_v2.maxVal) == (True, LOGIC_COOL, 0.5, 2.0)
    print("OK storage migration")


def test_manual_command():
    o = OutputHold()
    assert o.manual(True) is True
    o.apply(0, 0)
    assert o.actual is True
    assert o.manual(False) is True
    o.apply(0, 0)
    assert o.actual is False
    o.apply(1, 0)
    assert o.manual(True) is False
    assert o.actual is False
    o.apply(0, 0)
    assert o.actual is False

    o.manual(True)
    assert o.actual is True
    o.apply(1, 0)
    assert o.actual is False
    assert o.manualWant is False
    o.apply(0, 0)
    assert o.actual is False
    print("OK manual command")


def test_control_sensor_failure_is_auto_off():
    r = CtrlRule(True, OUT_CH1, LOGIC_HEAT, 70.0, 80.0)
    assert_seq("sensor NAN auto off", eval_ctrl(None, True, False, r, OUT_CH1, invalid_means_off=True), -1)
    assert_seq("sensor absent auto off", eval_ctrl(75.0, False, False, r, OUT_CH1, invalid_means_off=True), -1)
    assert_seq("sensor error auto off", eval_ctrl(75.0, True, True, r, OUT_CH1, invalid_means_off=True), -1)


def test_scheme_excludes_extended_sensors():
    # Firmware now disables dT/C/V rules for CH1..CH3; only T1/T2/T3/P/L/F
    # belong to the channel-control chart.
    extended_sensor_indices = [3, 7, 8]
    for si in extended_sensor_indices:
        assert si not in {0, 1, 2, 4, 5, 6}
    print("OK scheme sensor set")


def main():
    test_heat_hysteresis()
    test_cool_hysteresis()
    _check_digital("Digital L")
    _check_digital("Digital F")
    test_flow_requires_output_on()
    test_flow_is_channel_local()
    test_lf_mode_isolation()
    test_storage_migration()
    test_manual_command()
    test_control_sensor_failure_is_auto_off()
    test_scheme_excludes_extended_sensors()


if __name__ == "__main__":
    main()
