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

    # === FIX BUG 4: relay timeout -> neutral state, button allows retry ===
    @human_case(
        title="После таймаута WER подтверждения канал переходит в нейтральное состояние",
        situation="Реле CH3 включается вручную, но WER_CH3 удерживается в force_off — подтверждение не приходит.",
        steps=[
            "Изолировать CH3 от автоматики (нет правил T3->CH3).",
            "WER_CH3_mode=force_off (нет подтверждения).",
            "Вручную включить CH3.",
            "Дождаться relayError=timeout.",
        ],
        expected=(
            "CH3: relayError='timeout', manualWant=false (кнопка показывает выключено), "
            "actual остаётся в последнем подтверждённом состоянии, "
            "повторная команда ON принимается (можно повторить попытку)."
        ),
    )
    def test_ch3_timeout_goes_neutral_manualWant_cleared_retry_allowed(self):
        # Isolate CH3 from automatic rules so only manual commands control it.
        self.api.post_json("/api/v1/sensor/T3/ctrl", {"outIdx": 2, "enabled": False})

        self.api.post_json("/api/v1/emu/set", safe_emu_payload(
            T3=75.0,
            WER_CH3_mode="force_off",
        ))

        response = self.api.post_json("/api/v1/output/CH3/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "ch3_manual_response", response)
        self.assertTrue(response["accepted"])

        # Wait for timeout condition
        state = self.api.wait_for_state(
            lambda current: output_map(current)["CH3"].get("relayError") == "timeout",
            timeout=6.0,
        )

        ch3 = output_map(state)["CH3"]
        record_human_detail(self, "ch3_state_after_timeout", ch3)

        # Key assertions for Bug 4 fix:
        self.assertEqual(ch3.get("relayError"), "timeout")
        self.assertEqual(ch3.get("relayErrorText"), "таймаут подтверждения реле")
        # FIX: manualWant must be false so the button shows "выключено"
        # (previously it stayed true, making the UI show "включено" incorrectly)
        self.assertFalse(ch3.get("manualWant"), f"manualWant should be cleared after timeout, got: {ch3}")
        # FIX: manualRequest should be false since manualWant is cleared
        self.assertFalse(ch3.get("manualRequest"), f"manualRequest should be false after timeout")
        # FIX: operatorHoldOff should be false so retry ON is allowed
        self.assertFalse(ch3.get("operatorHoldOff"), f"operatorHoldOff should be false to allow retry")

        # Verify retry ON command is accepted
        retry_resp = self.api.post_json("/api/v1/output/CH3/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "ch3_retry_response", retry_resp)
        self.assertTrue(retry_resp["accepted"], f"Retry ON should be accepted, got: {retry_resp}")

        # Verify WER_CH3 fault is logged
        wer3 = confirmation_map(state)["WER_CH3"]
        self.assertEqual(wer3.get("fault"), "no_on_confirm")


if __name__ == "__main__":
    unittest.main(verbosity=2)
