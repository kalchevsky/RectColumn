#!/usr/bin/env python3
from __future__ import annotations

import html
import inspect
import json
import os
import platform
import sys
import time
import unittest
from collections import OrderedDict
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


STATUS_PASS = "PASS"
STATUS_FAIL = "FAIL"
STATUS_ERROR = "ERROR"
STATUS_SKIPPED = "SKIPPED"
STATUS_XFAIL = "XFAIL"
STATUS_UNEXPECTED_SUCCESS = "UNEXPECTED_SUCCESS"

_STATUS_PRIORITY = {
    STATUS_PASS: 0,
    STATUS_SKIPPED: 1,
    STATUS_FAIL: 2,
    STATUS_ERROR: 3,
    STATUS_XFAIL: 2,
    STATUS_UNEXPECTED_SUCCESS: 2,
}


@dataclass(frozen=True)
class HumanCaseMetadata:
    title: str | None = None
    situation: str | None = None
    steps: tuple[str, ...] = ()
    expected: str | None = None


@dataclass
class HumanReportRun:
    summary: dict[str, Any]
    records: list[dict[str, Any]]
    output_path: Path
    result: unittest.TestResult


def human_case(*, title: str | None = None, situation: str | None = None,
               steps: list[str] | tuple[str, ...] | None = None,
               expected: str | None = None):
    def decorator(func):
        func.__human_case__ = HumanCaseMetadata(
            title=title,
            situation=situation,
            steps=tuple(steps or ()),
            expected=expected,
        )
        return func

    return decorator


def _ensure_human_detail_store(testcase: Any) -> OrderedDict[str, Any]:
    details = getattr(testcase, "_human_details", None)
    if details is None:
        details = OrderedDict()
        setattr(testcase, "_human_details", details)
    return details


def record_human_detail(testcase: Any, key: str, value: Any) -> Any:
    details = _ensure_human_detail_store(testcase)
    if key in details:
        current = details[key]
        if isinstance(current, list):
            current.append(value)
        else:
            details[key] = [current, value]
    else:
        details[key] = value
    return value


def set_human_actual_result(testcase: Any, text: str) -> str:
    setattr(testcase, "_human_actual_result", text)
    return text


def _testcase_add_human_detail(self, key: str, value: Any) -> Any:
    return record_human_detail(self, key, value)


def _testcase_set_human_actual_result(self, text: str) -> str:
    return set_human_actual_result(self, text)


if not hasattr(unittest.TestCase, "add_human_detail"):
    setattr(unittest.TestCase, "add_human_detail", _testcase_add_human_detail)

if not hasattr(unittest.TestCase, "set_human_actual_result"):
    setattr(unittest.TestCase, "set_human_actual_result", _testcase_set_human_actual_result)


def get_report_base_url() -> str | None:
    for env_name in ("RECTLOLUMN_BASE_URL", "RECTCOLUMN_BASE_URL"):
        value = os.environ.get(env_name)
        if value:
            return value
    return None


def build_default_report_path(project_root: Path, now: datetime | None = None) -> Path:
    now = now or datetime.now()
    filename = f"test_report_{now.strftime('%Y-%m-%d_%H-%M-%S')}.html"
    return project_root / "reports" / filename


