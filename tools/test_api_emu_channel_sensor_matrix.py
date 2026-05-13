"""
Live EMU channel/sensor matrix tests.

These tests require firmware built with EMU_MODE=true.
If the device is running a real build (EMU_MODE=false), the whole test case skips.

Run from the project root:
    set RECTCOLUMN_BASE_URL=http://192.168.4.1
    python -m unittest -v tools.test_api_emu_channel_sensor_matrix

Linux/macOS:
    RECTCOLUMN_BASE_URL=http://192.168.4.1 python -m unittest -v tools.test_api_emu_channel_sensor_matrix
"""

from __future__ import annotations

import json
import time
import unittest

try:  # pragma: no cover - import style depends on how unittest is launched
    from .api_testlib import (
        LiveEmuApiTestCase,
        confirmation_map,
        output_config_payload_from_state,
        output_map,
        runtime_output_map,
        safe_emu_payload,
        sensor_map,
    )
except ImportError:  # pragma: no cover
    from api_testlib import (  # type: ignore
        LiveEmuApiTestCase,
        confirmation_map,
        output_config_payload_from_state,
        output_map,
        runtime_output_map,
        safe_emu_payload,
        sensor_map,
    )


MAIN_OUTPUTS = (("CH1", 0), ("CH2", 1), ("CH3", 2))
ANALOG_SENSORS = ("T1", "T2", "T3", "P")
DIGITAL_SENSORS = ("L", "F")
ALL_SENSORS = ANALOG_SENSORS + DIGITAL_SENSORS + ("dT", "C", "V")


def off_payload(sensor_id: str) -> dict[str, object]:
    if sensor_id in ANALOG_SENSORS:
        return {sensor_id: 85.0}
    return {sensor_id: False}


def neutral_payload(sensor_id: str) -> dict[str, object]:
    if sensor_id in ANALOG_SENSORS:
        return {sensor_id: 75.0}
    return {sensor_id: True}


def on_payload(sensor_id: str) -> dict[str, object]:
    return {sensor_id: 65.0}


