#!/usr/bin/env python3
from __future__ import annotations

import time

try:  # pragma: no cover - depends on unittest invocation style
    from .api_testlib import LiveEmuApiTestCase, safe_emu_payload, sensor_map
except ImportError:  # pragma: no cover
    from api_testlib import LiveEmuApiTestCase, safe_emu_payload, sensor_map  # type: ignore


class NotifyFailureEmuTests(LiveEmuApiTestCase):
    DEAD_URL = "http://10.255.255.1/rectcolumn-dead-topic"

    def setUp(self) -> None:
        super().setUp()
        self._snapshot = self.snapshot_config()
        self.addCleanup(self.restore_snapshot, self._snapshot)
        self._notify_cfg = self.api.get_json("/api/v1/notify/config")
        self.addCleanup(
            self.api.post_json,
            "/api/v1/notify/config",
            {
                "enabled": self._notify_cfg.get("enabled", False),
                "url": self._notify_cfg.get("url", ""),
                "token": self._notify_cfg.get("token", ""),
            },
        )

    def _prepare_notify_alarm_source(self) -> None:
        output_cfg = self._snapshot.get("output_config_payload", {})
        output_cfg["ch5Enabled"] = True
        output_cfg["soundMuted"] = False
        self.api.post_json("/api/v1/output/config", output_cfg)
        self.api.post_json(
            "/api/v1/notify/config",
            {
                "enabled": True,
                "url": self.DEAD_URL,
                "token": "",
            },
        )
        self.api.post_json(
            "/api/v1/sensor/T1/config",
            {
                "enabled": True,
                "periodMs": 1000,
                "alarmDelayMs": 0,
                "ctrlDelayMs": 0,
            },
        )
        self.api.post_json(
            "/api/v1/sensor/T1/alarm",
            {
                "idx": 0,
                "enabled": True,
                "threshold": 60.0,
                "isMax": True,
            },
        )
        self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=40.0))
        self.api.wait_for_state(
            lambda current: not sensor_map(current)["T1"]["alarms"][0]["triggered"],
            timeout=2.0,
        )

    def test_notify_worker_queue_limits_heap_growth_and_reports_drops(self):
        self._prepare_notify_alarm_source()

        before_diag = self.api.get_json("/api/v1/diag")
        before_status = self.api.get_json("/api/v1/notify/status")

        for _ in range(10):
            self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=90.0))
            time.sleep(0.05)
            self.api.post_json("/api/v1/emu/set", safe_emu_payload(T1=40.0))
            time.sleep(0.05)

        time.sleep(0.5)
        after_diag = self.api.get_json("/api/v1/diag")
        after_status = self.api.get_json("/api/v1/notify/status")

        before_heap = int(before_diag.get("freeHeap", 0))
        after_heap = int(after_diag.get("freeHeap", 0))
        heap_drop = before_heap - after_heap

        self.assertTrue(after_status.get("workerReady"))
        self.assertEqual(after_status.get("queueSize"), 8)
        self.assertLessEqual(heap_drop, 1024)
        self.assertGreaterEqual(after_status.get("droppedCount", 0), before_status.get("droppedCount", 0))
        self.assertLessEqual(after_status.get("queueDepth", 0), after_status.get("queueSize", 0))


if __name__ == "__main__":
    import unittest

    unittest.main(verbosity=2)
