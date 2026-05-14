// ================================================================
// config.h – Файл конфигурации
// ================================================================
#pragma once
#include <Arduino.h>

// ─── Флаги сборки ────────────────────────────────────────────────
// true  -> аппаратное оборудование НЕ опрашивается,
//          значения берутся из /api/v1/emu/*
// false -> режим реального оборудования
#define EMU_MODE true

// Включить периодический (каждые 5 секунд) вывод в Serial
// состояния всех датчиков и подтверждений.
// Установите 0 для production-сборки, если нужен тихий Serial.
#define SERIAL_DEBUG_SENSOR_SNAPSHOT  0

// ─── Версия прошивки / API ───────────────────────────────────────
#define FW_VERSION      "1.6.25"
#define API_VERSION     "v1"
#define AP_SSID_DEF     "Control_System"
#define DEVICE_NAME     "RectColumn"
// ─── Локальный WiFi для отладки  ─────────────────────────────────
// #define STA_SSID_DEF    "EnterFi"
// #define STA_PASS_DEF    "22QQkWmDianTwfzvx2Qo"

// ─── Режим общей функции GPIO35 ──────────────────────────────────
// GPIO35 не может одновременно работать как аналоговый вход V
// и как вход подтверждения WER_CH2.
// Выберите ОДИН режим на этапе компиляции для реального железа.
//
// GPIO35_MODE_V_SENSOR -> работает V, WER_CH2 недоступен
// GPIO35_MODE_WER_CH2  -> работает WER_CH2, V недоступен
#define GPIO35_MODE_V_SENSOR   0
#define GPIO35_MODE_WER_CH2    1
#define GPIO35_MODE            GPIO35_MODE_WER_CH2

// ─── DS18B20 (по одному датчику на шину OneWire) ─────────────────
#define PIN_T1   16
#define PIN_T2   17
#define PIN_T3   18

// ─── BMP180 I2C ──────────────────────────────────────────────────
#define PIN_BMP_SDA  21
#define PIN_BMP_SCL  22

// ─── Дискретные входы ────────────────────────────────────────────
// Датчики L/F — это сухие контакты:
// замкнуто => вход в HIGH => норма / есть проток,
// разомкнуто => вход удерживается в LOW через pull-down => авария.
// Для PIN_L/PIN_F в этой ревизии включается внутренняя подтяжка к GND
// (INPUT_PULLDOWN), т.к. эти GPIO её поддерживают.
#define PIN_L   19
#define PIN_F   23

// ─── Аналоговые входы ────────────────────────────────────────────
// C: GPIO39 = ADC1_CH3. Работает только как вход.
// V: в текущей распайке отдельный вход не доступен,
//    потому что GPIO35 отдан под подтверждение CH2.
#define PIN_C   39
#define PIN_V   35

// ─── Входы подтверждения реле ────────────────────────────────────
// Активное подтверждение должно поднимать вход в HIGH.
// Для PIN_WER_CH1 (GPIO27) включается внутренняя подтяжка к GND
// (INPUT_PULLDOWN), т.к. этот GPIO её поддерживает.
// Для PIN_WER_CH2/CH3/CH4 (GPIO35/34/36) внутренней подтяжки нет,
// поэтому для них по-прежнему нужна внешняя подтяжка к GND.
#define WER_ACTIVE_LOW    false
#define PIN_WER_CH1       27   // подтверждение CH1 / реле 220
#define PIN_WER_CH2       35   // подтверждение CH2 / клапан 2
#define PIN_WER_CH3       34   // подтверждение CH3 / клапан 1
#define PIN_WER_CH4       36   // подтверждение CH4 / внешний звонок

// ------------------------------------------------------------
// Профиль подтверждений / звука
// ------------------------------------------------------------
// Отдельного входа "WER_BELL" в этом профиле нет.
// Подтверждение внешнего звонка используется как WER_CH4.
// CH5 остаётся отдельным встроенным буззером на GPIO13.
#define RECT_HW_HAS_WER_BELL   0
#define PIN_WER_BELL          -1

