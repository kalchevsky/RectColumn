# FIX NOTES 1.6.32

## Что исправлено

### 1. RemoteNotifier

- Убрана модель `xTaskCreate()` на каждое уведомление.
- Добавлен один постоянный worker-task `notify-worker`, pinned на `APP_CPU_NUM`.
- Отправка уведомлений переведена на FreeRTOS-очередь фиксированного размера.
- Добавлен snapshot конфигурации (`enabled/url/token`) под `portMUX`, чтобы worker не держал mutex во время HTTP.
- Добавлен счётчик `droppedCount` и endpoint `/api/v1/notify/status`.

### 2. Подтверждения WER при STOP

- `ConfirmationManager::loop()` теперь вызывается всегда, даже при активном `STOP`.
- API `/api/v1/output` и relay-state продолжают показывать реальный WER для CH1..CH3 при STOP.
- Для CH4 подтверждение по-прежнему трактуется как команда, а не как физический WER-пин.

### 3. WER timeout только как индикация

- Убран `forceOff()` при таймауте подтверждения реле в `OutputManager::updateRelayCommandFeedback()`.
- Убрана safety-защёлка `RULEIDX_SAFETY_WER` в `ProcessSafety`.
- WER timeout и mismatch остались только как диагностические события в журнале и API.

### 4. dT в UI и backend

- `dT` разрешён как виртуальный аналоговый control-sensor для CH1..CH3.
- В UI для `dT` скрыты аппаратные поля (`enabled/period`), но оставлены редакторы тревог, control-правил и задержек.
- В backend `dT` проходит через ту же scheme-логику, что и остальные аналоговые control-sensors.

### 5. Аппаратная кнопка ACK

- Добавлена кнопка квитирования тревог на GPIO14 (`INPUT_PULLUP`, active low, debounce 50 мс).
- На фронте нажатия выполняется `acknowledgeCurrentAlarms()`, короткий beep и запись в `EventLog`.
- В `config.h` добавлено предупреждение, что GPIO14 является strapping pin.

## Какие файлы изменены

- `RemoteNotifier.h`
- `RectColumn.ino`
- `OutputManager.h`
- `ProcessSafety.h`
- `WebAPI.h`
- `SensorManager.h`
- `WebPageAppJs.h`
- `AckButton.h`
- `config.h`
- `docs/SHUTDOWN_LOGIC.md`
- `tools/test_logic_scheme.py`
- `tools/test_logic_full_matrix.py`
- `tools/test_api_emu_channel_sensor_matrix.py`
- `tools/test_notify_failure_emu.py`
- `tools/run_human_tests.py`

## Какие новые тесты добавлены

- Source-guards для single-worker `RemoteNotifier`, ACK-кнопки, `dT` в UI/backend, WER diagnostic-only.
- Новый live/EMU сценарий `tools/test_notify_failure_emu.py` для очереди уведомлений, dropped-count и heap-метрик.
- Обновлены live-ожидания для `dT` и WER timeout в `tools/test_api_emu_channel_sensor_matrix.py`.

## Какие потенциальные регрессии возможны

- Из-за одного worker-task уведомления теперь идут строго последовательно; при медленном ntfy backlog будет дропаться, а не распараллеливаться.
- `dT` теперь участвует в scheme-mapping CH1..CH3 как виртуальный аналоговый control-sensor; если в старой конфигурации оператор ожидал, что `dT` всегда игнорируется для главных каналов, поведение изменится.
- Размер прошивки вырос до ~96% flash, поэтому дальнейшие крупные добавления в web UI или diagnostics потребуют экономии flash.

## Что не удалось завершить и почему

- Live-проверка на железе не запускалась по требованию этого хода.
- Поведение ACK-кнопки на GPIO14 после power-on/reset нужно подтвердить на реальной плате из-за особенностей strapping pin.

## Фактически выполненные проверки

- `python -m unittest -v tools.test_logic_scheme tools.test_logic_full_matrix` -> `124 tests`, `OK`
- `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` -> успешно
- Размер сборки: flash `1260424` байт (`96%`), RAM `59472` байт (`18%`)
