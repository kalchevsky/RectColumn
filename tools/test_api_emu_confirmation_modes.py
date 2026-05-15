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
    from .human_report import human_case, record_human_detail
except ImportError:  # pragma: no cover
    from api_testlib import (  # type: ignore
        LiveEmuApiTestCase,
        confirmation_map,
        output_map,
        safe_emu_payload,
    )
    from human_report import human_case, record_human_detail  # type: ignore


class EmuConfirmationModeTests(LiveEmuApiTestCase):
    @human_case(
        title="WER_CH1 в режиме auto повторяет реальное состояние CH1",
        situation="Подтверждение WER для CH1 переведено в режим auto, без ручного принуждения сигнала обратной связи.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать нейтральный T1 и WER_CH1_mode=auto.",
            "Вручную включить CH1.",
            "Дождаться actual=true у CH1 и auto-подтверждения WER_CH1.",
        ],
        expected="CH1 включается без relayError, а WER_CH1.actual следует за выходом и остаётся в emuMode=auto.",
    )
    def test_auto_mode_follows_output_without_manual_wer_toggle(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0, WER_CH1_mode="auto"))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "manual_response", response)
        self.assertTrue(response["accepted"])

        state = self.api.wait_for_state(
            lambda current: (
                output_map(current)["CH1"]["actual"] is True
                and confirmation_map(current)["WER_CH1"]["actual"] is True
                and not output_map(current)["CH1"].get("relayError")
            ),
            timeout=3.0,
        )

        record_human_detail(self, "confirmation_state", confirmation_map(state)["WER_CH1"])
        record_human_detail(self, "output_state", output_map(state)["CH1"])
        self.assertEqual(confirmation_map(state)["WER_CH1"]["emuMode"], "auto")

    @human_case(
        title="WER force_off имитирует отсутствие подтверждения включения",
        situation="CH1 включают вручную, но сигнал WER_CH1 принудительно удерживается в force_off.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать нейтральный T1 и WER_CH1_mode=force_off.",
            "Вручную включить CH1.",
            "Дождаться таймаута подтверждения или faultLatched=no_on_confirm.",
        ],
        expected="CH1 получает relayError=timeout, а WER_CH1 остаётся в emuMode=force_off.",
    )
    def test_force_off_mode_can_simulate_missing_confirmation(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0, WER_CH1_mode="force_off"))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "manual_response", response)
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
        record_human_detail(self, "confirmation_state", confirmation_map(state)["WER_CH1"])
        record_human_detail(self, "output_state", ch1)
        self.assertEqual(confirmation_map(state)["WER_CH1"]["emuMode"], "force_off")
        self.assertEqual(ch1.get("relayError"), "timeout")


if __name__ == "__main__":
    unittest.main(verbosity=2)