// ─── Выходные пины ───────────────────────────────────────────────
#define PIN_CH1   26   // CH1: реле 220
#define PIN_CH2   25   // CH2: клапан 2
#define PIN_CH3   33   // CH3: клапан 1
#define PIN_CH4   32   // CH4: внешний звонок
#define PIN_CH5   13   // CH5: встроенный буззер
#define PIN_WIFI_LED 4 // LED индикации WiFi

static_assert(PIN_CH5 != PIN_WER_CH1, "PIN_CH5 conflicts with WER_CH1");
static_assert(PIN_CH5 != PIN_WER_CH2, "PIN_CH5 conflicts with WER_CH2");
static_assert(PIN_CH5 != PIN_WER_CH3, "PIN_CH5 conflicts with WER_CH3");
static_assert(PIN_CH5 != PIN_WER_CH4, "PIN_CH5 conflicts with WER_CH4");
static_assert(PIN_WIFI_LED != PIN_CH1, "PIN_WIFI_LED conflicts with CH1");
static_assert(PIN_WIFI_LED != PIN_CH2, "PIN_WIFI_LED conflicts with CH2");
static_assert(PIN_WIFI_LED != PIN_CH3, "PIN_WIFI_LED conflicts with CH3");
static_assert(PIN_WIFI_LED != PIN_CH4, "PIN_WIFI_LED conflicts with CH4");
static_assert(PIN_WIFI_LED != PIN_CH5, "PIN_WIFI_LED conflicts with CH5");
static_assert(PIN_WER_CH1 == 27, "Hardware map requires WER_CH1 on IO27");
static_assert(PIN_WER_CH2 == 35, "Hardware map requires WER_CH2 on IO35");
static_assert(PIN_WER_CH3 == 34, "Hardware map requires WER_CH3 on IO34");
static_assert(PIN_WER_CH4 == 36, "Hardware map requires WER_CH4 / bell feedback on IO36");
static_assert(PIN_V == PIN_WER_CH2, "V shares IO35 with WER_CH2 in this hardware profile");
static_assert(PIN_V != PIN_WER_CH1, "PIN_V conflicts with WER_CH1");
static_assert(PIN_V != PIN_WER_CH3, "PIN_V conflicts with WER_CH3");
static_assert(PIN_V != PIN_WER_CH4, "PIN_V conflicts with WER_CH4");

// ─── Паттерн звонка ──────────────────────────────────────────────
#define BELL_ON_MS     500UL
#define BELL_OFF_MS  20000UL
#define CMD_BEEP_MS      80UL
#define CMD_BEEP_COOLDOWN_MS 250UL
#define TEMP_ERROR_HOLD_MS 10000UL
#define WIFI_LED_BLINK_MS 500UL

// ─── Тайминги входов подтверждения ───────────────────────────────
#define WER_DEBOUNCE_MS            80UL
#define RELAY_CONFIRM_TIMEOUT_MS   1000UL
#define RELAY_CONFIRM_TIMEOUT_CH1_MS 1000UL
#define RELAY_CONFIRM_TIMEOUT_CH2_MS 5000UL   // клапан 2: подтверждение приходит заметно медленнее реле CH1
#define RELAY_CONFIRM_TIMEOUT_CH3_MS 5000UL   // клапан 1: подтверждение приходит заметно медленнее реле CH1
#define RELAY_CONFIRM_TIMEOUT_CH4_MS 1000UL
#define WER_CONFIRM_TIMEOUT_MS     RELAY_CONFIRM_TIMEOUT_MS

// ─── Жёсткие таймеры safety-слоя ────────────────────────────────
// Эти значения не редактируются через API и не зависят от ctrlDelayMs.
#define SAFETY_LEVEL_SHUTDOWN_MS   (5UL * 60UL * 1000UL)
#define SAFETY_FLOW_LOSS_MS        5000UL
#define SAFETY_PRESSURE_OFF_MS     0UL
#define DIGITAL_ALARM_DEBOUNCE_MS  500UL

