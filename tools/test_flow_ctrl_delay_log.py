#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import time
import unittest

try:  # pragma: no cover - depends on unittest invocation style
    from .api_testlib import LiveEmuApiTestCase, output_map, safe_emu_payload, sensor_map
    from .human_report import human_case, record_human_detail
except ImportError:  # pragma: no cover
    from api_testlib import LiveEmuApiTestCase, output_map, safe_emu_payload, sensor_map  # type: ignore
    from human_report import human_case, record_human_detail  # type: ignore


MAIN_OUTPUTS = (("CH1", 0), ("CH2", 1), ("CH3", 2))
ALL_SENSORS = ("T1", "T2", "T3", "dT", "P", "L", "F", "C", "V")


class FlowCtrlDelayLogTests(LiveEmuApiTestCase):
    CTRL_DELAY_MS = 1500

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

    def _set_sensor_rule(self, sensor_id: str, out_idx: int, *, enabled: bool) -> None:
        self._post_json_logged(
            f"/api/v1/sensor/{sensor_id}/ctrl",
            {"outIdx": out_idx, "enabled": enabled, "logic": "heat", "min": 0, "max": 100},
        )

    def _set_sensor_config(self, sensor_id: str, *, enabled: bool, ctrl_delay_ms: int,
                           alarm_delay_ms: int = 0) -> None:
        self._post_json_logged(
            f"/api/v1/sensor/{sensor_id}/config",
            {
                "enabled": enabled,
                "alarmDelayMs": alarm_delay_ms,
                "ctrlDelayMs": ctrl_delay_ms,
            },
        )

    def _disable_all_main_rules(self) -> None:
        for sensor_id in ALL_SENSORS:
            for _, out_idx in MAIN_OUTPUTS:
                self._post_json_logged(
                    f"/api/v1/sensor/{sensor_id}/ctrl",
                    {"outIdx": out_idx, "enabled": False, "logic": "heat", "min": 0, "max": 100},
                )

    def _turn_on_outputs(self, output_ids: tuple[str, ...]) -> None:
        for output_id in output_ids:
            response = self._post_json_logged(
                f"/api/v1/output/{output_id}/manual",
                {"state": True},
                ok_statuses=(200, 409),
            )
            accepted = response.get("accepted", False)
            duplicate = response.get("reason") == "duplicate" or response.get("detail") == "duplicate"
            self.assertTrue(accepted or duplicate, f"manual ON rejected for {output_id}: {response}")
        self.api.wait_for_state(
            lambda current: all(output_map(current)[output_id]["actual"] is True for output_id in output_ids),
            timeout=6.0,
        )

    def _turn_off_all_main_outputs(self) -> None:
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
        for output_id, _ in MAIN_OUTPUTS:
            self._post_json_logged(
                f"/api/v1/output/{output_id}/manual",
                {"state": False},
                ok_statuses=(200, 409),
            )
        self.api.wait_for_state(
            lambda current: all(output_map(current)[output_id]["actual"] is False for output_id, _ in MAIN_OUTPUTS),
            timeout=6.0,
        )

    def _reset_runtime(self) -> None:
        self._post_json_logged("/api/v1/mute", {"muted": True})
        self._post_json_logged("/api/v1/stop", {})
        self._post_json_logged("/api/v1/stop?release=1", {})
        self._post_json_logged("/api/v1/safety/reset", {})
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload())
        self._disable_all_main_rules()
        self._turn_off_all_main_outputs()

    def _capture_trace(self, duration: float, interval: float = 0.15) -> list[dict[str, object]]:
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
                    "autoOff": outputs["CH1"].get("autoOff"),
                    "forbidMask": outputs["CH1"].get("forbidMask"),
                    "effectiveForbidMask": outputs["CH1"].get("effectiveForbidMask"),
                    "forbidReasons": outputs["CH1"].get("forbidReasons", []),
                },
                "CH2": {
                    "actual": outputs["CH2"]["actual"],
                    "forbidden": outputs["CH2"]["forbidden"],
                    "forbidReasons": outputs["CH2"].get("forbidReasons", []),
                },
                "CH3": {
                    "actual": outputs["CH3"]["actual"],
                    "forbidden": outputs["CH3"]["forbidden"],
                    "forbidReasons": outputs["CH3"].get("forbidReasons", []),
                },
                "F": {
                    "enabled": sensors["F"]["enabled"],
                    "value": sensors["F"]["value"],
                    "ctrlDelayMs": sensors["F"]["ctrlDelayMs"],
                },
            })
            time.sleep(interval)
        return trace

    @human_case(
        title="ctrlDelayMs задерживает отключение CH1 по потоку и пишет подробный RELAY_OFF",
        situation="Для F включено только правило F -> CH1. CH1, CH2 и CH3 включаются вручную, затем пропадает проток.",
        steps=[
            "Сбросить runtime и отключить все посторонние main-правила.",
            "Включить F и задать ctrlDelayMs для защиты CH1.",
            "Включить CH1, CH2 и CH3, очистить журнал и подать F=false.",
            "Проверить, что до истечения ctrlDelayMs CH1 не выключается и нет RELAY_OFF по F.",
            "После истечения ctrlDelayMs проверить отключение только CH1 и наличие диагностической строки.",
        ],
        expected="До таймаута CH1 остаётся включённым без forbid/autoOff. После таймаута выключается только CH1, а лог содержит RELAY_OFF source=channel_control sensorId=F и диагностические поля задержки.",
    )
    def test_flow_protection_delay_and_diagnostic_log(self):
        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=self.CTRL_DELAY_MS, alarm_delay_ms=300)
        self._set_sensor_rule("F", 0, enabled=True)
        self._set_sensor_rule("F", 1, enabled=False)
        self._set_sensor_rule("F", 2, enabled=False)

        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=True))
        self.api.wait_for_state(
            lambda current: sensor_map(current)["F"]["value"] == 1,
            timeout=3.0,
        )

        self._turn_on_outputs(("CH1", "CH2", "CH3"))
        self.api.delete_json("/api/v1/log")
        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))

        early_trace = self._capture_trace(duration=0.8)
        before_timeout = self.api.get_json("/api/v1/state")
        early_log_status, early_log_text, early_log_headers = self.api.request_text("/api/v1/log/download")

        outputs_before = output_map(before_timeout)
        self.assertEqual(early_log_status, 200)
        self.assertTrue(outputs_before["CH1"]["actual"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertFalse(outputs_before["CH1"]["forbidden"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertFalse(outputs_before["CH1"].get("autoOff", False), msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertTrue(outputs_before["CH2"]["actual"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertTrue(outputs_before["CH3"]["actual"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertNotIn("RELAY_OFF source=channel_control", early_log_text)

        final_state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is False
                and output_map(current)["CH2"]["actual"] is True
                and output_map(current)["CH3"]["actual"] is True
            ),
            timeout=5.0,
        )
        final_outputs = output_map(final_state)
        final_log_status, final_log_text, final_log_headers = self.api.request_text("/api/v1/log/download")
        relay_off_line_match = re.search(
            r"(RELAY_OFF source=channel_control channel=CH1[^\r\n]*)",
            final_log_text,
        )

        record_human_detail(self, "post_history", self._post_history)
        record_human_detail(self, "early_trace", early_trace)
        record_human_detail(self, "before_timeout_state", before_timeout)
        record_human_detail(self, "final_state", final_state)
        record_human_detail(self, "early_log_headers", dict(early_log_headers.items()))
        record_human_detail(self, "final_log_headers", dict(final_log_headers.items()))
        record_human_detail(self, "early_log_excerpt", early_log_text[:1500])
        record_human_detail(self, "final_log_excerpt", final_log_text[:2500])

        self.assertEqual(final_log_status, 200)
        self.assertFalse(final_outputs["CH1"]["actual"])
        self.assertTrue(final_outputs["CH2"]["actual"])
        self.assertTrue(final_outputs["CH3"]["actual"])
        self.assertNotIn("F", final_outputs["CH2"].get("forbidReasons", []))
        self.assertNotIn("F", final_outputs["CH3"].get("forbidReasons", []))
        self.assertIn("RELAY_OFF source=channel_control channel=CH1", final_log_text)
        self.assertIn("sensorId=F", final_log_text)
        self.assertIn(f"ctrlDelayMs={self.CTRL_DELAY_MS}", final_log_text)
        self.assertIn("ruleState=logic=cool,min=0.50,max=2.00", final_log_text)
        self.assertIn("sensorValue=0.00", final_log_text)
        self.assertIsNotNone(relay_off_line_match, msg=final_log_text)
        relay_off_line = relay_off_line_match.group(1)
        elapsed_match = re.search(r"elapsedMs=(\d+)", relay_off_line)
        self.assertIsNotNone(elapsed_match, msg=relay_off_line)
        self.assertGreaterEqual(int(elapsed_match.group(1)), self.CTRL_DELAY_MS)

    @human_case(
        title="Повторное включение датчика F через UI запускает новую задержку защиты",
        situation="Датчик F выключен через интерфейс, при этом CH2 уже включён, цепь датчика разомкнута и правило F -> CH1 активно.",
        steps=[
            "Отключить F через sensor/config при активном правиле F -> CH1.",
            "Подать F=false и включить CH1 вместе с CH2.",
            "Очистить журнал и снова включить F через sensor/config.",
            "Проверить, что CH1 не выключается сразу после enable.",
            "После истечения ctrlDelayMs убедиться в отключении CH1 и наличии RELAY_OFF по F.",
        ],
        expected="Включение датчика не переиспользует старое состояние runtime: защита стартует заново от момента enable и не вызывает мгновенный shutdown.",
    )
    def test_enabling_flow_sensor_via_ui_does_not_bypass_delay(self):
        self._set_sensor_config("F", enabled=False, ctrl_delay_ms=self.CTRL_DELAY_MS, alarm_delay_ms=300)
        self._set_sensor_rule("F", 0, enabled=True)
        self._set_sensor_rule("F", 1, enabled=False)
        self._set_sensor_rule("F", 2, enabled=False)

        self._post_json_logged("/api/v1/emu/set", safe_emu_payload(F=False))
        self._turn_on_outputs(("CH1", "CH2"))
        self.api.delete_json("/api/v1/log")

        self._set_sensor_config("F", enabled=True, ctrl_delay_ms=self.CTRL_DELAY_MS, alarm_delay_ms=300)
        early_trace = self._capture_trace(duration=0.7)
        early_state = self.api.get_json("/api/v1/state")
        early_log_status, early_log_text, _ = self.api.request_text("/api/v1/log/download")

        record_human_detail(self, "enable_sensor_early_trace", early_trace)
        record_human_detail(self, "enable_sensor_early_state", early_state)
        record_human_detail(self, "enable_sensor_early_log_excerpt", early_log_text[:1500])

        self.assertEqual(early_log_status, 200)
        self.assertTrue(output_map(early_state)["CH1"]["actual"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertFalse(output_map(early_state)["CH1"]["forbidden"], msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertFalse(output_map(early_state)["CH1"].get("autoOff", False), msg=json.dumps(early_trace, ensure_ascii=False, indent=2))
        self.assertNotIn("RELAY_OFF source=channel_control", early_log_text)

        final_state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is False
                and output_map(current)["CH2"]["actual"] is True
            ),
            timeout=5.0,
        )
        final_log_status, final_log_text, _ = self.api.request_text("/api/v1/log/download")
        record_human_detail(self, "enable_sensor_final_state", final_state)
        record_human_detail(self, "enable_sensor_final_log_excerpt", final_log_text[:2500])

        self.assertEqual(final_log_status, 200)
        self.assertIn("RELAY_OFF source=channel_control channel=CH1", final_log_text)
        self.assertIn("sensorId=F", final_log_text)
        self.assertIn(f"ctrlDelayMs={self.CTRL_DELAY_MS}", final_log_text)
        self.assertIn("F", output_map(final_state)["CH1"].get("forbidReasons", []))


if __name__ == "__main__":
    unittest.main(verbosity=2)
