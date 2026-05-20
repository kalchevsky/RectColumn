#line 1 "E:\\RECCOLUM\\v040\\SRC\\RectColumn\\docs\\COREDUMP.md"
# Диагностика core dump

## Что включено в проекте

- Для прошивки зафиксирована локальная схема разделов `partitions.csv`, совместимая с `arduino-esp32 3.3.8`.
- В ней есть раздел `coredump` размером `64K`.
- При старте прошивка печатает в `Serial` причину предыдущего сброса и, для `PANIC`/`INT_WDT`/`TASK_WDT`, адрес и размер сохранённого дампа.

## Почему схема разделов закреплена в репозитории

В текущем `arduino-esp32 3.3.8` дефолтная `default.csv` уже содержит `coredump`, но проект хранит локальный `partitions.csv`, чтобы:

- стандартная команда `arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .` использовала ту же разметку без дополнительных флагов;
- будущая смена дефолтов в core не отключила `coredump` незаметно.

Важно:

- изменение `partitions.csv` меняет `partitions.bin`;
- следующая прошивка перепишет таблицу разделов устройства;
- текущая версия файла повторяет layout дефолтной 4MB-схемы `arduino-esp32 3.3.8`, поэтому сама по себе не меняет разметку относительно этого core.

## Как увидеть наличие дампа

После паники или watchdog-сброса в UART должно появиться примерно:

```text
[BOOT] reset reason = 7 (паника ядра (PANIC))
[BOOT] coredump available: addr=0x3f0000 size=12345
[BOOT] use 'espcoredump.py info_corefile' to decode
```

Если дампа нет:

```text
[BOOT] coredump not found in flash
```

## Как извлечь и расшифровать

1. Собрать проект, чтобы получить актуальный ELF.
2. Считать из flash диапазон, который прошивка вывела в `Serial`.
3. Расшифровать дамп через `espcoredump.py`.

Пример команд:

```powershell
esptool.py read_flash <addr> <size> coredump.bin
espcoredump.py info_corefile -c coredump.bin .build-esp32/RectColumn.ino.elf
```

Если нужен более подробный разбор:

```powershell
espcoredump.py dbg_corefile -c coredump.bin .build-esp32/RectColumn.ino.elf
```

## Что проверить при смене ESP32 core

- В `sdkconfig.h` текущего профиля должны остаться включены `CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=1` и `CONFIG_ESP_COREDUMP_DATA_FORMAT_ELF=1`.
- Если новый core отключит flash coredump, прошивка выведет TODO-сообщение на старте, а этот документ нужно будет обновить вместе с новой схемой включения.
