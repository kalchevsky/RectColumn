#!/usr/bin/env python3
from __future__ import annotations

import unittest
from pathlib import Path

from api_testlib import LiveEmuApiTestCase


class EmuPanelStaticContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html = (Path(__file__).resolve().parents[1] / "emupanel-v3.html").read_text(encoding="utf-8", errors="ignore")

    def test_panel_knows_current_safety_endpoint(self):
        self.assertIn("/api/v1/safety/reset", self.html)

    def test_panel_reads_current_output_fields(self):
        for token in ("effectiveForbidMask", "forbidReasons", "forbidReasonText", "relayError", "relayErrorText", "confirmExpected"):
            self.assertIn(token, self.html)

    def test_panel_reads_current_confirmation_fault_fields(self):
        for token in ("faultLatched", "fault", "emuModeText", "WER_CH4_mode"):
            self.assertIn(token, self.html)


class EmuPanelLiveContractTests(LiveEmuApiTestCase):
    def test_live_api_exposes_fields_the_panel_uses(self):
        state = self.api.get_json("/api/v1/state")
        self.assertTrue(state["outputs"])
        self.assertTrue(state["confirmations"])

        output = state["outputs"][0]
        confirmation = state["confirmations"][0]

        for key in ("effectiveForbidMask", "forbidReasons", "forbidReasonText", "confirmExpected", "relayPending"):
            self.assertIn(key, output)
        for key in ("faultLatched", "fault", "faultText", "timeoutMs", "debounceMs", "emuMode", "emuModeText"):
            self.assertIn(key, confirmation)


if __name__ == "__main__":
    unittest.main(verbosity=2)
