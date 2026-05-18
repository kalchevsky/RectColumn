# FIX NOTES 1.6.32-hotfix1

## Что исправлено и почему

- Исправлена регрессия `ConfirmationManager::loop()` при активном `STOP`.
- После 1.6.32 WER-пины для CH1..CH3 продолжали опрашиваться даже во время `STOP` — это правильно, потому что UI должен видеть реальное физическое состояние реле.
- Но вместе с этим продолжала работать fault-логика `STUCK_ON` / `NO_ON_CONFIRM`, из-за чего при штатном операторском `STOP` могли появляться ложные события `WER_CHx`.
- В hotfix при активном `STOP` для CH1..CH3 fault-логика пропускается, а `c.actual` остаётся живым для UI.
- Уже защёлкнутый до `STOP` `faultLatched` не сбрасывается автоматически и по-прежнему снимается только явным `POST /api/v1/safety/reset`.

## Какие файлы изменены

- `ConfirmationManager.h`
- `config.h`
- `docs/SHUTDOWN_LOGIC.md`
- `tools/test_logic_scheme.py`

## Какой регрессионный тест добавлен

- В `tools/test_logic_scheme.py` добавлен модельный тест `test_stop_does_not_latch_wer_fault`.
- Там же добавлен source-guard `test_stop_suppresses_new_wer_faults_for_main_outputs`, который проверяет наличие STOP-ветки в `ConfirmationManager`.

## Какие потенциальные побочные эффекты возможны

- Во время активного `STOP` новые fault-события WER для CH1..CH3 больше не формируются, даже если физический WER-пин реально завис в HIGH. Это осознанное поведение, чтобы исключить ложные аварии во время операторского останова.
- После снятия `STOP` fault-логика возобновляется штатно на следующей итерации, без отдельного rearm.

## Фактически выполненные проверки

- `python -m unittest -v tools.test_logic_scheme tools.test_logic_full_matrix` -> `126 tests`, `OK`
- `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` -> успешно
