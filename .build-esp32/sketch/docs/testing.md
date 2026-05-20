#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\docs\\testing.md"
# Testing

## Обычный `unittest`

Локальные unit-тесты по логике:

```powershell
python -m unittest -v tools.test_logic_scheme
python -m unittest -v tools.test_logic_full_matrix
python -m unittest -v tools.test_logic_scheme tools.test_logic_full_matrix
```

Запуск всех тестовых модулей из `tools/`:

```powershell
python -m unittest discover -s tools -p "test_*.py" -v
```

## HTML-отчёт

Человекочитаемый HTML-отчёт запускается отдельной командой и не меняет обычный `unittest`-запуск:

```powershell
python tools/run_human_tests.py --unit
python -m tools.run_human_tests --unit
```

Если аргументы не указаны, `tools/run_human_tests.py` по умолчанию запускает unit-набор.

Готовый отчёт сохраняется в папку `reports/`:

```text
reports/test_report_<timestamp>.html
```

Пример:

```text
reports/test_report_2026-05-15_14-30-22.html
```

## Режимы запуска HTML-отчёта

Только локальные unit-тесты:

```powershell
python tools/run_human_tests.py --unit
```

Только live/API тесты:

```powershell
$env:RECTCOLUMN_BASE_URL="http://192.168.10.244"
python tools/run_human_tests.py --live
```

Все тестовые модули `tools/test_*.py`:

```powershell
python tools/run_human_tests.py --all
```

Конкретные модули:

```powershell
python tools/run_human_tests.py --module tools.test_logic_scheme --output reports/logic_scheme.html
python tools/run_human_tests.py --module tools.test_api_emu_manual_logic --module tools.test_api_emu_confirmation_modes
```

Поддерживаемые опции:

```text
--unit
--live
--all
--module <module>
--output <file>
```

## Live/API тесты

Для live-тестов прошивка и `EMU_MODE` не переключаются автоматически. Скрипт только использует уже доступную плату и читает базовый URL из:

- `RECTCOLUMN_BASE_URL`
- `RECTLOLUMN_BASE_URL`

Примеры:

```powershell
$env:RECTCOLUMN_BASE_URL="http://192.168.10.244"
python tools/run_human_tests.py --live
```

```cmd
set RECTCOLUMN_BASE_URL=http://192.168.10.244
python tools/run_human_tests.py --live
```
