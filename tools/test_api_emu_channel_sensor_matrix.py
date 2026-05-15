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
        last_error: AssertionError | None = None
        for attempt in range(3):
            self._post_json_logged("/api/v1/mute", {"muted": True})
            self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
            self._disable_all_main_rules()
            self._post_json_logged("/api/v1/output/CH4/manual", {"state": False}, ok_statuses=(200, 409))
            self._post_json_logged("/api/v1/output/CH5/manual", {"state": False}, ok_statuses=(200, 409))
            self._post_json_logged("/api/v1/stop", {})
            self._post_json_logged("/api/v1/stop?release=1", {})
            self._post_json_logged("/api/v1/safety/reset", {})
            self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
            self._turn_off_all_main_outputs()
            try:
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
                        and output_map(current)["CH4"]["actual"] is False
                        and output_map(current)["CH5"]["actual"] is False
                    ),
                    timeout=8.0 if attempt == 0 else 5.0,
                )
                return
            except AssertionError as exc:
                last_error = exc
                time.sleep(0.8)
        if last_error is not None:
            raise last_error

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
                           alarm_delay_ms: int | None = None,
                           enabled: bool | None = None) -> None:
        payload: dict[str, object] = {}
        if enabled is not None:
            payload["enabled"] = enabled
        if alarm_delay_ms is not None:
            payload["alarmDelayMs"] = alarm_delay_ms
        if ctrl_delay_ms is not None:
            payload["ctrlDelayMs"] = ctrl_delay_ms
        if not payload:
            return
        self._post_json_logged(f"/api/v1/sensor/{sensor_id}/config", payload)

    def _set_sensor_alarm(self, sensor_id: str, idx: int, *, enabled: bool,
                          threshold: float, is_max: bool) -> None:
        self._post_json_logged(
            f"/api/v1/sensor/{sensor_id}/alarm",
            {"idx": idx, "enabled": enabled, "threshold": threshold, "isMax": is_max},
            ok_statuses=(200,),
        )

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

    def _assert_enabled_sensor_error_blocks_manual_on(self, sensor_id: str, output_id: str, out_idx: int) -> None:
        self._prepare_single_rule(sensor_id, output_id, out_idx)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**{f"{sensor_id}err": True}))

        state = self.api.wait_for_state(
            lambda current: (
                sensor_map(current)[sensor_id]["error"] is True
                and output_map(current)[output_id]["forbidden"] is True
            ),
            timeout=4.0,
        )
        self.assertIn(sensor_id, output_map(state)[output_id].get("forbidReasons", []))

        status, response = self.api.request_json(
            f"/api/v1/output/{output_id}/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")
        self.assertIn(f"Команда {output_id}: включение", response.get("userMessage", ""))
        self.assertIn(sensor_id, response.get("userMessage", ""))

    def _assert_emupanel_error_blocks_immediate_manual_on(self, sensor_id: str,
                                                          output_id: str,
                                                          out_idx: int) -> None:
        self._prepare_single_rule(sensor_id, output_id, out_idx)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**{f"{sensor_id}err": True}))

        status, response = self.api.request_json(
            f"/api/v1/output/{output_id}/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")

        state = self._wait_outputs_idle({output_id: False}, timeout=4.0)
        sensor_state = sensor_map(state)[sensor_id]
        output_state = output_map(state)[output_id]
        debug = output_state.get("debug", {})
        debug_sensors = {
            item["id"]: item
            for item in debug.get("controlSensors", [])
            if isinstance(item, dict) and item.get("id")
        }
        self.assertTrue(sensor_state["error"])
        self.assertTrue(output_state["forbidden"])
        self.assertTrue(output_state.get("autoOff", False))
        self.assertFalse(output_state.get("manualRequest", True))
        self.assertFalse(output_state.get("finalRelayState", True))
        self.assertFalse(output_state.get("physicalRelayState", True))
        self.assertEqual(output_state.get("blockReason"), output_state.get("forbidReasonText"))
        self.assertIn(sensor_id, debug_sensors)
        self.assertTrue(debug_sensors[sensor_id].get("sensorError", False))
        self.assertTrue(debug_sensors[sensor_id].get("autoOff", False))

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

    def test_stop_global_block_clears_main_outputs_without_replaying_manual_on(self):
        self._reset_runtime()
        self._disable_all_main_rules()
        self._turn_on_outputs(("CH1", "CH2", "CH3"))

        self._post_json_logged("/api/v1/stop", {})
        stopped = self._wait_outputs_idle({"CH1": False, "CH2": False, "CH3": False}, timeout=5.0)
        stopped_outputs = output_map(stopped)
        self.assertTrue(stopped.get("stopLatched", False))
        for output_id in ("CH1", "CH2", "CH3"):
            self.assertFalse(stopped_outputs[output_id].get("manualWant", False))
            self.assertFalse(stopped_outputs[output_id].get("operatorHoldOff", False))

        self._post_json_logged("/api/v1/stop?release=1", {})
        released = self._wait_outputs_idle({"CH1": False, "CH2": False, "CH3": False}, timeout=5.0)
        self.assertFalse(released.get("stopLatched", True))
        released_outputs = output_map(released)
        for output_id in ("CH1", "CH2", "CH3"):
            self.assertFalse(released_outputs[output_id]["actual"])
            self.assertFalse(released_outputs[output_id].get("manualWant", False))
            self.assertFalse(released_outputs[output_id].get("operatorHoldOff", False))

    def test_auto_off_priority_over_auto_on_for_same_channel(self):
        self._reset_runtime()
        self._set_sensor_config("T1", enabled=True, ctrl_delay_ms=0)
        self._set_sensor_config("P", enabled=True, ctrl_delay_ms=0)
        self._set_sensor_rule("T1", 0, enabled=True, logic="heat", min_v=70.0, max_v=80.0)
        self._set_sensor_rule("P", 0, enabled=True, logic="heat", min_v=70.0, max_v=100.0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=65.0, P=90.0))
        self._wait_outputs({"CH1": True}, timeout=4.0)

        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=65.0, P=110.0))

        state = self._wait_outputs({"CH1": False, "CH2": False, "CH3": False}, timeout=4.0)
        ch1 = output_map(state)["CH1"]
        self.assertFalse(ch1["actual"])
        self.assertTrue(
            "P" in ch1.get("forbidReasons", []),
            msg=json.dumps(self._debug_snapshot("auto_off_priority_conflict"), ensure_ascii=False, indent=2),
        )
        self.assertGreater(ch1.get("wantOnMask", 0), 0)

    def test_manual_commands_execute_once_when_auto_is_neutral(self):
        self._reset_runtime()
        self._disable_all_main_rules()

        response_on = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response_on["accepted"])
        state_on = self._wait_outputs_idle({"CH1": True}, timeout=5.0)
        ch1_on = output_map(state_on)["CH1"]
        self.assertTrue(ch1_on["actual"])
        self.assertFalse(ch1_on.get("manualWant", False))
        self.assertFalse(ch1_on.get("operatorHoldOff", False))

        time.sleep(0.6)
        hold_state = self.api.get_json("/api/v1/state")
        self.assertTrue(output_map(hold_state)["CH1"]["actual"])

        response_off = self._post_json_logged("/api/v1/output/CH1/manual", {"state": False}, ok_statuses=(200,))
        self.assertTrue(response_off["accepted"])
        state_off = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1_off = output_map(state_off)["CH1"]
        self.assertFalse(ch1_off["actual"])
        self.assertFalse(ch1_off.get("manualWant", False))
        self.assertFalse(ch1_off.get("operatorHoldOff", False))

    def test_disabled_sensor_does_not_block_manual_on_on_real_api(self):
        for sensor_id, output_id, out_idx in (("T1", "CH1", 0), ("T2", "CH2", 1), ("T3", "CH3", 2)):
            with self.subTest(sensor=sensor_id, output=output_id):
                self._reset_runtime()
                self._set_sensor_config(sensor_id, enabled=False, ctrl_delay_ms=0)
                self._set_sensor_rule(sensor_id, out_idx, enabled=True, logic="heat", min_v=70.0, max_v=80.0)
                self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**{f"{sensor_id}err": True}))

                response = self._post_json_logged(f"/api/v1/output/{output_id}/manual", {"state": True}, ok_statuses=(200,))
                self.assertTrue(response["accepted"])

                state = self._wait_outputs_idle({output_id: True}, timeout=5.0)
                output_state = output_map(state)[output_id]
                debug = output_state.get("debug", {})
                debug_sensors = {
                    item["id"]: item
                    for item in debug.get("controlSensors", [])
                    if isinstance(item, dict) and item.get("id")
                }
                self.assertFalse(output_state["forbidden"])
                self.assertFalse(output_state.get("autoOff", False))
                self.assertNotIn(sensor_id, output_state.get("forbidReasons", []))
                self.assertIn(sensor_id, debug_sensors)
                self.assertFalse(debug_sensors[sensor_id].get("sensorEnabled", True))
                self.assertFalse(debug_sensors[sensor_id].get("autoOff", True))

    def test_disabling_sensor_clears_stale_auto_off_and_manual_block(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=85.0))
        self.api.wait_for_state(
            lambda current: output_map(current)["CH1"]["forbidden"] is True,
            timeout=4.0,
        )

        self._set_sensor_config("T1", enabled=False, ctrl_delay_ms=0)
        cleared = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(cleared)["CH1"]
        self.assertFalse(ch1["forbidden"])
        self.assertFalse(ch1.get("autoOff", False))
        self.assertNotIn("T1", ch1.get("forbidReasons", []))

        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])
        self._wait_outputs_idle({"CH1": True}, timeout=5.0)

    def test_disabling_rule_clears_stale_auto_off_and_manual_block(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=85.0))
        self.api.wait_for_state(
            lambda current: output_map(current)["CH1"]["forbidden"] is True,
            timeout=4.0,
        )

        self._set_sensor_rule("T1", 0, enabled=False, logic="heat", min_v=70.0, max_v=80.0)
        cleared = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(cleared)["CH1"]
        self.assertFalse(ch1["forbidden"])
        self.assertFalse(ch1.get("autoOff", False))
        self.assertNotIn("T1", ch1.get("forbidReasons", []))

        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])
        self._wait_outputs_idle({"CH1": True}, timeout=5.0)

    def test_reenabling_sensor_recomputes_from_live_neutral_value(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=85.0))
        self.api.wait_for_state(
            lambda current: output_map(current)["CH1"]["forbidden"] is True,
            timeout=4.0,
        )

        self._set_sensor_config("T1", enabled=False, ctrl_delay_ms=0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=75.0))
        self._set_sensor_config("T1", enabled=True, ctrl_delay_ms=0)

        state = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(state)["CH1"]
        self.assertFalse(ch1["forbidden"])
        self.assertFalse(ch1.get("autoOff", False))
        self.assertNotIn("T1", ch1.get("forbidReasons", []))

        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])
        self._wait_outputs_idle({"CH1": True}, timeout=5.0)

    def test_pressure_alarm_does_not_block_manual_when_pressure_rule_is_disabled(self):
        self._reset_runtime()
        self._set_sensor_config("P", enabled=True, ctrl_delay_ms=0)
        self._set_sensor_rule("P", 0, enabled=False, logic="heat", min_v=70.0, max_v=80.0)
        self._set_sensor_alarm("P", 0, enabled=True, threshold=1050.0, is_max=True)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(P=1100.0))

        state = self.api.wait_for_state(
            lambda current: sensor_map(current)["P"]["alarms"][0]["triggered"] is True,
            timeout=4.0,
        )
        self.assertTrue(sensor_map(state)["P"]["alarms"][0]["triggered"])
        self.assertFalse(output_map(state)["CH1"]["forbidden"])

        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

    def test_flow_alarm_is_inactive_while_ch2_is_off(self):
        self._reset_runtime()
        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=500, alarm_delay_ms=500)
        self._set_sensor_rule("F", 0, enabled=True)
        self._set_sensor_alarm("F", 0, enabled=True, threshold=0.0, is_max=False)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))
        time.sleep(0.8)

        state = self.api.get_json("/api/v1/state")
        self.assertFalse(sensor_map(state)["F"]["alarms"][0]["triggered"])

    def test_flow_alarm_appears_only_after_ch2_turns_on_without_flow(self):
        self._reset_runtime()
        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=500, alarm_delay_ms=500)
        self._set_sensor_rule("F", 0, enabled=True)
        self._set_sensor_alarm("F", 0, enabled=True, threshold=0.0, is_max=False)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))

        before_on = self.api.get_json("/api/v1/state")
        self.assertFalse(sensor_map(before_on)["F"]["alarms"][0]["triggered"])

        self._turn_on_outputs(("CH2",))
        alarmed = self.api.wait_for_state(
            lambda current: sensor_map(current)["F"]["alarms"][0]["triggered"] is True,
            timeout=4.0,
        )
        self.assertTrue(output_map(alarmed)["CH2"]["actual"])
        self.assertTrue(sensor_map(alarmed)["F"]["alarms"][0]["triggered"])

        self._post_json_logged("/api/v1/output/CH2/manual", {"state": False}, ok_statuses=(200, 409))
        cleared = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH2"]["actual"] is False
                and sensor_map(current)["F"]["alarms"][0]["triggered"] is False
            ),
            timeout=5.0,
        )
        self.assertFalse(sensor_map(cleared)["F"]["alarms"][0]["triggered"])

    def test_active_auto_off_blocks_manual_on_and_does_not_replay_later_on_real_api(self):
        self._reset_runtime()
        self._set_sensor_config("T1", enabled=True, ctrl_delay_ms=0)
        self._set_sensor_rule("T1", 0, enabled=True, logic="heat", min_v=70.0, max_v=80.0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=85.0))

        forbidden_state = self.api.wait_for_state(
            lambda current: output_map(current)["CH1"]["forbidden"] is True,
            timeout=4.0,
        )
        self.assertIn("T1", output_map(forbidden_state)["CH1"].get("forbidReasons", []))

        status, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")

        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=75.0))
        cleared = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(cleared)["CH1"]
        self.assertFalse(ch1["actual"])
        self.assertFalse(ch1.get("manualWant", False))
        self.assertFalse(ch1.get("operatorHoldOff", False))

    def test_enabled_t1_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on("T1", "CH1", 0)

    def test_enabled_t2_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on("T2", "CH2", 1)

    def test_enabled_t3_error_blocks_manual_on_for_affected_channel(self):
        self._assert_enabled_sensor_error_blocks_manual_on("T3", "CH3", 2)

    def test_emupanel_t1_error_then_manual_on_keeps_ch1_off(self):
        self._assert_emupanel_error_blocks_immediate_manual_on("T1", "CH1", 0)

    def test_emupanel_t2_error_then_manual_on_keeps_ch2_off(self):
        self._assert_emupanel_error_blocks_immediate_manual_on("T2", "CH2", 1)

    def test_emupanel_t3_error_then_manual_on_keeps_ch3_off(self):
        self._assert_emupanel_error_blocks_immediate_manual_on("T3", "CH3", 2)

    def test_active_sensor_error_turns_already_enabled_channel_off(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._turn_on_outputs(("CH1",))
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1err=True))

        state = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(state)["CH1"]
        self.assertTrue(sensor_map(state)["T1"]["error"])
        self.assertIn("T1", ch1.get("forbidReasons", []))
        self.assertFalse(ch1.get("manualWant", False))

    def test_manual_on_during_active_sensor_error_is_consumed_and_not_replayed_after_clearing_error(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1err=True))
        self.api.wait_for_state(
            lambda current: (
                sensor_map(current)["T1"]["error"] is True
                and output_map(current)["CH1"]["forbidden"] is True
            ),
            timeout=4.0,
        )

        status, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])

        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1=75.0, T1err=False))
        cleared = self._wait_outputs_idle({"CH1": False}, timeout=5.0)
        ch1 = output_map(cleared)["CH1"]
        self.assertFalse(sensor_map(cleared)["T1"]["error"])
        self.assertFalse(ch1["actual"])
        self.assertFalse(ch1.get("manualWant", False))
        self.assertFalse(ch1.get("operatorHoldOff", False))

    def test_manual_off_remains_allowed_during_sensor_error_auto_off(self):
        self._prepare_single_rule("T1", "CH1", 0)
        self._turn_on_outputs(("CH1",))
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(T1err=True))
        self.api.wait_for_state(
            lambda current: (
                sensor_map(current)["T1"]["error"] is True
                and output_map(current)["CH1"]["actual"] is False
                and output_map(current)["CH1"]["forbidden"] is True
            ),
            timeout=5.0,
        )

        response = self._post_json_logged("/api/v1/output/CH1/manual", {"state": False}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

    def test_l_delay_turns_off_after_timeout(self):
        self._prepare_single_rule("L", "CH1", 0)
        self._turn_on_outputs(("CH1",))
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(L=False))
        time.sleep(0.2)
        early = self.api.get_json("/api/v1/state")
        self.assertTrue(output_map(early)["CH1"]["actual"])

        state = self._wait_outputs({"CH1": False}, timeout=4.0)
        ch1 = output_map(state)["CH1"]
        self.assertFalse(ch1["actual"])
        self.assertIn("L", ch1.get("forbidReasons", []))

    def test_f_depends_on_ch2_for_ch1(self):
        self._reset_runtime()
        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=500)
        self._set_sensor_rule("F", 0, enabled=True)
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))

        self._turn_on_outputs(("CH1",))
        time.sleep(0.8)
        with_ch2_off = self.api.get_json("/api/v1/state")
        ch1_before = output_map(with_ch2_off)["CH1"]
        self.assertTrue(ch1_before["actual"])
        self.assertNotIn("F", ch1_before.get("forbidReasons", []))

        self._turn_on_outputs(("CH2",))
        state = self._wait_outputs({"CH1": False, "CH2": True}, timeout=4.0)
        ch1_after = output_map(state)["CH1"]
        self.assertFalse(ch1_after["actual"])
        self.assertIn("F", ch1_after.get("forbidReasons", []))

    def _assert_off_condition_turns_off_only_target_main_channel(self, sensor_id: str,
                                                                 output_id: str,
                                                                 out_idx: int) -> None:
        self._prepare_single_rule(sensor_id, output_id, out_idx)
        self._turn_on_outputs(tuple(output for output, _ in MAIN_OUTPUTS))
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(**off_payload(sensor_id)))

        expected = {current_output: True for current_output, _ in MAIN_OUTPUTS}
        expected[output_id] = False
        state = self._wait_outputs(expected, timeout=6.0)
        outputs = output_map(state)
        if sensor_id != "F":
            self.assertIn(sensor_id, outputs[output_id].get("forbidReasons", []))
        for other_output, _ in MAIN_OUTPUTS:
            if other_output == output_id:
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

    def test_ch4_manual_action_does_not_create_wer_timeout_on_real_api(self):
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
                and output_map(current)["CH4"]["actual"] is True
                and not output_map(current)["CH4"].get("relayPending", False)
                and confirmation_map(current)["WER_CH4"]["emuMode"] == "force_off"
            ),
            timeout=4.0,
        )
        outputs = output_map(state)
        confirmations = confirmation_map(state)
        self.assertTrue(outputs["CH1"]["actual"])
        self.assertTrue(outputs["CH2"]["actual"])
        self.assertTrue(outputs["CH4"]["actual"])
        self.assertIsNone(outputs["CH4"].get("relayError"))
        self.assertFalse(outputs["CH4"].get("relayPending", False))
        self.assertFalse(state.get("safetyAlarmActive", False))
        self.assertEqual(confirmations["WER_CH4"].get("emuMode"), "force_off")
        self.assertFalse(confirmations["WER_CH4"].get("available", True))
        self.assertFalse(confirmations["WER_CH4"].get("timeout", False))
        self.assertFalse(confirmations["WER_CH4"].get("faultLatched", False))
        self.assertNotIn("SAFETY_WER", outputs["CH1"].get("forbidReasons", []))
        self.assertNotIn("SAFETY_WER", outputs["CH2"].get("forbidReasons", []))

    def test_ch5_manual_action_does_not_create_wer_timeout_on_real_api(self):
        self._reset_runtime()
        state = self.api.get_json("/api/v1/state")
        output_cfg = output_config_payload_from_state(state)
        output_cfg["ch5Enabled"] = True
        self._post_json_logged("/api/v1/output/config", output_cfg)

        response = self._post_json_logged("/api/v1/output/CH5/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH5"]["actual"] is True
                and not output_map(current)["CH5"].get("relayPending", False)
            ),
            timeout=4.0,
        )
        ch5 = output_map(state)["CH5"]
        self.assertTrue(ch5["actual"])
        self.assertIsNone(ch5.get("relayError"))
        self.assertFalse(ch5.get("relayPending", False))
        self.assertFalse(ch5.get("pending", False))


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
