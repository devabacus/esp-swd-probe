# p967 С6 — замер напряжения цели (VTref)

Ветка `feat/s3-vtref` от `origin/feat/s3-idf61`. План b24-967 С6, владелец b24-107-48421. Аренда исполнителя 635.

## Что сделано
- **Чистая часть** `components/DAP/source/vtref_calc.c` (+ `include/vtref_calc.h`, без ESP-IDF): отсчёт АЦП → мВ на выводе
  (линейный запасной путь), пересчёт через делитель в мВ цели, классификация `нет питания / 1,8 В / 3,3 В / выше предела`.
  Пороги — параметр функции, в прошивке из Kconfig.
- **Прошивка** `components/DAP/source/vtref.c`: `adc_oneshot` ADC1, ослабление 12 дБ, калибровка curve fitting
  (при её отсутствии — линейный запас по `DAP_VTREF_FULLSCALE_MV`), усреднение `DAP_VTREF_SAMPLES`; строка в журнал
  при старте и раз в `DAP_VTREF_LOG_PERIOD_MS` (2000). Запуск из `app_main` в обеих ветках (USB и Wi-Fi).
- **Vendor-команда CMSIS-DAP 0x80** (`ID_DAP_Vendor0`, `DAP_vendor.c`): ответ `80 <мВ lo> <мВ hi> <класс>`.
  Выбор «команда 0x80», а не новый `DAP_Info` ID: так ответ несёт и число, и класс в одном обмене.
- **Kconfig** (меню «Target voltage (VTref)»): `DAP_VTREF_ENABLE`, `DAP_PIN_VTREF` (умолчание GPIO1 = ADC1_CH0; свободен при
  SWD 4/5/6 и JTAG 9/10/14; диапазон 1..10), `DAP_VTREF_DIV_TOP_OHM`/`BOTTOM_OHM` (умолчание 0 — делителя нет),
  `DAP_VTREF_FULLSCALE_MV`, `DAP_VTREF_SAMPLES`, `DAP_VTREF_LOG_PERIOD_MS`, пороги `DAP_VTREF_NONE_BELOW_MV` (1200),
  `DAP_VTREF_DIRECT_MIN_MV` (2700), `DAP_VTREF_OVER_MV` (3600). Запрещённые выводы и совпадение с выводами SWD/JTAG
  — `_Static_assert` в `DAP_config.h`.
- **Хост** `tools/vtref.py`: через OpenOCD `cmsis-dap cmd 80`, печатает мВ и вывод; `--raw-response` разбирает готовый ответ без железа.

## Отступление от задания
Добавлен четвёртый класс «выше 3,6 В — не подключать напрямую» (5-вольтовая цель без делителя убила бы вход): порог — тоже настройка.

## Проверка
- Хост-тест `tests/host/run.sh` (gcc, `-Wall -Wextra -Werror`): `test_vtref_calc` — сначала красный на заглушке (23 падения),
  потом зелёный; разбор ответа и тексты `tools/vtref.py` — `test_vtref_host.py` (8 тестов, зелёные).
  Тест берёт пороги НЕ из умолчаний Kconfig — доказывает, что они параметр.
- `idf.py set-target esp32s3 && idf.py build` (IDF 6.1): зелёная, символы `vtref_*` в образе.
  ⚠ Подмодуль `components/corsacOTA` в свежем клоне пуст — `git submodule update --init components/corsacOTA`, иначе `main.c` не собирается (до моих правок).

## Не проверено (честно)
- **На железе не проверялось**: ни АЦП на плате, ни ответ команды 0x80 по USB, ни работа `tools/vtref.py` с настоящим OpenOCD
  (формат вывода `cmsis-dap cmd` и то, что `init` без цели доходит до выполнения команды, — по памяти документации, не замером).
  Если `init` падает без цели, понадобится другой вызов (запасной путь — `--raw-response` и вызов команды любым другим клиентом).
- Точность АЦП: у S3 ADC1 на 12 дБ нелинеен у краёв шкалы (> ~3,1 В); калибровка curve fitting это компенсирует, делитель даёт запас.
