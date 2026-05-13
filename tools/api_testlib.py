#!/usr/bin/env python3
"""
Shared helpers for live RectColumn API tests in EMU_MODE.

The tests are intentionally host-side and talk to the controller over HTTP.
Set RECTCOLUMN_BASE_URL when the device is not on the default AP address.
"""

from __future__ import annotations

import json
import os
import sys
import time
import unittest
import urllib.error
import urllib.request
from typing import Any, Callable


DEFAULT_BASE_URL = "http://192.168.4.1"


class RectColumnApi:
    def __init__(self, base_url: str | None = None, timeout: float = 6.0):
        self.base_url = (base_url or os.environ.get("RECTCOLUMN_BASE_URL") or DEFAULT_BASE_URL).rstrip("/")
        self.timeout = timeout

    def _url(self, path: str) -> str:
        if path.startswith("http://") or path.startswith("https://"):
            return path
        return self.base_url + path

    def request_json(self, path: str, *, method: str = "GET", payload: Any | None = None,
                     ok_statuses: tuple[int, ...] = (200,)) -> tuple[int, Any]:
        body = None
        headers = {}
        if payload is not None:
            body = json.dumps(payload).encode("utf-8")
            headers["Content-Type"] = "application/json"

        req = urllib.request.Request(self._url(path), data=body, headers=headers, method=method)
        try:
            with urllib.request.urlopen(req, timeout=self.timeout) as resp:
                raw = resp.read().decode("utf-8", errors="replace")
                data = json.loads(raw) if raw else None
                status = resp.status
        except urllib.error.HTTPError as exc:
            raw = exc.read().decode("utf-8", errors="replace")
            try:
                data = json.loads(raw) if raw else None
            except json.JSONDecodeError:
                data = {"ok": False, "raw": raw}
            status = exc.code
        except Exception as exc:  # pragma: no cover - used only for live device access
            raise unittest.SkipTest(f"Device is unreachable at {self.base_url}: {exc}") from exc

        if status not in ok_statuses:
            raise AssertionError(f"{method} {path} returned HTTP {status}: {data}")
        return status, data

    def get_json(self, path: str, *, ok_statuses: tuple[int, ...] = (200,)) -> Any:
        return self.request_json(path, method="GET", ok_statuses=ok_statuses)[1]

    def post_json(self, path: str, payload: Any | None = None,
                  *, ok_statuses: tuple[int, ...] = (200,)) -> Any:
        return self.request_json(path, method="POST", payload=payload, ok_statuses=ok_statuses)[1]

    def delete_json(self, path: str, *, ok_statuses: tuple[int, ...] = (200,)) -> Any:
        return self.request_json(path, method="DELETE", ok_statuses=ok_statuses)[1]

    def wait_for_state(self, predicate: Callable[[dict[str, Any]], bool], *,
                       timeout: float = 4.0, interval: float = 0.2) -> dict[str, Any]:
        deadline = time.time() + timeout
        last = None
        while time.time() < deadline:
            last = self.get_json("/api/v1/state")
            if predicate(last):
                return last
            time.sleep(interval)
        raise AssertionError(f"Timed out waiting for state predicate. Last state: {json.dumps(last, ensure_ascii=False)[:1200]}")


