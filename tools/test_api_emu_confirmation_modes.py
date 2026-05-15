#!/usr/bin/env python3
from __future__ import annotations

import unittest

try:  # pragma: no cover - depends on unittest invocation style
    from .api_testlib import (
        LiveEmuApiTestCase,
        confirmation_map,
        output_map,
        safe_emu_payload,
    )
except ImportError:  # pragma: no cover
    from api_testlib import (  # type: ignore
        LiveEmuApiTestCase,
        confirmation_map,
        output_map,
        safe_emu_payload,
    )


class EmuConfirmationModeTests(LiveEmuApiTestCase):
    def test_auto_mode_follows_output_without_manual_wer_toggle(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0, WER_CH1_mode="auto"))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is True
                and confirmation_map(current)["WER_CH1"]["actual"] is True
                and not output_map(current)["CH1"].get("relayError")
            ),
            timeout=3.0,
        )

        self.assertEqual(confirmation_map(state)["WER_CH1"]["emuMode"], "auto")

    def test_force_off_mode_can_simulate_missing_confirmation(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0, WER_CH1_mode="force_off"))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"].get("relayError") == "timeout"
                or (
                    confirmation_map(current)["WER_CH1"]["faultLatched"] is True
                    and confirmation_map(current)["WER_CH1"]["fault"] == "no_on_confirm"
                )
            ),
            timeout=4.0,
        )

        ch1 = output_map(state)["CH1"]
        self.assertEqual(confirmation_map(state)["WER_CH1"]["emuMode"], "force_off")
        self.assertEqual(ch1.get("relayError"), "timeout")


if __name__ == "__main__":
    unittest.main(verbosity=2)
