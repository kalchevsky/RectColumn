#!/usr/bin/env python3
from __future__ import annotations

import unittest

from api_testlib import LiveEmuApiTestCase, output_map, safe_emu_payload, sensor_map


class EmuManualLogicTests(LiveEmuApiTestCase):
    def test_manual_on_allowed_when_stop_and_forbids_are_clear(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])
        self.assertEqual(response["detail"], "")

    def test_manual_on_blocked_by_sensor_forbid(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=85.0))

        state = self.api.wait_for_state(lambda current: output_map(current)["CH1"]["forbidden"] is True)
        ch1 = output_map(state)["CH1"]
        self.assertIn("T1", ch1.get("forbidReasons", []))

        status, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")
        self.assertIn("запрещено текущими условиями автоматики", response["userMessage"])
        self.assertIn("T1", response["userMessage"])

    def test_manual_on_blocked_when_control_sensor_is_in_error(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1err=True))

        state = self.api.wait_for_state(
            lambda current: sensor_map(current)["T1"]["error"] is True and output_map(current)["CH1"]["forbidden"] is True
        )
        self.assertIn("T1", output_map(state)["CH1"].get("forbidReasons", []))

        _, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(response["detail"], "forbidden")

    def test_manual_on_blocked_by_stop(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0))
        self.api.post_json("/api/v1/stop", {})

        _, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        self.assertEqual(response["detail"], "stop_active")
        self.assertIn("активен STOP", response["userMessage"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