class HumanTextTestResult(unittest.TextTestResult):
    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream, descriptions, verbosity)
        self.records: list[dict[str, Any]] = []
        self._records_by_key: dict[int, dict[str, Any]] = {}
        self._record_index = 0

    def startTest(self, test):
        record = self._get_or_create_record(test)
        record["started_at"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        record["_started_perf"] = time.perf_counter()
        record["status"] = STATUS_PASS
        record["traceback"] = ""
        record["actual_result"] = ""
        record["skip_reason"] = ""
        record["subtests"] = []
        record["subtests_passed"] = 0
        setattr(test, "_human_details", OrderedDict())
        setattr(test, "_human_actual_result", None)
        super().startTest(test)

    def stopTest(self, test):
        record = self._get_or_create_record(test)
        started = record.pop("_started_perf", None)
        if started is not None:
            record["duration_seconds"] = time.perf_counter() - started
        else:
            record.setdefault("duration_seconds", 0.0)
        record["diagnostics"] = self._extract_diagnostics(test)
        explicit_actual = getattr(test, "_human_actual_result", None)
        if explicit_actual:
            record["actual_result"] = explicit_actual
        elif not record.get("actual_result"):
            record["actual_result"] = self._default_actual_result(record)
        super().stopTest(test)

    def addSuccess(self, test):
        self._set_outcome(test, STATUS_PASS)
        super().addSuccess(test)

    def addFailure(self, test, err):
        self._set_outcome(test, STATUS_FAIL, err=err)
        super().addFailure(test, err)

    def addError(self, test, err):
        self._set_outcome(test, STATUS_ERROR, err=err)
        super().addError(test, err)

    def addSkip(self, test, reason):
        self._set_outcome(test, STATUS_SKIPPED, actual_result=f"Тест пропущен: {reason}", skip_reason=reason)
        super().addSkip(test, reason)

    def addExpectedFailure(self, test, err):
        self._set_outcome(
            test,
            STATUS_XFAIL,
            err=err,
            actual_result="Ожидаемое падение теста подтвердилось.",
        )
        super().addExpectedFailure(test, err)

    def addUnexpectedSuccess(self, test):
        self._set_outcome(
            test,
            STATUS_UNEXPECTED_SUCCESS,
            actual_result="Тест неожиданно прошёл, хотя был помечен как ожидаемо падающий.",
        )
        super().addUnexpectedSuccess(test)

    def addSubTest(self, test, subtest, err):
        record = self._get_or_create_record(test)
        if err is None:
            record["subtests_passed"] = record.get("subtests_passed", 0) + 1
        else:
            status = self._status_from_exc(err, test)
            tb_text = self._exc_info_to_string(err, subtest)
            record.setdefault("subtests", []).append({
                "name": self._stringify_subtest(subtest),
                "status": status,
                "traceback": tb_text,
            })
            actual_result = (
                "Внутри теста есть под-сценарий с ошибкой."
                if status == STATUS_ERROR
                else "Внутри теста есть под-сценарий, который не прошёл проверку."
            )
            self._set_outcome(test, status, actual_result=actual_result)
        super().addSubTest(test, subtest, err)

    def _get_or_create_record(self, test) -> dict[str, Any]:
        key = id(test)
        record = self._records_by_key.get(key)
        if record is not None:
            return record
        self._record_index += 1
        record = self._build_record(test)
        record["index"] = self._record_index
        self.records.append(record)
        self._records_by_key[key] = record
        return record

    def _build_record(self, test) -> dict[str, Any]:
        test_id = self._safe_test_id(test)
        module_name = getattr(test.__class__, "__module__", "")
        class_name = getattr(test.__class__, "__name__", test.__class__.__name__)
        method_name = getattr(test, "_testMethodName", "")
        method = getattr(test, method_name, None) if method_name else None
        docstring = inspect.getdoc(method) if method else None
        metadata = getattr(method, "__human_case__", None)
        title = metadata.title if metadata and metadata.title else _fallback_title(method_name or test_id)
        if metadata and metadata.situation:
            situation = metadata.situation
        elif docstring:
            situation = docstring
        else:
            situation = (
                f"Технический тест {module_name}.{class_name}.{method_name} без расширенного описания."
                if module_name and class_name and method_name
                else f"Технический тест {test_id} без расширенного описания."
            )
        steps = list(metadata.steps) if metadata else []
        expected = metadata.expected if metadata and metadata.expected else "Ожидаемый результат не был явно описан."
        return {
            "id": test_id,
            "module": module_name,
            "class_name": class_name,
            "method_name": method_name,
            "title": title,
            "situation": situation,
            "steps": steps,
            "expected": expected,
            "status": STATUS_PASS,
            "traceback": "",
            "duration_seconds": 0.0,
            "diagnostics": OrderedDict(),
            "actual_result": "",
            "skip_reason": "",
            "subtests": [],
            "subtests_passed": 0,
        }

    def _set_outcome(self, test, status: str, *, err=None,
                     actual_result: str | None = None,
                     skip_reason: str = "") -> None:
        record = self._get_or_create_record(test)
        current_status = record.get("status", STATUS_PASS)
        if _STATUS_PRIORITY.get(status, 0) >= _STATUS_PRIORITY.get(current_status, 0):
            record["status"] = status
        if err is not None:
            record["traceback"] = self._exc_info_to_string(err, test)
        if actual_result:
            record["actual_result"] = actual_result
        elif err is not None and not record.get("actual_result"):
            record["actual_result"] = self._message_from_traceback(record["traceback"], status)
        if skip_reason:
            record["skip_reason"] = skip_reason

    def _extract_diagnostics(self, test) -> OrderedDict[str, Any]:
        details = getattr(test, "_human_details", None)
        if isinstance(details, OrderedDict):
            return details
        if isinstance(details, dict):
            return OrderedDict(details.items())
        return OrderedDict()

    def _status_from_exc(self, err, test) -> str:
        exc_type = err[0]
        if issubclass(exc_type, unittest.SkipTest):
            return STATUS_SKIPPED
        if issubclass(exc_type, test.failureException):
            return STATUS_FAIL
        return STATUS_ERROR

    def _safe_test_id(self, test) -> str:
        test_id = getattr(test, "id", None)
        if callable(test_id):
            try:
                return str(test_id())
            except Exception:
                return str(test)
        return str(test)

    def _stringify_subtest(self, subtest) -> str:
        description = str(subtest)
        if description.startswith("test"):
            return description
        return description or "subTest"

    def _message_from_traceback(self, tb_text: str, status: str) -> str:
        message = _last_non_empty_line(tb_text)
        if message:
            return message
        if status == STATUS_FAIL:
            return "Тест завершился с ошибкой проверки."
        if status == STATUS_ERROR:
            return "Тест завершился с необработанной ошибкой."
        return "Результат теста зафиксирован."

    def _default_actual_result(self, record: dict[str, Any]) -> str:
        status = record.get("status")
        if status == STATUS_PASS:
            diag_keys = list(record.get("diagnostics", {}).keys())
            if diag_keys:
                return "Тест прошёл успешно. Диагностика сохранена: " + ", ".join(diag_keys) + "."
            if record.get("subtests_passed"):
                return f"Тест прошёл успешно, подтверждено под-сценариев: {record['subtests_passed']}."
            return "Тест прошёл успешно."
        if status == STATUS_SKIPPED:
            return record.get("skip_reason") or "Тест пропущен."
        if status == STATUS_XFAIL:
            return "Ожидаемое падение подтверждено."
        if status == STATUS_UNEXPECTED_SUCCESS:
            return "Тест неожиданно прошёл."
        return self._message_from_traceback(record.get("traceback", ""), status)


def run_suite_and_write_report(suite: unittest.TestSuite, *,
                               output_path: str | os.PathLike[str] | Path,
                               mode: str,
                               selected_modules: list[str] | None = None,
                               base_url: str | None = None,
                               stream=None,
                               verbosity: int = 2) -> HumanReportRun:
    started_at = datetime.now()
    runner_kwargs = {
        "verbosity": verbosity,
        "resultclass": HumanTextTestResult,
    }
    runner_kwargs["stream"] = stream if stream is not None else sys.stderr
    runner = unittest.TextTestRunner(**runner_kwargs)
    result = runner.run(suite)
    finished_at = datetime.now()
    records = sorted(result.records, key=lambda item: item["index"])
    summary = build_summary(
        records=records,
        result=result,
        started_at=started_at,
        finished_at=finished_at,
        mode=mode,
        selected_modules=selected_modules or [],
        base_url=base_url,
    )
    html_text = render_html_report(summary, records)
    resolved_output = Path(output_path)
    resolved_output.parent.mkdir(parents=True, exist_ok=True)
    resolved_output.write_text(html_text, encoding="utf-8")
    return HumanReportRun(
        summary=summary,
        records=records,
        output_path=resolved_output,
        result=result,
    )


def build_summary(*, records: list[dict[str, Any]], result: unittest.TestResult,
                  started_at: datetime, finished_at: datetime, mode: str,
                  selected_modules: list[str], base_url: str | None) -> dict[str, Any]:
    counts = {
        STATUS_PASS: 0,
        STATUS_FAIL: 0,
        STATUS_ERROR: 0,
        STATUS_SKIPPED: 0,
    }
    extra_statuses = {
        STATUS_XFAIL: 0,
        STATUS_UNEXPECTED_SUCCESS: 0,
    }
    for record in records:
        status = record.get("status", STATUS_PASS)
        if status in counts:
            counts[status] += 1
        elif status in extra_statuses:
            extra_statuses[status] += 1
    total = len(records)
    overall_status = "OK" if result.wasSuccessful() else "FAIL"
    return {
        "run_started_at": started_at.strftime("%Y-%m-%d %H:%M:%S"),
        "run_finished_at": finished_at.strftime("%Y-%m-%d %H:%M:%S"),
        "python_version": platform.python_version(),
        "python_full": platform.python_implementation() + " " + platform.python_version(),
        "base_url": base_url,
        "mode": mode,
        "total": total,
        "passed": counts[STATUS_PASS],
        "failed": counts[STATUS_FAIL] + extra_statuses[STATUS_UNEXPECTED_SUCCESS],
        "errors": counts[STATUS_ERROR],
        "skipped": counts[STATUS_SKIPPED],
        "xfail": extra_statuses[STATUS_XFAIL],
        "unexpected_success": extra_statuses[STATUS_UNEXPECTED_SUCCESS],
        "duration_seconds": (finished_at - started_at).total_seconds(),
        "overall_status": overall_status,
        "selected_modules": selected_modules,
        "tests_run_technical": result.testsRun,
    }


def render_html_report(summary: dict[str, Any], records: list[dict[str, Any]]) -> str:
    cards_html = "\n".join(render_test_card(record) for record in records) or (
        '<section class="empty-state">Тесты не были найдены.</section>'
    )
    modules_text = "\n".join(summary.get("selected_modules") or ["Автовыбор по умолчанию"])
    title = f"RectColumn Test Report - {summary['run_started_at']}"
    return f"""<!DOCTYPE html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>{html.escape(title)}</title>
  <style>
    :root {{
      --page-bg: #f3f6f8;
      --panel-bg: #ffffffee;
      --card-bg: #ffffff;
      --border: #d6dde5;
      --text: #1f2b3a;
      --muted: #607085;
      --shadow: 0 18px 42px rgba(40, 70, 100, 0.12);
      --pass: #1f8b4c;
      --pass-bg: #e8f6ee;
      --fail: #c33a2f;
      --fail-bg: #fdebea;
      --error: #bd5c12;
      --error-bg: #fff1e5;
      --skip: #6f7782;
      --skip-bg: #eef1f4;
      --accent: #0f5f8c;
      --accent-soft: #dcecf7;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      color: var(--text);
      background:
        radial-gradient(circle at top right, #e4f1f7 0, transparent 28%),
        linear-gradient(180deg, #f7fbfd 0%, var(--page-bg) 100%);
      font-family: "Trebuchet MS", "Segoe UI", sans-serif;
    }}
    .page {{
      width: min(1260px, calc(100% - 32px));
      margin: 0 auto;
      padding: 20px 0 32px;
    }}
    .summary-panel {{
      position: sticky;
      top: 0;
      z-index: 10;
      backdrop-filter: blur(10px);
      background: var(--panel-bg);
      border: 1px solid rgba(214, 221, 229, 0.9);
      border-radius: 24px;
      box-shadow: var(--shadow);
      padding: 20px;
      margin-bottom: 20px;
    }}
    .summary-head {{
      display: flex;
      align-items: flex-start;
      justify-content: space-between;
      gap: 16px;
      flex-wrap: wrap;
    }}
    .summary-head h1 {{
      margin: 0 0 6px;
      font-size: clamp(24px, 3vw, 34px);
      letter-spacing: 0.02em;
    }}
    .summary-head p {{
      margin: 0;
      color: var(--muted);
      max-width: 760px;
      line-height: 1.5;
    }}
    .status-pill {{
      display: inline-flex;
      align-items: center;
      gap: 8px;
      border-radius: 999px;
      padding: 10px 16px;
      font-weight: 700;
      letter-spacing: 0.04em;
      text-transform: uppercase;
      border: 1px solid transparent;
    }}
    .status-pill.pass {{
      color: var(--pass);
      background: var(--pass-bg);
      border-color: #b8e2c8;
    }}
    .status-pill.fail {{
      color: var(--fail);
      background: var(--fail-bg);
      border-color: #efc0bc;
    }}
    .status-pill.error {{
      color: var(--error);
      background: var(--error-bg);
      border-color: #f0ceab;
    }}
    .stats-grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 12px;
      margin: 18px 0;
    }}
    .stat {{
      background: linear-gradient(180deg, #ffffff 0%, #f7fafc 100%);
      border: 1px solid var(--border);
      border-radius: 18px;
      padding: 14px 16px;
      min-height: 92px;
    }}
    .stat .label {{
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--muted);
      margin-bottom: 8px;
    }}
    .stat .value {{
      font-size: 28px;
      font-weight: 700;
      line-height: 1.1;
    }}
    .summary-meta {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 10px;
      margin-bottom: 16px;
    }}
    .meta-chip {{
      background: var(--accent-soft);
      border: 1px solid #c5dceb;
      border-radius: 16px;
      padding: 12px 14px;
      line-height: 1.45;
    }}
    .meta-chip strong {{
      display: block;
      font-size: 12px;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      margin-bottom: 4px;
      color: var(--accent);
    }}
    .toolbar {{
      display: flex;
      gap: 12px;
      align-items: center;
      justify-content: space-between;
      flex-wrap: wrap;
    }}
    .filters {{
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }}
    .filter-btn {{
      border: 1px solid var(--border);
      border-radius: 999px;
      background: #fff;
      color: var(--text);
      padding: 9px 14px;
      font: inherit;
      cursor: pointer;
      transition: 120ms ease-in-out;
    }}
    .filter-btn.active,
    .filter-btn:hover {{
      background: var(--accent);
      color: #fff;
      border-color: var(--accent);
    }}
    .search-box {{
      min-width: min(360px, 100%);
      flex: 1;
      max-width: 420px;
    }}
    .search-box input {{
      width: 100%;
      border-radius: 999px;
      border: 1px solid var(--border);
      padding: 11px 16px;
      font: inherit;
      background: #fff;
    }}
    .cards {{
      display: grid;
      gap: 16px;
    }}
    .test-card {{
      background: var(--card-bg);
      border-radius: 22px;
      border: 1px solid var(--border);
      box-shadow: var(--shadow);
      padding: 20px;
    }}
    .test-card.pass {{ border-left: 8px solid var(--pass); }}
    .test-card.fail {{ border-left: 8px solid var(--fail); }}
    .test-card.error {{ border-left: 8px solid var(--error); }}
    .test-card.skipped {{ border-left: 8px solid var(--skip); }}
    .test-head {{
      display: flex;
      align-items: flex-start;
      justify-content: space-between;
      gap: 16px;
      flex-wrap: wrap;
      margin-bottom: 14px;
    }}
    .test-head h2 {{
      margin: 0;
      font-size: 24px;
      line-height: 1.2;
    }}
    .badge {{
      display: inline-flex;
      align-items: center;
      gap: 6px;
      border-radius: 999px;
      padding: 8px 12px;
      font-size: 12px;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.06em;
    }}
    .badge.pass {{ color: var(--pass); background: var(--pass-bg); }}
    .badge.fail {{ color: var(--fail); background: var(--fail-bg); }}
    .badge.error {{ color: var(--error); background: var(--error-bg); }}
    .badge.skipped {{ color: var(--skip); background: var(--skip-bg); }}
    .meta-row {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
      gap: 10px;
      margin-bottom: 16px;
    }}
    .meta-row div {{
      background: #f8fbfd;
      border: 1px solid #e1e7ee;
      border-radius: 14px;
      padding: 10px 12px;
    }}
    .meta-row strong {{
      display: block;
      color: var(--muted);
      font-size: 12px;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      margin-bottom: 4px;
    }}
    .sections {{
      display: grid;
      gap: 14px;
    }}
    .section {{
      background: #fbfdff;
      border: 1px solid #e5ebf1;
      border-radius: 18px;
      padding: 14px 16px;
    }}
    .section h3 {{
      margin: 0 0 8px;
      font-size: 14px;
      text-transform: uppercase;
      letter-spacing: 0.06em;
      color: var(--accent);
    }}
    .section p {{
      margin: 0;
      line-height: 1.6;
    }}
    .section ol {{
      margin: 0;
      padding-left: 20px;
      line-height: 1.65;
    }}
    details {{
      border: 1px solid #e1e7ee;
      border-radius: 16px;
      background: #fff;
      padding: 10px 14px;
    }}
    details > summary {{
      cursor: pointer;
      font-weight: 700;
      color: var(--accent);
    }}
    pre {{
      margin: 10px 0 0;
      white-space: pre-wrap;
      word-break: break-word;
      background: #0f172112;
      border-radius: 14px;
      padding: 12px 14px;
      overflow-x: auto;
      font-family: Consolas, "Courier New", monospace;
      font-size: 13px;
      line-height: 1.55;
    }}
    .diag-grid {{
      display: grid;
      gap: 10px;
    }}
    .diag-block {{
      border: 1px solid #e1e7ee;
      border-radius: 14px;
      padding: 12px;
      background: #fff;
    }}
    .diag-block strong {{
      display: block;
      margin-bottom: 6px;
    }}
    .muted {{
      color: var(--muted);
    }}
    .hidden {{
      display: none !important;
    }}
    .summary-details {{
      margin-top: 14px;
    }}
    .empty-state {{
      border: 1px dashed var(--border);
      border-radius: 20px;
      background: #fff;
      padding: 28px;
      text-align: center;
      color: var(--muted);
    }}
    @media (max-width: 720px) {{
      .page {{
        width: min(100%, calc(100% - 18px));
      }}
      .summary-panel {{
        border-radius: 18px;
        padding: 16px;
      }}
      .test-card {{
        padding: 16px;
        border-radius: 18px;
      }}
      .search-box {{
        max-width: none;
        width: 100%;
      }}
    }}
  </style>
</head>
<body>
  <div class="page">
    <header class="summary-panel">
      <div class="summary-head">
        <div>
          <h1>RectColumn Human Test Report</h1>
          <p>Человекочитаемый отчёт по логике датчиков, каналов, сигнализаций и manual-команд.</p>
        </div>
        <div class="status-pill {status_pill_class(summary)}">{html.escape(summary['overall_status'])}</div>
      </div>

      <div class="stats-grid">
        {render_stat("Всего тестов", summary["total"])}
        {render_stat("Успешно", summary["passed"])}
        {render_stat("Падений", summary["failed"])}
        {render_stat("Ошибок", summary["errors"])}
        {render_stat("Пропущено", summary["skipped"])}
        {render_stat("Время", format_duration(summary["duration_seconds"]))}
      </div>

      <div class="summary-meta">
        {render_meta_chip("Дата и время запуска", summary["run_started_at"])}
        {render_meta_chip("Python", summary["python_full"])}
        {render_meta_chip("Режим тестов", summary["mode"])}
        {render_meta_chip("Базовый URL платы", summary["base_url"] or "Не задан")}
      </div>

      <div class="toolbar">
        <div class="filters">
          <button class="filter-btn active" type="button" data-filter="ALL">All ({summary["total"]})</button>
          <button class="filter-btn" type="button" data-filter="{STATUS_PASS}">Pass ({summary["passed"]})</button>
          <button class="filter-btn" type="button" data-filter="{STATUS_FAIL}">Fail ({summary["failed"]})</button>
          <button class="filter-btn" type="button" data-filter="{STATUS_ERROR}">Error ({summary["errors"]})</button>
          <button class="filter-btn" type="button" data-filter="{STATUS_SKIPPED}">Skipped ({summary["skipped"]})</button>
        </div>
        <label class="search-box">
          <input id="searchInput" type="search" placeholder="Поиск по названию, описанию, модулю, шагам">
        </label>
      </div>

      <details class="summary-details">
        <summary>Модули запуска</summary>
        <pre>{html.escape(modules_text)}</pre>
      </details>
    </header>

    <main class="cards" id="cardsRoot">
      {cards_html}
    </main>
  </div>

  <script>
    const buttons = Array.from(document.querySelectorAll('.filter-btn'));
    const searchInput = document.getElementById('searchInput');
    const cards = Array.from(document.querySelectorAll('.test-card'));
    let activeFilter = 'ALL';

    function applyFilters() {{
      const term = (searchInput.value || '').trim().toLowerCase();
      for (const card of cards) {{
        const matchesStatus = activeFilter === 'ALL' || card.dataset.status === activeFilter;
        const haystack = card.dataset.search || '';
        const matchesText = !term || haystack.includes(term);
        card.classList.toggle('hidden', !(matchesStatus && matchesText));
      }}
    }}

    for (const button of buttons) {{
      button.addEventListener('click', () => {{
        activeFilter = button.dataset.filter;
        for (const current of buttons) {{
          current.classList.toggle('active', current === button);
        }}
        applyFilters();
      }});
    }}

    searchInput.addEventListener('input', applyFilters);
    applyFilters();
  </script>
</body>
</html>
"""


def render_test_card(record: dict[str, Any]) -> str:
    status = record.get("status", STATUS_PASS)
    normalized_status = status if status in {STATUS_PASS, STATUS_FAIL, STATUS_ERROR, STATUS_SKIPPED} else (
        STATUS_FAIL if status == STATUS_UNEXPECTED_SUCCESS else STATUS_FAIL
    )
    search_blob = " ".join(filter(None, [
        record.get("title", ""),
        record.get("situation", ""),
        " ".join(record.get("steps", [])),
        record.get("expected", ""),
        record.get("actual_result", ""),
        record.get("module", ""),
        record.get("class_name", ""),
        record.get("method_name", ""),
    ])).lower()

    traceback_html = ""
    if record.get("traceback"):
        traceback_html = (
            "<details><summary>Traceback</summary>"
            f"<pre>{html.escape(record['traceback'])}</pre>"
            "</details>"
        )

    subtests_html = ""
    if record.get("subtests"):
        chunks = []
        for subtest in record["subtests"]:
            chunks.append(
                "<div class=\"diag-block\">"
                f"<strong>{html.escape(subtest['name'])} [{html.escape(subtest['status'])}]</strong>"
                f"<pre>{html.escape(subtest['traceback'])}</pre>"
                "</div>"
            )
        subtests_html = (
            "<div class=\"section\">"
            "<h3>Подсценарии</h3>"
            f"<p>Успешных под-сценариев: {record.get('subtests_passed', 0)}</p>"
            f"<div class=\"diag-grid\">{''.join(chunks)}</div>"
            "</div>"
        )
    elif record.get("subtests_passed"):
        subtests_html = (
            "<div class=\"section\">"
            "<h3>Подсценарии</h3>"
            f"<p>Успешно подтверждено под-сценариев: {record.get('subtests_passed', 0)}.</p>"
            "</div>"
        )

    diagnostics_html = render_diagnostics(record.get("diagnostics", OrderedDict()))
    steps_html = render_steps(record.get("steps", []))

    return (
        f"<article class=\"test-card {status_css_class(normalized_status)}\" "
        f"data-status=\"{html.escape(normalized_status)}\" "
        f"data-search=\"{html.escape(search_blob)}\">"
        "<div class=\"test-head\">"
        "<div>"
        f"<div class=\"badge {status_css_class(normalized_status)}\">{html.escape(status)}</div>"
        f"<h2>{html.escape(record.get('title', 'Без названия'))}</h2>"
        "</div>"
        f"<div class=\"muted\">{html.escape(format_duration(record.get('duration_seconds', 0.0)))}</div>"
        "</div>"
        "<div class=\"meta-row\">"
        f"{render_meta_value('Модуль', record.get('module') or 'Не определён')}"
        f"{render_meta_value('Класс', record.get('class_name') or 'Не определён')}"
        f"{render_meta_value('Метод', record.get('method_name') or record.get('id') or 'Не определён')}"
        f"{render_meta_value('Статус', status)}"
        "</div>"
        "<div class=\"sections\">"
        f"{render_section('Ситуация', record.get('situation') or 'Описание не указано.')}"
        f"{steps_html}"
        f"{render_section('Ожидаемый результат', record.get('expected') or 'Не указано.')}"
        f"{render_section('Фактический результат', record.get('actual_result') or 'Результат не зафиксирован.')}"
        f"{diagnostics_html}"
        f"{subtests_html}"
        f"{traceback_html}"
        "</div>"
        "</article>"
    )


def render_diagnostics(diagnostics: OrderedDict[str, Any]) -> str:
    if not diagnostics:
        return (
            "<div class=\"section\">"
            "<h3>Диагностика</h3>"
            "<p class=\"muted\">Дополнительные данные не передавались.</p>"
            "</div>"
        )
    blocks = []
    for key, value in diagnostics.items():
        blocks.append(
            "<div class=\"diag-block\">"
            f"<strong>{html.escape(str(key))}</strong>"
            f"<pre>{html.escape(serialize_detail(value))}</pre>"
            "</div>"
        )
    return (
        "<div class=\"section\">"
        "<h3>Диагностика</h3>"
        f"<div class=\"diag-grid\">{''.join(blocks)}</div>"
        "</div>"
    )


def render_steps(steps: list[str]) -> str:
    if not steps:
        return (
            "<div class=\"section\">"
            "<h3>Шаги и условия</h3>"
            "<p class=\"muted\">Подробные шаги не были указаны.</p>"
            "</div>"
        )
    items = "".join(f"<li>{html.escape(step)}</li>" for step in steps)
    return (
        "<div class=\"section\">"
        "<h3>Шаги и условия</h3>"
        f"<ol>{items}</ol>"
        "</div>"
    )


def render_section(title: str, text: str) -> str:
    return (
        "<div class=\"section\">"
        f"<h3>{html.escape(title)}</h3>"
        f"<p>{html.escape(text).replace(chr(10), '<br>')}</p>"
        "</div>"
    )


def render_stat(label: str, value: Any) -> str:
    return (
        "<div class=\"stat\">"
        f"<div class=\"label\">{html.escape(str(label))}</div>"
        f"<div class=\"value\">{html.escape(str(value))}</div>"
        "</div>"
    )


def render_meta_chip(label: str, value: Any) -> str:
    return (
        "<div class=\"meta-chip\">"
        f"<strong>{html.escape(str(label))}</strong>"
        f"{html.escape(str(value))}"
        "</div>"
    )


def render_meta_value(label: str, value: Any) -> str:
    return (
        "<div>"
        f"<strong>{html.escape(str(label))}</strong>"
        f"{html.escape(str(value))}"
        "</div>"
    )


def format_duration(seconds: float) -> str:
    return f"{seconds:.3f} s"


def serialize_detail(value: Any) -> str:
    if isinstance(value, str):
        return value
    try:
        return json.dumps(value, ensure_ascii=False, indent=2, default=_json_default)
    except TypeError:
        return repr(value)


def _json_default(value: Any) -> Any:
    if isinstance(value, Path):
        return str(value)
    if isinstance(value, set):
        return sorted(value)
    return repr(value)


def _fallback_title(name: str) -> str:
    title = name
    if title.startswith("test_"):
        title = title[5:]
    title = title.replace("_", " ").strip()
    if not title:
        return "Безымянный тест"
    return title[0].upper() + title[1:]


def _last_non_empty_line(text: str) -> str:
    for line in reversed(text.splitlines()):
        stripped = line.strip()
        if stripped:
            return stripped
    return ""


def status_css_class(status: str) -> str:
    if status == STATUS_PASS:
        return "pass"
    if status == STATUS_FAIL:
        return "fail"
    if status == STATUS_ERROR:
        return "error"
    return "skipped"


def status_pill_class(summary: dict[str, Any]) -> str:
    if summary.get("overall_status") == "OK":
        return "pass"
    if summary.get("errors", 0):
        return "error"
    return "fail"
