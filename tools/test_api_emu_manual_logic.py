#!/usr/bin/env python3
from __future__ import annotations

import unittest

try:  # pragma: no cover - depends on unittest invocation style
    from .api_testlib import LiveEmuApiTestCase, output_map, safe_emu_payload, sensor_map
    from .human_report import human_case, record_human_detail
except ImportError:  # pragma: no cover
    from api_testlib import LiveEmuApiTestCase, output_map, safe_emu_payload, sensor_map  # type: ignore
    from human_report import human_case, record_human_detail  # type: ignore


class EmuManualLogicTests(LiveEmuApiTestCase):
    @human_case(
        title="Ручное включение CH1 разрешено при чистом состоянии автоматики",
        situation="Для CH1 оставлено только правило по датчику T1, датчик находится в нейтральной зоне, STOP и автоматические запреты отсутствуют.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать нейтральное значение T1=75.0.",
            "Отправить manual ON для CH1 через API.",
        ],
        expected="API принимает manual ON, а detail остаётся пустым.",
    )
    def test_manual_on_allowed_when_stop_and_forbids_are_clear(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0))

        response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "manual_response", response)
        self.assertTrue(response["accepted"])
        self.assertEqual(response["detail"], "")

    @human_case(
        title="Ошибка по T1 запрещает manual ON и возвращает понятную причину",
        situation="У CH1 включено управление по T1, а значение T1 переведено в зону auto-off выше максимума.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать значение T1=85.0, чтобы автоматика запретила CH1.",
            "Дождаться forbidReasons для CH1.",
            "Попробовать вручную включить CH1 через API.",
        ],
        expected="API отвечает HTTP 409, accepted=false, detail=forbidden, а userMessage содержит CH1 и датчик T1.",
    )
    def test_manual_on_blocked_by_sensor_forbid(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=85.0))

        state = self.api.wait_for_state(lambda current: output_map(current)["CH1"]["forbidden"] is True)
        ch1 = output_map(state)["CH1"]
        record_human_detail(self, "output_state_before_manual", ch1)
        self.assertIn("T1", ch1.get("forbidReasons", []))

        status, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        record_human_detail(self, "manual_http_status", status)
        record_human_detail(self, "manual_response", response)
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")
        self.assertIn("Команда CH1: включение", response["userMessage"])
        self.assertIn("запрещено автоматикой", response["userMessage"])
        self.assertIn("T1", response["userMessage"])

    @human_case(
        title="Ошибка датчика T1 блокирует manual ON только на затронутом канале",
        situation="Правило CH1 <- T1 активно, а сам T1 переведён в состояние sensor error.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать флаг ошибки T1err=true.",
            "Дождаться, пока CH1 станет forbidden.",
            "Попробовать вручную включить CH1.",
        ],
        expected="Manual ON отклоняется с detail=forbidden и с упоминанием T1 в ответе API.",
    )
    def test_manual_on_blocked_when_control_sensor_is_in_error(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1err=True))

        state = self.api.wait_for_state(
            lambda current: sensor_map(current)["T1"]["error"] is True
            and output_map(current)["CH1"]["forbidden"] is True
        )
        record_human_detail(self, "sensor_state", sensor_map(state)["T1"])
        record_human_detail(self, "output_state_before_manual", output_map(state)["CH1"])
        self.assertIn("T1", output_map(state)["CH1"].get("forbidReasons", []))

        status, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(409,),
        )
        record_human_detail(self, "manual_http_status", status)
        record_human_detail(self, "manual_response", response)
        self.assertEqual(status, 409)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "forbidden")
        self.assertIn("Команда CH1: включение", response["userMessage"])
        self.assertIn("T1", response["userMessage"])

    @human_case(
        title="Manual OFF остаётся разрешённым во время auto-off по ошибке датчика",
        situation="CH1 уже включён вручную, затем датчик T1 уходит в ошибку и автоматика выключает канал.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Включить CH1 вручную в нейтральной зоне T1.",
            "Перевести T1 в состояние ошибки.",
            "После auto-off отправить manual OFF.",
        ],
        expected="Запрос manual OFF принимается, даже если CH1 уже выключен автоматикой по ошибке T1.",
    )
    def test_manual_off_remains_allowed_when_control_sensor_is_in_error(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=75.0))
        response_on = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200,))
        record_human_detail(self, "manual_on_response", response_on)
        self.assertTrue(response_on["accepted"])
        self.api.wait_for_state(lambda current: output_map(current)["CH1"]["actual"] is True)

        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1err=True))
        error_state = self.api.wait_for_state(
            lambda current: sensor_map(current)["T1"]["error"] is True
            and output_map(current)["CH1"]["actual"] is False
        )
        record_human_detail(self, "state_after_error", {
            "sensor": sensor_map(error_state)["T1"],
            "output": output_map(error_state)["CH1"],
        })

        _, response_off = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": False},
            ok_statuses=(200,),
        )
        record_human_detail(self, "manual_off_response", response_off)
        self.assertTrue(response_off["accepted"])

    @human_case(
        title="STOP отдельно блокирует manual ON",
        situation="Автоматические запреты очищены, но активирован глобальный STOP.",
        steps=[
            "Изолировать правило CH1 <- T1 и подать нейтральный T1.",
            "Активировать STOP.",
            "Попробовать вручную включить CH1.",
        ],
        expected="API отклоняет manual ON с detail=stop_active и сообщением про активный STOP.",
    )
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
        record_human_detail(self, "manual_response", response)
        self.assertEqual(response["detail"], "stop_active")
        self.assertIn("активен STOP", response["userMessage"])

    @human_case(
        title="Manual OFF сообщает, что канал удерживается auto-on",
        situation="T1 находится ниже минимума и автоматически требует включить CH1. Оператор пытается выключить CH1 вручную.",
        steps=[
            "Изолировать правило CH1 <- T1.",
            "Подать T1=65.0 и дождаться auto-on CH1.",
            "Отправить manual OFF для CH1.",
        ],
        expected="API не принимает выключение, detail=auto_on_active, а userMessage объясняет, что CH1 удерживается автоматикой по T1.",
    )
    def test_manual_off_reports_specific_auto_reason(self):
        self.isolate_ch1_t1_rule()
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=65.0))
        self.api.wait_for_state(lambda current: output_map(current)["CH1"]["actual"] is True)

        _, response = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": False},
            ok_statuses=(200,),
        )
        record_human_detail(self, "manual_response", response)
        self.assertFalse(response["accepted"])
        self.assertEqual(response["detail"], "auto_on_active")
        self.assertIn("Команда CH1: выключение", response["userMessage"])
        self.assertIn("T1", response["userMessage"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
