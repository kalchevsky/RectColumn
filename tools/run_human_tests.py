#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
import unittest
from pathlib import Path

try:  # pragma: no cover - depends on how the script is started
    from .human_report import (
        build_default_report_path,
        get_report_base_url,
        run_suite_and_write_report,
    )
except ImportError:  # pragma: no cover
    from human_report import (  # type: ignore
        build_default_report_path,
        get_report_base_url,
        run_suite_and_write_report,
    )


PROJECT_ROOT = Path(__file__).resolve().parents[1]
TOOLS_DIR = Path(__file__).resolve().parent

if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))


UNIT_TEST_MODULES = [
    "tools.test_logic_scheme",
    "tools.test_logic_full_matrix",
]

LIVE_TEST_MODULES = [
    "tools.test_api_emu_smoke",
    "tools.test_api_emu_manual_logic",
    "tools.test_api_emu_confirmation_modes",
    "tools.test_api_emu_channel_sensor_matrix",
    "tools.test_flow_ctrl_delay_log",
]


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run RectColumn tests and generate a human-readable HTML report.",
    )
    parser.add_argument(
        "--unit",
        action="store_true",
        help="Run only local unit tests: tools.test_logic_scheme and tools.test_logic_full_matrix.",
    )
    parser.add_argument(
        "--live",
        action="store_true",
        help="Run live/API tests for the prepared EMU board.",
    )
    parser.add_argument(
        "--all",
        action="store_true",
        help="Discover and run all tools/test_*.py tests.",
    )
    parser.add_argument(
        "--module",
        action="append",
        default=[],
        help="Run a specific unittest module. May be specified multiple times.",
    )
    parser.add_argument(
        "--output",
        help="Explicit path to the HTML report file, for example reports/my_report.html.",
    )
    parser.add_argument(
        "--quiet",
        action="store_true",
        help="Suppress verbose unittest console output and keep only the final summary.",
    )
    return parser.parse_args(argv)


def unique_modules(modules: list[str]) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []
    for module in modules:
        if module not in seen:
            seen.add(module)
            ordered.append(module)
    return ordered


def resolve_modules(args: argparse.Namespace) -> tuple[list[str], bool, bool]:
    if args.all:
        return [], True, False

    selected: list[str] = []
    if args.unit:
        selected.extend(UNIT_TEST_MODULES)
    if args.live:
        selected.extend(LIVE_TEST_MODULES)
    if args.module:
        selected.extend(args.module)

    if not selected:
        selected.extend(UNIT_TEST_MODULES)
        return unique_modules(selected), False, True

    return unique_modules(selected), False, False


def discover_all_suite() -> unittest.TestSuite:
    loader = unittest.defaultTestLoader
    return loader.discover(
        start_dir=str(TOOLS_DIR),
        pattern="test_*.py",
        top_level_dir=str(PROJECT_ROOT),
    )


def load_module_suite(modules: list[str]) -> unittest.TestSuite:
    loader = unittest.defaultTestLoader
    return loader.loadTestsFromNames(modules)


def infer_mode(modules: list[str], discover_all: bool) -> str:
    if discover_all:
        return "mixed (local/unit + live/API)"
    if not modules:
        return "local/unit"
    live_flags = [module.startswith("tools.test_api_") for module in modules]
    if all(live_flags):
        return "live/API"
    if not any(live_flags):
        return "local/unit"
    return "mixed (local/unit + live/API)"


def resolve_output_path(raw_output: str | None) -> Path:
    if not raw_output:
        return build_default_report_path(PROJECT_ROOT)
    output = Path(raw_output)
    if not output.is_absolute():
        output = PROJECT_ROOT / output
    return output


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    modules, discover_all, used_default_selection = resolve_modules(args)
    suite = discover_all_suite() if discover_all else load_module_suite(modules)

    selected_modules = ["tools discover -s tools -p test_*.py"] if discover_all else modules
    mode = infer_mode(modules, discover_all)
    output_path = resolve_output_path(args.output)
    base_url = get_report_base_url()
    verbosity = 1 if args.quiet else 2

    run = run_suite_and_write_report(
        suite,
        output_path=output_path,
        mode=mode,
        selected_modules=selected_modules,
        base_url=base_url,
        verbosity=verbosity,
    )

    if used_default_selection:
        print("Default selection: unit tests.")
    print(f"HTML report: {run.output_path}")
    print(
        "Summary: "
        f"status={run.summary['overall_status']}, "
        f"total={run.summary['total']}, "
        f"passed={run.summary['passed']}, "
        f"failed={run.summary['failed']}, "
        f"errors={run.summary['errors']}, "
        f"skipped={run.summary['skipped']}, "
        f"mode={run.summary['mode']}"
    )
    if base_url:
        print(f"Board base URL: {base_url}")

    return 0 if run.result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main())
