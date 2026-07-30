# Контекст проекта RectColumn

Промышленный контроллер ректификационной колонны на ESP32. Веб-сервер в браузере через WiFi.

## Архитектурные принципы (НЕ нарушать)

1. **Сигнализация и управление реле — независимые ветви.** Аларм не должен блокировать ручное управление, кроме случаев явного safetyForbid (только L, F).
2. **Подтверждения реле (confirm pins) — только визуальная индикация.** Не влияют на логику команд.
3. **STOP имеет высший приоритет** — сбрасывает manualWant, lastWant, lastForbid, operatorHoldOff.
4. **Ручное управление — низший приоритет**, блокируется автоматикой. Команда живёт ≤1 сек, потом превращается в NONE.
5. **При отключении датчика в UI** — все его флаги управления немедленно обнуляются.
6. **Аларм гасится только оператором** через выкл→вкл датчика в UI или аппаратной кнопкой квитирования (квитируется только звук+всплывашка, мигающая надпись остаётся).

## Состояния датчика
- **ВЫКЛЮЧЕН** — флаги управления = 0
- **ОШИБКА** — исключён из управления, активен аларм
- **НОРМА** — участвует в управлении

## Сборка и заливка

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 --build-path .build-esp32 .
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32 --build-path .build-esp32 .
```

Веб-интерфейс после заливки: `http://192.168.10.244`

## Генерация веб-ассетов

| Header | Источник | Команда |
|---|---|---|
| `WebPageRoot.h` | `TEMP/root.html` | `python tools/gen_web_assets.py --raw-string --input TEMP/root.html --output WebPageRoot.h --symbol PAGE_ROOT_V2 --length-symbol PAGE_ROOT_V2_LEN` |
| `WebPageWifi.h` | `TEMP/wifi.html` | `python tools/gen_web_assets.py --raw-string --input TEMP/wifi.html --output WebPageWifi.h --symbol PAGE_WIFI_V2 --length-symbol PAGE_WIFI_V2_LEN` |
| `WebPageAppCss.h` | `OUT/app.css` | `python tools/gen_web_assets.py --raw-string --input OUT/app.css --output WebPageAppCss.h --symbol PAGE_APP_CSS --length-symbol PAGE_APP_CSS_LEN` |
| `WebPageAppJs.h` | `OUT/page-app.js` | `python tools/gen_web_assets.py --raw-string --input OUT/page-app.js --output WebPageAppJs.h --symbol PAGE_APP_JS --length-symbol PAGE_APP_JS_LEN` |
| `WebUplotCss.h` | `OUT/uplot.min.css` | `python tools/gen_web_assets.py --input OUT/uplot.min.css --output WebUplotCss.h --symbol UPLOT_CSS_GZ --length-symbol UPLOT_CSS_GZ_LEN` |
| `WebUplotJs.h` | `OUT/uplot.min.js` | `python tools/gen_web_assets.py --input OUT/uplot.min.js --output WebUplotJs.h --symbol UPLOT_JS_GZ --length-symbol UPLOT_JS_GZ_LEN` |

Флаг `--raw-string` пишет читаемый `const char[] PROGMEM` через C++ raw-string literal и подбирает уникальный разделитель так, чтобы закрывающая последовательность не встретилась внутри ассета. Флаг `--raw` пишет исходные байты без gzip в `uint8_t[]`. Без флагов генератор сохраняет текущее gzip-поведение.

## Стиль кода
- camelCase для функций/переменных, `_private` для приватных членов
- SNAKE_CASE для констант `#define`
- Без `.clang-format`, держаться стиля существующих файлов
- Комментарии на русском допустимы

## Версионирование
- Текущая версия в `config.h::FW_VERSION`
- Обновлять при каждом релизном изменении

## Правила работы
- Создавать отдельную ветку под каждый ERR: `fix/err-XX-краткое-описание`
- Один ERR = один PR
- В коммите указывать `ERR-XX: <что сделано>`
- Прогонять `tests/` после правки
- Если правка ломает другие тесты — остановиться и сообщить
