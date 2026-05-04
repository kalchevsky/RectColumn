#!/usr/bin/env python3
from __future__ import annotations

import unittest

from api_testlib import LiveEmuApiTestCase, confirmation_map, safe_emu_payload, sensor_map


class EmuSmokeTests(LiveEmuApiTestCase):
    def test_core_endpoints_respond_with_expected_shape(self):
        health = self.api.get_json("/api/v1/health")
        info = self.api.get_json("/api/v1/info")
        diag = self.api.get_json("/api/v1/diag")
        schema = self.api.get_json("/api/v1/schema")
        state = self.api.get_json("/api/v1/state")

        self.assertTrue(health["ok"])
        self.assertTrue(schema["ok"])
        self.assertTrue(state["emu"])
        self.assertIn("sensorIds", schema)
        self.assertIn("outputIds", schema)
        self.assertIn("confirmationIds", schema)
        self.assertIn("pin35Mode", diag)
        self.assertIn("fw", info)
        self.assertIn("confirmations", state)

    def test_emu_set_round_trips_sensor_values_and_confirmation_inputs(self):
        payload = safe_emu_payload(T1=64.5, T2=42.0, P=1100.0, L=True, F=False, WER_CH2=True)
        res = self.api.post_json("/api/v1/emu/set", payload)
        self.assertTrue(res["ok"])

        state = self.api.wait_for_state(
            lambda current: (
                abs(sensor_map(current)["T1"]["value"] - 64.5) < 0.2
                and abs(sensor_map(current)["T2"]["value"] - 42.0) < 0.2
                and abs(sensor_map(current)["P"]["value"] - 1100.0) < 0.2
                and confirmation_map(current)["WER_CH2"]["actual"] is True
            )
        )

        sensors = sensor_map(state)
        confirmations = confirmation_map(state)
        self.assertAlmostEqual(sensors["T1"]["value"], 64.5, places=1)
        self.assertAlmostEqual(sensors["T2"]["value"], 42.0, places=1)
        self.assertAlmostEqual(sensors["P"]["value"], 1100.0, places=1)
        self.assertFalse(sensors["F"]["value"] > 0.5)
        self.assertTrue(confirmations["WER_CH2"]["actual"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