// ─── Режим сенсорного STOP ──────────────────────────────────────
// false -> STOP используется только оператором вручную.
// true  -> safety-mode: подтверждённая авария уровня L
//          дополнительно защёлкивает STOP.
// Потеря потока F остаётся поканальной защитой через CH1FOF/CH2FOF/CH3FOF
// и не должна глобально отключать CH1-CH3.
#define SAFETY_MODE_SENSOR_STOP    false

// ─── Валидация пользовательских параметров ──────────────────────
#define SENSOR_PERIOD_MIN_MS       100UL
#define SENSOR_PERIOD_MAX_MS       (60UL * 60UL * 1000UL)
#define SENSOR_ALARM_DELAY_MAX_MS  (10UL * 60UL * 1000UL)
#define SENSOR_CTRL_DELAY_MAX_MS   (60UL * 60UL * 1000UL)
#define SENSOR_MAX_AGE_FACTOR      3UL
#define CTRL_MIN_DEADBAND          0.001f

// ─── WiFi / переподключение ──────────────────────────────────────
#define WIFI_CONNECT_TIMEOUT_MS      12000UL
#define WIFI_RECONNECT_COOLDOWN_MS   30000UL
#define WIFI_SCAN_RECONNECT_PAUSE_MS 15000UL
#define WIFI_RSSI_WARN_DBM           -75

// ─── Журнал событий ──────────────────────────────────────────────
#define LOG_MAX_ENTRIES  100

// ─── Отладка через Serial ────────────────────────────────────────
#define SERIAL_DEBUG_SNAPSHOT_INTERVAL_MS  5000UL

// ─── Диагностика стабильности (тестовый профиль) ─────────────────
#define STABILITY_BOOT_DIAG          1
#define STABILITY_HEARTBEAT_MS       30000UL

// ─── Индексы датчиков и выходов ──────────────────────────────────
#define SEN_T1    0
#define SEN_T2    1
#define SEN_T3    2
#define SEN_DT    3
#define SEN_P     4
#define SEN_L     5
#define SEN_F     6
#define SEN_C     7
#define SEN_V     8
#define SEN_COUNT 9

#define OUT_CH1   0
#define OUT_CH2   1
#define OUT_CH3   2
#define OUT_CH4   3
#define OUT_CH5   4
#define OUT_COUNT 5

// Дополнительные биты правил,
// используемые внутри OutputManager.
#define RULEIDX_MANUAL          (SEN_COUNT)
#define RULEIDX_SOUND           (SEN_COUNT + 1)
#define RULEIDX_STOP            (SEN_COUNT + 2)
#define RULEIDX_SAFETY_LEVEL    (SEN_COUNT + 3)
#define RULEIDX_SAFETY_FLOW     (SEN_COUNT + 4)
#define RULEIDX_SAFETY_PRESSURE (SEN_COUNT + 5)
#define RULEIDX_SAFETY_WER      (SEN_COUNT + 6)
#define RULEIDX_MAX             (SEN_COUNT + 7)

// ─── Режимы логики управления ────────────────────────────────────
#define LOGIC_HEAT  0
#define LOGIC_COOL  1

// ─── Периоды опроса датчиков по умолчанию ────────────────────────
#define DEF_T_PERIOD_MS       5000UL
#define DEF_P_PERIOD_MS       10000UL
#define DEF_FAST_PERIOD_MS    1000UL
#define STAGGER_SLOW_MS       1000UL

// ─── Коды диагностики датчиков ───────────────────────────────────
enum SensorDiagCode : uint8_t {
    SENSOR_DIAG_NONE = 0,
    SENSOR_DIAG_ADC2_WIFI_CONFLICT,
    SENSOR_DIAG_GPIO35_RESERVED_FOR_WER_CH2,
    SENSOR_DIAG_TEMP_RECOVERY_HOLD,
};
