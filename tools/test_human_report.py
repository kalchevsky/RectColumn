#!/usr/bin/env python3
from __future__ import annotations

import io
import tempfile
import unittest
from pathlib import Path

try:  # pragma: no cover - depends on unittest invocation style
    from .human_report import human_case, run_suite_and_write_report
except ImportError:  # pragma: no cover
    from human_report import human_case, run_suite_and_write_report  # type: ignore


class HumanReportGeneratorTests(unittest.TestCase):
    def test_html_report_contains_human_case_and_statuses(self):
        class DummyPassCase(unittest.TestCase):
            @human_case(
                title="Читабельный успешный сценарий",
                situation="Проверяется, что отчёт сохраняет human_case-описание.",
                steps=["Выполнить проходной тест.", "Сохранить HTML-отчёт."],
                expected="PASS карточка содержит title, ситуацию и шаги.",
            )
            def test_ok(self):
                pass

        class DummyFailCase(unittest.TestCase):
            def test_broken(self):
                """Fallback description from docstring."""
                self.fail("planned failure for HTML report self-test")

        suite = unittest.TestSuite()
        suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(DummyPassCase))
        suite.addTests(unittest.defaultTestLoader.loadTestsFromTestCase(DummyFailCase))

        with tempfile.TemporaryDirectory() as tmpdir:
            report_path = Path(tmpdir) / "report.html"
            result = run_suite_and_write_report(
                suite,
                output_path=report_path,
                mode="local/unit",
                selected_modules=["tools.test_human_report"],
                stream=io.StringIO(),
                verbosity=0,
            )

            self.assertTrue(report_path.exists())
            html_text = report_path.read_text(encoding="utf-8")
            self.assertIn("Читабельный успешный сценарий", html_text)
            self.assertIn("Fallback description from docstring.", html_text)
            self.assertIn("PASS", html_text)
            self.assertIn("FAIL", html_text)
            self.assertIn("planned failure for HTML report self-test", html_text)
            self.assertFalse(result.result.wasSuccessful())


if __name__ == "__main__":
    unittest.main(verbosity=2)
