#!/usr/bin/env python3
"""
Live RectColumn API audit.

The script can run a read-only audit by default and active checks with
--allow-write. In active mode it can:
  - release STOP and reset latched safety faults;
  - mute sound outputs to avoid self-interference during relay tests;
  - toggle outputs through /api/v1/output/{id}/manual;
  - test STOP behavior;
  - run a small set of EMU-only scenarios when --emu-scenarios is enabled.

Report output is Markdown so it can be attached directly to issue notes.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Callable

from api_testlib import (
    RectColumnApi,
    confirmation_map,
    output_map,
    safe_emu_payload,
    sensor_map,
)


DEFAULT_TIMEOUT = 6.0


@dataclass
class AuditResult:
    name: str
    status: str
    detail: str
    data: dict[str, Any] = field(default_factory=dict)


class RectColumnAuditor:
    def __init__(self, api: RectColumnApi, *, allow_write: bool, emu_scenarios: bool, timeout: float):
        self.api = api
        self.allow_write = allow_write
        self.emu_scenarios = emu_scenarios
        self.timeout = timeout
        self.results: list[AuditResult] = []
        self.info: dict[str, Any] = {}
        self.diag: dict[str, Any] = {}
        self.schema: dict[str, Any] = {}
        self.state: dict[str, Any] = {}
        self._initial_muted: bool | None = None

    def add(self, name: str, status: str, detail: str, **data: Any) -> None:
        self.results.append(AuditResult(name=name, status=status, detail=detail, data=data))

    def fetch_baseline(self) -> None:
        health = self.api.get_json("/api/v1/health")
        self.info = self.api.get_json("/api/v1/info")
        self.diag = self.api.get_json("/api/v1/diag")
        self.schema = self.api.get_json("/api/v1/schema")
        self.state = self.api.get_json("/api/v1/state")

        if health.get("ok"):
            self.add("health", "PASS", "Health endpoint returned ok=true")
        else:
            self.add("health", "FAIL", "Health endpoint did not return ok=true", response=health)

        if self.schema.get("ok") and self.schema.get("outputIds") and self.schema.get("sensorIds"):
            self.add(
                "schema",
                "PASS",
                f"Schema lists {len(self.schema.get('outputIds', []))} outputs and {len(self.schema.get('sensorIds', []))} sensors",
            )
        else:
            self.add("schema", "FAIL", "Schema response is incomplete", response=self.schema)

    def run(self) -> None:
        self.fetch_baseline()
        self.audit_sensors()
        self.audit_wifi_scan()

        if self.allow_write:
            self.run_active_checks()
        else:
            self.add("active-checks", "SKIP", "Run again with --allow-write to test STOP and manual output control")

        if self.emu_scenarios:
            if self.state.get("emu"):
                self.run_emu_checks()
            else:
                self.add("emu-scenarios", "SKIP", "EMU scenarios requested, but the device is running real hardware mode")

        self.state = self.api.get_json("/api/v1/state")

    def audit_sensors(self) -> None:
        sensors = self.state.get("sensors", [])
        if not sensors:
            self.add("sensors", "FAIL", "State payload does not contain sensors[]")
            return

        failing = 0
        warnings = 0
        for sensor in sensors:
            sensor_id = sensor.get("id", "?")
            if not sensor.get("enabled", True):
                warnings += 1
                self.add(f"sensor:{sensor_id}", "WARN", "Sensor is disabled in configuration", sensor=sensor)
                continue
            if sensor.get("hwLimited"):
                warnings += 1
                self.add(
                    f"sensor:{sensor_id}",
                    "WARN",
                    sensor.get("note", "Sensor is limited by hardware/profile configuration"),
                    sensor=sensor,
                )
                continue
            if sensor.get("error") or not sensor.get("present", True):
                failing += 1
                self.add(
                    f"sensor:{sensor_id}",
                    "FAIL",
                    sensor.get("note", "Sensor is in error or not present"),
                    sensor=sensor,
                )
                continue
            if sensor.get("stale"):
                warnings += 1
                self.add(f"sensor:{sensor_id}", "WARN", "Sensor value is stale", sensor=sensor)
                continue

        if failing == 0 and warnings == 0:
            self.add("sensor-summary", "PASS", "All enabled sensors are present and report valid values")
        else:
            self.add(
                "sensor-summary",
                "WARN" if failing == 0 else "FAIL",
                f"Sensors with issues: fail={failing}, warn={warnings}",
            )

    def audit_wifi_scan(self) -> None:
        try:
            scan = self.api.get_json("/api/v1/wifi/scan")
        except Exception as exc:
            self.add("wifi-scan", "FAIL", f"WiFi scan request failed: {exc}")
            return

        self.info = self.api.get_json("/api/v1/info")
        self.diag = self.api.get_json("/api/v1/diag")

        if not isinstance(scan, list):
            self.add("wifi-scan", "FAIL", "WiFi scan response is not a JSON array", response=scan)
            return

        last_status = self.info.get("lastScanStatusText", self.diag.get("lastScanStatusText", "unknown"))
        count = len(scan)
        if count > 0:
            self.add("wifi-scan", "PASS", f"WiFi scan returned {count} visible network(s)", networks=scan[:10], scan_status=last_status)
            return

        detail = f"WiFi scan returned 0 networks; last scan status={last_status}"
        if str(last_status).startswith("WIFI_SCAN_FAILED"):
            self.add("wifi-scan", "FAIL", detail, scan_status=last_status)
        else:
            self.add("wifi-scan", "WARN", detail, scan_status=last_status)

    def run_active_checks(self) -> None:
        self._initial_muted = bool(self.info.get("muted", False))
        try:
            self._prepare_active_session()
            self.test_stop_behavior()
            for output_id in self.schema.get("outputIds", []):
                self.test_output_cycle(output_id)
        finally:
            self._cleanup_active_session()

    def run_emu_checks(self) -> None:
        try:
            self.api.post_json("/api/v1/emu/set", safe_emu_payload())
            self.api.post_json("/api/v1/stop?release=1", {})
            self.api.post_json("/api/v1/safety/reset", {})

            for output_id in ("CH1", "CH2", "CH3", "CH4"):
                confirm_id = f"WER_{output_id}"
                response = self.api.post_json(f"/api/v1/output/{output_id}/manual", {"state": True}, ok_statuses=(200, 409))
                if not response.get("accepted"):
                    self.add(
                        f"emu:{output_id}:manual-on",
                        "FAIL",
                        f"EMU manual ON was rejected: {response.get('detailText') or response.get('detail')}",
                        response=response,
                    )
                    continue

                state = self.api.wait_for_state(
                    lambda current, oid=output_id, cid=confirm_id: (
                        output_map(current)[oid]["actual"] is True
                        and confirmation_map(current)[cid]["actual"] is True
                    ),
                    timeout=self.timeout,
                )
                self.add(
                    f"emu:{output_id}:manual-on",
                    "PASS",
                    "EMU auto-confirm followed the output",
                    output=output_map(state)[output_id],
                    confirm=confirmation_map(state)[confirm_id],
                )
                self.api.post_json(f"/api/v1/output/{output_id}/manual", {"state": False}, ok_statuses=(200,))

            self.api.post_json("/api/v1/emu/set", safe_emu_payload(WER_CH1_mode="force_off"))
            response = self.api.post_json("/api/v1/output/CH1/manual", {"state": True}, ok_statuses=(200, 409))
            if not response.get("accepted"):
                self.add("emu:confirm-timeout", "FAIL", "Could not start CH1 timeout scenario", response=response)
            else:
                state = self.api.wait_for_state(
                    lambda current: (
                        confirmation_map(current)["WER_CH1"]["faultLatched"] is True
                        and confirmation_map(current)["WER_CH1"]["fault"] == "no_on_confirm"
                    ),
                    timeout=max(self.timeout, 6.0),
                )
                self.add(
                    "emu:confirm-timeout",
                    "PASS",
                    "EMU timeout scenario latched the expected confirmation fault",
                    output=output_map(state)["CH1"],
                    confirm=confirmation_map(state)["WER_CH1"],
                )
        finally:
            try:
                self.api.post_json("/api/v1/emu/set", safe_emu_payload())
            except Exception:
                pass

    def _prepare_active_session(self) -> None:
        self.api.post_json("/api/v1/stop?release=1", {})
        self.api.post_json("/api/v1/safety/reset", {})
        self.api.post_json("/api/v1/mute", {"muted": True})

    def _cleanup_active_session(self) -> None:
        for output_id in self.schema.get("outputIds", []):
            try:
                self.api.post_json(f"/api/v1/output/{output_id}/manual", {"state": False}, ok_statuses=(200, 409))
            except Exception:
                pass
        try:
            self.api.post_json("/api/v1/stop?release=1", {})
        except Exception:
            pass
        try:
            self.api.post_json("/api/v1/safety/reset", {})
        except Exception:
            pass
        if self._initial_muted is not None:
            try:
                self.api.post_json("/api/v1/mute", {"muted": self._initial_muted})
            except Exception:
                pass

    def test_stop_behavior(self) -> None:
        self.api.post_json("/api/v1/stop", {})
        state = self.api.get_json("/api/v1/state")
        outputs = output_map(state)
        blocked = self.api.request_json(
            "/api/v1/output/CH1/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(200, 409),
        )[1]

        if any(outputs[ch]["actual"] for ch in ("CH1", "CH2", "CH3")):
            self.add("stop", "FAIL", "STOP did not switch off all main outputs", outputs=outputs)
        elif blocked.get("accepted"):
            self.add("stop", "FAIL", "Manual ON was accepted while STOP was active", response=blocked)
        else:
            self.add("stop", "PASS", "STOP blocks manual ON for CH1..CH3 and keeps outputs off", response=blocked)

        self.api.post_json("/api/v1/stop?release=1", {})

    def test_output_cycle(self, output_id: str) -> None:
        state_before = self.api.get_json("/api/v1/state")
        out_before = output_map(state_before).get(output_id, {})
        if not out_before:
            self.add(f"output:{output_id}", "FAIL", "Output is missing from /api/v1/state")
            return

        if output_id == "CH5":
            self.add(
                f"output:{output_id}",
                "WARN",
                "CH5 shares the sound path and command-beep path; active checks are limited to endpoint acceptance",
                output=out_before,
            )

        status_on, response_on = self.api.request_json(
            f"/api/v1/output/{output_id}/manual",
            method="POST",
            payload={"state": True},
            ok_statuses=(200, 409),
        )
        if not response_on.get("accepted"):
            self.add(
                f"output:{output_id}:on",
                "FAIL",
                f"Manual ON rejected: {response_on.get('detailText') or response_on.get('userMessage') or response_on.get('detail')}",
                http_status=status_on,
                response=response_on,
            )
            return

        try:
            state_on = self.api.wait_for_state(
                lambda current, oid=output_id: (
                    bool(output_map(current)[oid]["actual"])
                    or bool(output_map(current)[oid].get("relayError"))
                ),
                timeout=self.timeout,
            )
        except AssertionError as exc:
            self.add(f"output:{output_id}:on", "FAIL", f"Timed out waiting for ON state: {exc}", response=response_on)
            return

        out_on = output_map(state_on)[output_id]
        if out_on.get("relayError"):
            self.add(
                f"output:{output_id}:on",
                "FAIL",
                f"Manual ON ended with relayError={out_on.get('relayError')}: {out_on.get('relayErrorText')}",
                output=out_on,
            )
            return
        if out_on.get("actual") is not True:
            self.add(f"output:{output_id}:on", "FAIL", "Output did not reach actual=true after manual ON", output=out_on)
            return

        status_off, response_off = self.api.request_json(
            f"/api/v1/output/{output_id}/manual",
            method="POST",
            payload={"state": False},
            ok_statuses=(200, 409),
        )
        if not response_off.get("accepted"):
            self.add(
                f"output:{output_id}:off",
                "FAIL",
                f"Manual OFF rejected: {response_off.get('detailText') or response_off.get('userMessage') or response_off.get('detail')}",
                http_status=status_off,
                response=response_off,
            )
            return

        try:
            state_off = self.api.wait_for_state(
                lambda current, oid=output_id: output_map(current)[oid]["actual"] is False,
                timeout=self.timeout,
            )
        except AssertionError as exc:
            self.add(f"output:{output_id}:off", "FAIL", f"Timed out waiting for OFF state: {exc}", response=response_off)
            return

        out_off = output_map(state_off)[output_id]
        if out_off.get("actual") is not False:
            self.add(f"output:{output_id}:off", "FAIL", "Output remained ON after manual OFF", output=out_off)
            return

        self.add(
            f"output:{output_id}",
            "PASS",
            "Manual ON/OFF cycle completed successfully",
            before=out_before,
            on=out_on,
            off=out_off,
        )

    def render_markdown(self) -> str:
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        pass_count = sum(1 for item in self.results if item.status == "PASS")
        warn_count = sum(1 for item in self.results if item.status == "WARN")
        fail_count = sum(1 for item in self.results if item.status == "FAIL")
        skip_count = sum(1 for item in self.results if item.status == "SKIP")

        lines = [
            "# RectColumn API Audit Report",
            "",
            f"- Generated: {now}",
            f"- Base URL: `{self.api.base_url}`",
            f"- Firmware: `{self.info.get('fw', 'unknown')}`",
            f"- API version: `{self.info.get('apiVersion', 'unknown')}`",
            f"- EMU mode: `{self.info.get('emu', False)}`",
            f"- Active tests enabled: `{self.allow_write}`",
            f"- EMU scenarios enabled: `{self.emu_scenarios}`",
            "",
            "## Summary",
            "",
            f"- PASS: {pass_count}",
            f"- WARN: {warn_count}",
            f"- FAIL: {fail_count}",
            f"- SKIP: {skip_count}",
            "",
            "## Results",
            "",
            "| Status | Test | Detail |",
            "| --- | --- | --- |",
        ]

        for item in self.results:
            detail = item.detail.replace("\n", " ").replace("|", "\\|")
            lines.append(f"| {item.status} | `{item.name}` | {detail} |")

        lines.extend(
            [
                "",
                "## Device Snapshot",
                "",
                "```json",
                json.dumps(
                    {
                        "info": self.info,
                        "diag": self.diag,
                        "outputs": self.state.get("outputs", []),
                        "sensors": self.state.get("sensors", []),
                        "confirmations": self.state.get("confirmations", []),
                    },
                    ensure_ascii=False,
                    indent=2,
                ),
                "```",
                "",
            ]
        )
        return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a RectColumn API audit and produce a Markdown report.")
    parser.add_argument("--base-url", default=None, help="Controller base URL, for example http://192.168.4.1")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT, help="Per-step timeout in seconds")
    parser.add_argument("--allow-write", action="store_true", help="Enable active STOP/output tests that change device state")
    parser.add_argument("--emu-scenarios", action="store_true", help="Run EMU-specific scenarios; implies --allow-write")
    parser.add_argument("--report", default=None, help="Write the Markdown report to this path")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    allow_write = bool(args.allow_write or args.emu_scenarios)
    api = RectColumnApi(base_url=args.base_url, timeout=args.timeout)
    auditor = RectColumnAuditor(
        api,
        allow_write=allow_write,
        emu_scenarios=bool(args.emu_scenarios),
        timeout=args.timeout,
    )

    try:
        auditor.run()
    except Exception as exc:
        print(f"Audit failed: {exc}", file=sys.stderr)
        return 2

    report = auditor.render_markdown()

    if args.report:
        report_path = Path(args.report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(report, encoding="utf-8")
        print(f"Report written to {report_path}")
    else:
        print(report)

    fail_count = sum(1 for item in auditor.results if item.status == "FAIL")
    return 1 if fail_count else 0


if __name__ == "__main__":
    sys.exit(main())