def sensor_map(state: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {sensor["id"]: sensor for sensor in state.get("sensors", [])}


def output_map(state: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {output["id"]: output for output in state.get("outputs", [])}


def confirmation_map(state: dict[str, Any]) -> dict[str, dict[str, Any]]:
    return {item["id"]: item for item in state.get("confirmations", [])}


def runtime_output_map(outputs: Any) -> dict[str, dict[str, Any]]:
    if isinstance(outputs, list):
        return {item.get("id", ""): item for item in outputs if isinstance(item, dict) and item.get("id")}
    return {}


def output_config_payload_from_state(state: dict[str, Any]) -> dict[str, Any]:
    outputs = state.get("outputs", [])
    payload_outputs = []
    for output in outputs:
        output_id = output.get("id")
        if output_id not in ("CH1", "CH2", "CH3"):
            continue
        payload_outputs.append({
            "id": output_id,
            "mode": output.get("mode", "heat"),
        })
    return {
        "outputs": payload_outputs,
        "ch4Enabled": state.get("ch4Enabled", True),
        "ch5Enabled": state.get("ch5Enabled", True),
        "soundMuted": state.get("muted", False),
    }


def safe_emu_payload(**overrides: Any) -> dict[str, Any]:
    payload = {
        "T1": 75.0,
        "T2": 30.0,
        "T3": 25.0,
        "P": 1013.0,
        "L": True,
        "F": True,
        "C": 500.0,
        "V": 0.0,
        "T1err": False,
        "T2err": False,
        "T3err": False,
        "WER_CH1_mode": "auto",
        "WER_CH2_mode": "auto",
        "WER_CH3_mode": "auto",
        "WER_CH4_mode": "auto",
    }
    payload.update(overrides)
    return payload


class LiveEmuApiTestCase(unittest.TestCase):
    api: RectColumnApi

    @classmethod
    def setUpClass(cls):
        cls.api = RectColumnApi()
        health = cls.api.get_json("/api/v1/health")
        state = cls.api.get_json("/api/v1/state")
        if not health.get("ok"):
            raise unittest.SkipTest(f"Health endpoint did not return ok=true at {cls.api.base_url}")
        if not state.get("emu"):
            raise unittest.SkipTest(f"Device at {cls.api.base_url} is not running in EMU_MODE")

    def snapshot_config(self) -> dict[str, Any]:
        state = self.api.get_json("/api/v1/state")
        return {
            "state": state,
            "output_config": self.api.get_json("/api/v1/output/config"),
            "output_config_payload": output_config_payload_from_state(state),
        }

    def restore_snapshot(self, snapshot: dict[str, Any]) -> None:
        state = snapshot.get("state", {})
        try:
            for sensor in state.get("sensors", []):
                self.api.post_json(
                    f"/api/v1/sensor/{sensor['id']}/config",
                    {
                        "enabled": sensor.get("enabled", True),
                        "periodMs": sensor.get("periodMs", 1000),
                        "alarmDelayMs": sensor.get("alarmDelayMs", 0),
                        "ctrlDelayMs": sensor.get("ctrlDelayMs", 0),
                    },
                )
                for idx, alarm in enumerate(sensor.get("alarms", [])):
                    self.api.post_json(
                        f"/api/v1/sensor/{sensor['id']}/alarm",
                        {
                            "idx": idx,
                            "enabled": alarm.get("enabled", False),
                            "threshold": alarm.get("threshold", 0),
                            "isMax": alarm.get("isMax", True),
                        },
                    )
                for ctrl in sensor.get("ctrl", []):
                    self.api.post_json(
                        f"/api/v1/sensor/{sensor['id']}/ctrl",
                        {
                            "outIdx": ctrl.get("outIdx", 0),
                            "enabled": ctrl.get("enabled", False),
                            "logic": ctrl.get("logic", "heat"),
                            "min": ctrl.get("min", 0),
                            "max": ctrl.get("max", 100),
                        },
                    )

            output_config_payload = snapshot.get("output_config_payload") or output_config_payload_from_state(state)
            self.api.post_json("/api/v1/output/config", output_config_payload)
            self.api.post_json("/api/v1/safety/reset", {})
            self.api.post_json("/api/v1/stop?release=1", {})
        except Exception as exc:  # pragma: no cover - cleanup path
            sys.stderr.write(f"[cleanup-warning] restore_snapshot failed: {exc}\n")
            try:
                self.api.request_json("/api/v1/safety/reset", method="POST", payload={}, ok_statuses=(200, 400, 404))
                self.api.request_json("/api/v1/stop?release=1", method="POST", payload={}, ok_statuses=(200, 400, 404))
            except Exception as inner_exc:
                sys.stderr.write(f"[cleanup-warning] fallback reset failed: {inner_exc}\n")

    def isolate_ch1_t1_rule(self) -> None:
        snap = self.snapshot_config()
        self.addCleanup(self.restore_snapshot, snap)

        self.api.post_json("/api/v1/stop?release=1", {})
        self.api.post_json("/api/v1/safety/reset", {})

        for sensor_id in ("T1", "T2", "T3", "dT", "P", "L", "F", "C", "V"):
            self.api.post_json(
                f"/api/v1/sensor/{sensor_id}/ctrl",
                {"outIdx": 0, "enabled": False, "logic": "heat", "min": 0, "max": 100},
            )

        self.api.post_json(
            "/api/v1/sensor/T1/ctrl",
            {"outIdx": 0, "enabled": True, "logic": "heat", "min": 70.0, "max": 80.0},
        )
        self.api.post_json("/api/v1/emu/set", safe_emu_payload())
