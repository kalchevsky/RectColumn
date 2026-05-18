# FIX NOTES 1.6.32-hotfix2

## Что исправлено

### 1. Warm-up при повторном включении датчика

- В `SensorBase` добавлено 3-секундное окно `warm-up` после перехода `enabled: false -> true`.
- Пока окно активно, `evalCtrl()` возвращает нейтральный `0` и не отключает каналы по временно невалидному/ещё не обновлённому значению датчика.
- В API состояния датчиков добавлено поле `warmup`, чтобы UI и тесты видели это состояние явно.

### 2. Защита notifier от redirect-сценариев и burst-повторов

- В `RemoteNotifier` для HTTP-отправки зафиксированы:
  - `HTTPC_DISABLE_FOLLOW_REDIRECTS`
  - `HTTP/1.0`
  - `Connection: close`
- При ответах `301/302/307/308` notifier теперь возвращает явную ошибку:
  - `server returned redirect (HTTPS required?); use a publish URL that does not redirect`
- Стек `notify-worker` увеличен до `16384`.
- Запись `String`-полей конфигурации синхронизирована под существующим `_cfgMux`.
- Дополнительно после первого redirect-failure worker:
  - блокирует дальнейшие отправки до следующего `setConfig()`
  - сбрасывает очередь, чтобы не долбить один и тот же невалидный URL серией быстрых повторов

## Что проверено

- `python -m unittest -v tools.test_logic_scheme tools.test_logic_full_matrix` -> `129 tests`, `OK`
- `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` в real-режиме -> успешно
- `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` в EMU-режиме -> успешно
- `arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` -> успешно
- Live EMU:
  - `tools.test_api_emu_channel_sensor_matrix.EmuChannelSensorMatrixTests.test_reenabling_error_sensor_keeps_output_on_during_warmup` -> `OK`
  - `tools.test_notify_failure_emu` -> `OK`
- Direct notifier check:
  - `POST /api/v1/notify/test` с `http://httpbin.org/status/302` -> `400` и ожидаемый текст `server returned redirect ...`
- Burst worker check:
  - 10 alarm-событий через worker с URL `http://httpbin.org/status/302` -> без ребута, `workerReady=true`, в журнале есть `Notify failed: server returned redirect ...`
- Sanity-check для `http://ntfy.sh/...`:
  - на 2026-05-18 plain `POST` по `http://ntfy.sh/...` в нашем окружении отвечал `200`, без редиректа и без `PANIC`

## Важное замечание

- Изначальный redirect-guard сам по себе оказался недостаточным для серии быстрых worker-событий на гарантированном `302`-URL: до добавления блокировки повторных отправок удалось воспроизвести `PANIC`.
- Поэтому в hotfix оставлен дополнительный fail-fast на уровне worker/queue, а не только настройка `HTTPClient`.