class EmuChannelSensorMatrixTests(LiveEmuApiTestCase):
    def setUp(self):
        snapshot = self.snapshot_config()
        self.addCleanup(self.restore_snapshot, snapshot)
        self._post_history: list[dict[str, object]] = []
        self._reset_runtime()

    def _post_json_logged(self, path: str, payload: object | None = None, *,
                          ok_statuses: tuple[int, ...] = (200,)) -> dict[str, object]:
        response = self.api.post_json(path, payload, ok_statuses=ok_statuses)
        self._post_history.append({
            "path": path,
            "payload": payload,
            "response": response,
        })
        return response if isinstance(response, dict) else {"raw": response}

    def _debug_snapshot(self, label: str) -> dict[str, object]:
        state = self.api.get_json("/api/v1/state")
        output_config = self.api.get_json("/api/v1/output/config")
        sensors = sensor_map(state)
        return {
            "label": label,
            "state_raw": state,
            "state_summary": {
                "fw": state.get("fw"),
                "emu": state.get("emu"),
                "muted": state.get("muted"),
                "stopLatched": state.get("stopLatched"),
                "safetyAlarmActive": state.get("safetyAlarmActive"),
            },
            "outputs_state": output_map(state),
            "outputs_runtime": runtime_output_map(output_config),
            "confirmations": confirmation_map(state),
            "f_sensor": sensors.get("F"),
            "post_history": list(self._post_history),
        }

    def _capture_state_trace(self, duration: float = 1.4, interval: float = 0.12) -> list[dict[str, object]]:
        trace: list[dict[str, object]] = []
        started = time.time()
        while time.time() - started < duration:
            state = self.api.get_json("/api/v1/state")
            outputs = output_map(state)
            sensors = sensor_map(state)
            trace.append({
                "t": round(time.time() - started, 3),
                "CH1": {
                    "actual": outputs["CH1"]["actual"],
                    "forbidden": outputs["CH1"]["forbidden"],
                    "forbidMask": outputs["CH1"]["forbidMask"],
                    "sensorForbidMask": outputs["CH1"].get("sensorForbidMask"),
                    "effectiveForbidMask": outputs["CH1"]["effectiveForbidMask"],
                    "forbidReasons": outputs["CH1"].get("forbidReasons", []),
                    "manualWant": outputs["CH1"].get("manualWant"),
                },
                "CH2": {
                    "actual": outputs["CH2"]["actual"],
                    "forbidden": outputs["CH2"]["forbidden"],
                    "forbidMask": outputs["CH2"]["forbidMask"],
                    "sensorForbidMask": outputs["CH2"].get("sensorForbidMask"),
                    "effectiveForbidMask": outputs["CH2"]["effectiveForbidMask"],
                    "forbidReasons": outputs["CH2"].get("forbidReasons", []),
                },
                "CH3": {
                    "actual": outputs["CH3"]["actual"],
                    "forbidden": outputs["CH3"]["forbidden"],
                    "forbidMask": outputs["CH3"]["forbidMask"],
                    "sensorForbidMask": outputs["CH3"].get("sensorForbidMask"),
                    "effectiveForbidMask": outputs["CH3"]["effectiveForbidMask"],
                    "forbidReasons": outputs["CH3"].get("forbidReasons", []),
                },
                "F": sensors.get("F"),
            })
            time.sleep(interval)
        return trace

    def _reset_runtime(self) -> None:
        self._post_json_logged("/api/v1/mute", {"muted": True})
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
        self._disable_all_main_rules()
        self._post_json_logged("/api/v1/stop", {})
        self._post_json_logged("/api/v1/stop?release=1", {})
        self._post_json_logged("/api/v1/safety/reset", {})
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
        self._turn_off_all_main_outputs()
        self.api.wait_for_state(
            lambda current: (
                not current.get("stopLatched", False)
                and not current.get("safetyAlarmActive", False)
                and all(
                    not output_map(current)[output_id]["actual"]
                    and not output_map(current)[output_id].get("pending", False)
                    and not output_map(current)[output_id].get("relayPending", False)
                    for output_id, _ in MAIN_OUTPUTS
                )
            ),
            timeout=5.0,
        )

    def _disable_all_main_rules(self) -> None:
        for sensor_id in ALL_SENSORS:
            for _, out_idx in MAIN_OUTPUTS:
                self._post_json_logged(
                    f"/api/v1/sensor/{sensor_id}/ctrl",
                    {"outIdx": out_idx, "enabled": False, "logic": "heat", "min": 0, "max": 100},
                    ok_statuses=(200,),
                )

    def _set_sensor_rule(self, sensor_id: str, out_idx: int, *, enabled: bool,
                         logic: str = "heat", min_v: float = 70.0, max_v: float = 80.0) -> None:
        self._post_json_logged(
            f"/api/v1/sensor/{sensor_id}/ctrl",
            {"outIdx": out_idx, "enabled": enabled, "logic": logic, "min": min_v, "max": max_v},
            ok_statuses=(200,),
        )

    def _set_sensor_config(self, sensor_id: str, *, ctrl_delay_ms: int | None = None,
                           enabled: bool | None = None) -> None:
        payload: dict[str, object] = {}
        if enabled is not None:
            payload["enabled"] = enabled
        if ctrl_delay_ms is not None:
            payload["ctrlDelayMs"] = ctrl_delay_ms
        if not payload:
            return
        self._post_json_logged(f"/api/v1/sensor/{sensor_id}/config", payload)

    def _wait_outputs(self, expected: dict[str, bool], *, timeout: float = 4.0):
        return self.api.wait_for_state(
            lambda current: all(output_map(current)[output_id]["actual"] is value
                                for output_id, value in expected.items()),
            timeout=timeout,
        )

    def _wait_outputs_idle(self, expected: dict[str, bool], *, timeout: float = 5.0):
        return self.api.wait_for_state(
            lambda current: all(
                output_map(current)[output_id]["actual"] is value
                and not output_map(current)[output_id].get("pending", False)
                and not output_map(current)[output_id].get("relayPending", False)
                for output_id, value in expected.items()
            ),
            timeout=timeout,
        )

    def _turn_on_outputs(self, output_ids: tuple[str, ...]) -> None:
        expected = {output_id: True for output_id in output_ids}
        last_error: AssertionError | None = None
        for attempt in range(2):
            current_state = self.api.get_json("/api/v1/state")
            current_outputs = output_map(current_state)
            for output_id in output_ids:
                if current_outputs[output_id]["actual"] is True:
                    continue
                response = self._post_json_logged(
                    f"/api/v1/output/{output_id}/manual",
                    {"state": True},
                    ok_statuses=(200, 409),
                )
                accepted = response.get("accepted", False)
                duplicate = response.get("reason") == "duplicate" or response.get("detail") == "duplicate"
                self.assertTrue(accepted or duplicate, f"manual ON rejected for {output_id}: {response}")
            try:
                self._wait_outputs(expected, timeout=6.0 if attempt == 0 else 4.0)
                return
            except AssertionError as exc:
                last_error = exc
                time.sleep(0.4)
        debug_dump = self._debug_snapshot("turn_on_outputs_failed")
        raise AssertionError(f"{last_error}\n{json.dumps(debug_dump, ensure_ascii=False, indent=2)}")

    def _turn_off_all_main_outputs(self) -> None:
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
        for output_id, _ in MAIN_OUTPUTS:
            self._post_json_logged(f"/api/v1/output/{output_id}/manual", {"state": False}, ok_statuses=(200, 409))
        self._wait_outputs_idle({output_id: False for output_id, _ in MAIN_OUTPUTS}, timeout=5.0)

    def _prepare_single_rule(self, sensor_id: str, output_id: str, out_idx: int) -> None:
        self._reset_runtime()
        delay_ms = 500 if sensor_id in DIGITAL_SENSORS else 0
        self._set_sensor_config(sensor_id, enabled=True, ctrl_delay_ms=delay_ms)
        self._set_sensor_rule(sensor_id, out_idx, enabled=True, logic="heat", min_v=70.0, max_v=80.0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**neutral_payload(sensor_id)))
        self.api.wait_for_state(
            lambda current: any(
                ctrl["outIdx"] == out_idx and ctrl["enabled"] is True
                for ctrl in sensor_map(current)[sensor_id].get("ctrl", [])
            ),
            timeout=3.0,
        )

    def test_flow_regression_only_ch1_turns_off_when_ch2_flow_rule_is_disabled(self):
        self._reset_runtime()
        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=500)
        self._set_sensor_rule("F", 0, enabled=True)
        self._set_sensor_rule("F", 1, enabled=False)
        self._set_sensor_rule("F", 2, enabled=False)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=True))
        before_config = self._debug_snapshot("before_flow_loss")

        self._turn_on_outputs(("CH1", "CH2", "CH3"))
        after_config = self._debug_snapshot("after_outputs_on")
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))
        state_trace = self._capture_state_trace(duration=1.6, interval=0.12)

        state = self._wait_outputs({"CH1": False, "CH2": True, "CH3": True}, timeout=4.0)
        outputs = output_map(state)
        debug_dump = {
            "before_config": before_config,
            "after_config": after_config,
            "after_flow_loss": self._debug_snapshot("after_flow_loss"),
            "state_trace": state_trace,
        }
        self.assertEqual(
            [ctrl["enabled"] for ctrl in sensor_map(state)["F"]["ctrl"][:3]],
            [True, False, False],
            msg=json.dumps(debug_dump, ensure_ascii=False, indent=2),
        )
        self.assertFalse(outputs["CH1"]["actual"], msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))
        self.assertTrue(outputs["CH2"]["actual"], msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))
        self.assertTrue(outputs["CH3"]["actual"], msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))
        self.assertEqual(sensor_map(state)["F"]["value"], 0, msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))
        self.assertNotIn("F", outputs["CH2"].get("forbidReasons", []), msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))
        self.assertNotIn("F", outputs["CH3"].get("forbidReasons", []), msg=json.dumps(debug_dump, ensure_ascii=False, indent=2))

    def _assert_off_condition_turns_off_only_target_main_channel(self, sensor_id: str,
                                                                 output_id: str,
                                                                 out_idx: int) -> None:
        self._prepare_single_rule(sensor_id, output_id, out_idx)
        self._turn_on_outputs(tuple(output for output, _ in MAIN_OUTPUTS))
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**off_payload(sensor_id)))

        expected = {current_output: True for current_output, _ in MAIN_OUTPUTS}
        expected[output_id] = False
        if sensor_id == "P" and output_id != "CH1":
            expected["CH1"] = False
        state = self._wait_outputs(expected, timeout=6.0)
        outputs = output_map(state)
        if sensor_id != "F":
            self.assertIn(sensor_id, outputs[output_id].get("forbidReasons", []))
        if sensor_id == "P":
            self.assertIn("SAFETY_PRESSURE", outputs["CH1"].get("forbidReasons", []))
            if output_id != "CH1":
                self.assertNotIn("P", outputs["CH1"].get("forbidReasons", []))
        for other_output, _ in MAIN_OUTPUTS:
            if other_output == output_id:
                continue
            if sensor_id == "P" and other_output == "CH1" and output_id != "CH1":
                continue
            self.assertNotIn(sensor_id, outputs[other_output].get("forbidReasons", []))

    def test_analog_heat_on_turns_on_only_target_channel(self):
        for sensor_id in ANALOG_SENSORS:
            for output_id, out_idx in MAIN_OUTPUTS:
                with self.subTest(sensor=sensor_id, output=output_id):
                    self._prepare_single_rule(sensor_id, output_id, out_idx)
                    self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**on_payload(sensor_id)))

                    expected = {current_output: False for current_output, _ in MAIN_OUTPUTS}
                    expected[output_id] = True
                    state = self._wait_outputs(expected, timeout=4.0)
                    outputs = output_map(state)
                    self.assertFalse(outputs[output_id]["forbidden"])
                    self.assertGreater(outputs[output_id].get("wantOnMask", 0), 0)

    def test_analog_neutral_holds_channel_state_without_false_forbid(self):
        for sensor_id in ANALOG_SENSORS:
            for output_id, out_idx in MAIN_OUTPUTS:
                with self.subTest(sensor=sensor_id, output=output_id):
                    self._prepare_single_rule(sensor_id, output_id, out_idx)
                    self._turn_on_outputs((output_id,))
                    self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**neutral_payload(sensor_id)))

                    state = self._wait_outputs({output_id: True}, timeout=4.0)
                    current = output_map(state)[output_id]
                    self.assertFalse(current["forbidden"])
                    self.assertNotIn(sensor_id, current.get("forbidReasons", []))

    def test_api_rejects_extended_sensor_rules_for_main_channels(self):
        self._reset_runtime()
        for sensor_id in ("dT", "C", "V"):
            for _, out_idx in MAIN_OUTPUTS:
                with self.subTest(sensor=sensor_id, out_idx=out_idx):
                    status, response = self.api.request_json(
                        f"/api/v1/sensor/{sensor_id}/ctrl",
                        method="POST",
                        payload={"outIdx": out_idx, "enabled": True, "logic": "heat", "min": 0, "max": 100},
                        ok_statuses=(400,),
                    )
                    self.assertEqual(status, 400)
                    self.assertEqual(response.get("type"), "bad_params")
                    self.assertIn("CH1..CH3", response.get("error", ""))

    def test_wer_ch1_force_off_times_out_only_ch1_on_real_api(self):
        self._reset_runtime()
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(WER_CH1_mode="force_off"))
        self._turn_on_outputs(("CH2",))
        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is False
                and output_map(current)["CH2"]["actual"] is True
                and output_map(current)["CH1"].get("relayError") == "timeout"
                and confirmation_map(current)["WER_CH1"]["emuMode"] == "force_off"
            ),
            timeout=5.0,
        )
        outputs = output_map(state)
        confirmations = confirmation_map(state)
        self.assertFalse(outputs["CH1"]["actual"])
        self.assertTrue(outputs["CH2"]["actual"])
        self.assertEqual(outputs["CH1"].get("relayError"), "timeout")
        self.assertFalse(outputs["CH1"].get("manualWant"))
        self.assertFalse(state.get("safetyAlarmActive", False))
        self.assertEqual(confirmations["WER_CH1"].get("emuMode"), "force_off")
        self.assertFalse(confirmations["WER_CH1"].get("faultLatched", False))
        self.assertNotIn("SAFETY_WER", outputs["CH1"].get("forbidReasons", []))
        self.assertNotIn("SAFETY_WER", outputs["CH2"].get("forbidReasons", []))

    def test_wer_ch4_force_off_times_out_without_blocking_main_channels_on_real_api(self):
        self._reset_runtime()
        state = self.api.get_json("/api/v1/state")
        output_cfg = output_config_payload_from_state(state)
        output_cfg["ch4Enabled"] = True
        self._post_json_logged("/api/v1/output/config", output_cfg)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(WER_CH4_mode="force_off"))

        self._turn_on_outputs(("CH1", "CH2"))
        response = self._post_json_logged("/api/v1/output/CH4/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is True
                and output_map(current)["CH2"]["actual"] is True
                and output_map(current)["CH4"]["actual"] is False
                and output_map(current)["CH4"].get("relayError") == "timeout"
                and confirmation_map(current)["WER_CH4"]["emuMode"] == "force_off"
            ),
            timeout=5.0,
        )
        outputs = output_map(state)
        confirmations = confirmation_map(state)
        self.assertTrue(outputs["CH1"]["actual"])
        self.assertTrue(outputs["CH2"]["actual"])
        self.assertFalse(outputs["CH4"]["actual"])
        self.assertEqual(outputs["CH4"].get("relayError"), "timeout")
        self.assertFalse(outputs["CH4"].get("manualWant"))
        self.assertFalse(state.get("safetyAlarmActive", False))
        self.assertEqual(confirmations["WER_CH4"].get("emuMode"), "force_off")
        self.assertFalse(confirmations["WER_CH4"].get("faultLatched", False))
        self.assertNotIn("SAFETY_WER", outputs["CH1"].get("forbidReasons", []))
        self.assertNotIn("SAFETY_WER", outputs["CH2"].get("forbidReasons", []))


def _make_off_condition_test(sensor_id: str, output_id: str, out_idx: int):
    def test(self: EmuChannelSensorMatrixTests):
        self._assert_off_condition_turns_off_only_target_main_channel(sensor_id, output_id, out_idx)

    test.__name__ = f"test_off_condition_{sensor_id.lower()}_{output_id.lower()}"
    return test


for _sensor_id in ANALOG_SENSORS + DIGITAL_SENSORS:
    for _output_id, _out_idx in MAIN_OUTPUTS:
        setattr(
            EmuChannelSensorMatrixTests,
            f"test_off_condition_{_sensor_id.lower()}_{_output_id.lower()}",
            _make_off_condition_test(_sensor_id, _output_id, _out_idx),
        )


if __name__ == "__main__":
    unittest.main(verbosity=2)
