# esp-swd-probe — наш форк wireless-esp8266-dap

Форк [windowsair/wireless-esp8266-dap](https://github.com/windowsair/wireless-esp8266-dap) (MIT): программатор/отладчик
CMSIS-DAP на ESP32/ESP32-S3. Общее описание, схемы и режимы Wi-Fi — в `README.md` (оригинал). Здесь — только то, что
добавлено у нас.

## Сборка у нас

Цель — ESP32-S3 (девкит), режим по умолчанию — **USB (CMSIS-DAP v2, нативный USB S3, без Wi-Fi)**.

- **ESP-IDF 6.1** (проверено на v6.1, `xtensa-esp-elf` 15.2.0). Оригинал рассчитан на IDF 4.x/5.x; на 6.1 пришлось
  только заменить `ESP_IF_WIFI_STA` → `WIFI_IF_STA` (уже было в коде) — других поломок не найдено.
- Подмодуль нужен для сборки: `git submodule update --init components/corsacOTA` (`ESP8266_RTOS_SDK` для S3 не нужен).
- Зависимости из реестра компонентов (`espressif/mdns`, `espressif/esp_tinyusb`) тянутся при первой сборке, версии
  зафиксированы в `dependencies.lock`.

```bash
. $IDF_PATH/export.sh
idf.py set-target esp32s3        # если sdkconfig остался от другой цели или от старых defaults — удалить sdkconfig
idf.py menuconfig                # wireless dap config: транспорт, выводы, Wi-Fi, частота
idf.py build
idf.py -p <порт> flash
```

Размер образа: USB-режим — 0x36060 байт (221 КБ), Wi-Fi/USBIP-режим — 0xcae80 (831 КБ); раздел приложения 0x177000.

### Настройки (menuconfig → «wireless dap config»)

| Параметр | Kconfig | По умолчанию |
| --- | --- | --- |
| Транспорт | `DAP_TRANSPORT_USB` / `DAP_TRANSPORT_WIFI_USBIP` | USB на S3 |
| SWDIO | `DAP_PIN_SWDIO` | GPIO11 (FSPID) |
| SWCLK | `DAP_PIN_SWCLK` | GPIO12 (FSPICLK) |
| nRESET | `DAP_PIN_NRESET` | GPIO13 |
| TDO / TDI / nTRST (только JTAG) | `DAP_PIN_TDO` / `DAP_PIN_TDI` / `DAP_PIN_NTRST` | GPIO9 / GPIO10 / GPIO14 |
| Частота SWD по умолчанию | `DAP_SWD_CLOCK_HZ` | 1 000 000 Гц |
| Wi-Fi SSID / пароль (только Wi-Fi-режим) | `DAP_WIFI_SSID` / `DAP_WIFI_PASSWORD` | пусто, в git не хранятся |
| USB VID / PID (USB-режим) | `DAP_USB_VID` / `DAP_USB_PID` | 0x303A / 0x4001 |

- Запрещённые выводы (отказ на сборке, `_Static_assert` в `DAP_config.h`): 0, 3, 45, 46 (strapping), 19, 20 (нативный USB),
  26–37 (flash/PSRAM). Выводы 43/44 — UART0 консоли, лучше не занимать.
- Частота `DAP_SWD_CLOCK_HZ` — только значение до тех пор, пока хост не задал свою (pyOCD/OpenOCD делают это сами).
- SWDIO/SWCLK на GPIO11/12 идут напрямую через IO_MUX (быстрее всего). Любые другие выводы — через матрицу GPIO
  (код есть и собирается, **на железе не проверен**).

### Подключение к целевой плате

`SWDIO`, `SWCLK`, `nRESET` на выводы по таблице + общая земля. Питание цели — своё; напряжение сигналов ESP32-S3 — 3,3 В.

> ⚠ **nRF52840 SuperMini (новый): выводы питаются от 1,8 В до записи UICR.** Подключать SWDIO/SWCLK/nRESET
> **только через сдвиг уровня с опорой от VTG модуля**, а не напрямую на 3,3 В ESP32-S3. Источник — lead-boards,
> sd-nrf52840, `docs/codex-review-2026-10-27-r2.md:27`.

### Что проверено, что нет

- Проверено: `idf.py build` зелёный на ESP-IDF 6.1 в трёх вариантах — USB (умолчание), Wi-Fi/USBIP, Wi-Fi + выводы вне
  IO_MUX (4/5/6); запрещённый вывод (19) отказывает на сборке.
- **Не проверено на железе** (плату не трогали, стенд — отдельный этап): enumeration CMSIS-DAP v2 в Windows (WinUSB через
  MS OS 2.0), прохождение пакетов DAP, SWD до цели, вариант выводов вне 11/12. Пакет DAP в USB-режиме — 64 байта (full-speed).
