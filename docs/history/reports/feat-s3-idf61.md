# Отчёт: C1 — esp-swd-probe на ESP32-S3 / ESP-IDF 6.1 (ветка feat/s3-idf61)

Задание b24-967, тред p953-swd-c1, аренда 626. Плату не трогали.

## Сделано
1. **Сборка под esp32s3 на IDF 6.1** — исходный проект собрался как есть (после `git submodule update --init
   components/corsacOTA`: пустой каталог подмодуля давал `corsacOTA.h: No such file`). Другой версии IDF не требуется.
   Исходный образ Wi-Fi/USBIP: 0xcaee0 байт.
2. **Kconfig** (`main/Kconfig.projbuild`): выводы SWDIO/SWCLK/nRESET (+TDO/TDI/nTRST), частота SWD
   (`DAP_SWD_CLOCK_HZ`, умолчание 1 МГц), Wi-Fi SSID/пароль (пусто; зашитые `OTA/DAP:12345678` убраны из
   `wifi_configuration.h`). Умолчания: 11/12/13 (+9/10/14) — свободные выводы девкита. Запрещённые (0,3,19,20,26–37,45,46)
   отказывают на сборке. `spi_switch.c`: выводы S3 больше не зашиты в 11/12; на 11/12 — IO_MUX как раньше, на других — матрица GPIO.
3. **USB-режим по умолчанию** (`DAP_TRANSPORT_USB`, S3): в оригинале нативного USB не было (только USB/IP по Wi-Fi), поэтому
   добавлен `main/usb_dap.c` — CMSIS-DAP v2 (vendor bulk 0x02/0x81, MS OS 2.0 WinUSB + GUID openocd, строка интерфейса
   «CMSIS-DAP»), поверх `espressif/esp_tinyusb` ^2.0 (в `main/idf_component.yml`) и `DAP_ExecuteCommand`. Пакет 64 байта.
   Wi-Fi в USB-режиме не стартует.
4. `README-RU.md`: раздел «Сборка у нас», выводы, предупреждение про nRF52840 SuperMini (1,8 В до UICR, сдвиг уровня от VTG).

## Проверка
- `idf.py build` (IDF v6.1): USB-режим — `wireless_esp_dap.bin` 0x36060 (86% раздела свободно); Wi-Fi-режим
  с выводами 4/5/6 — 0xcae80 (46% свободно); вывод 19 — отказ `static assertion failed`.
- Не проверено на железе: enumeration/WinUSB, обмен DAP, SWD до цели, выводы вне 11/12. Это этап С2 лида.

## Известные ограничения
- В Wi-Fi-режиме пустой SSID ведёт к попыткам подключения к сети с пустым именем — задавать SSID в menuconfig.
- `dependencies.lock` закоммичен (версии tinyusb 0.21.0~6, esp_tinyusb 2.4.0, mdns 1.15.1).
