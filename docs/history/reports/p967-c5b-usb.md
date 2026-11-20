# p967-c5b-usb — CMD_INFO без ответа на образе d02572e (ESP32-S3 USB)

Ветка `feat/s3-usb-packets`, PR не открыт. Железо — лид.

## Вывод по гипотезе «приём не вооружается сам»
По исходникам TinyUSB (managed_components/espressif__tinyusb 0.21.0~6, `src/class/vendor/vendor_device.c`) гипотеза **не подтверждается**:
- RX_BUFSIZE=0 ⇒ `CFG_TUD_VENDOR_TXRX_BUFFERED=0` (vendor_device.h:67-68), режим «без FIFO».
- Приём на OUT вооружается сам в `vendord_open`: строки 326-329 (`usbd_edpt_xfer(... epout, rx_xfer_len)` при `CFG_TUD_VENDOR_RX_MANUAL_XFER==0`, по умолчанию 0 — vendor_device.h:73-74).
- После каждого пакета: `vendord_xfer_cb` строки 366-371 — коллбэк `tud_vendor_rx_cb(idx, epout, xferred)` и повторный `usbd_edpt_xfer` (369-371). Явный `tud_vendor_read_xfer` нужен только при `RX_MANUAL_XFER=1` (134-148).
- Ответ без буфера: `tud_vendor_n_write` строки 161-168 — `usbd_edpt_claim` + memcpy + `usbd_edpt_xfer` сразу, flush не нужен (его в этом режиме нет — 185-198 под `TXRX_BUFFERED`). Возвращает 0 при занятой/закрытой IN (163).
Значит код usb_dap.c логически верен для режима без FIFO.

## Наиболее вероятная причина (по коду, не проверено на железе)
Устаревший `sdkconfig` у лида: `sdkconfig.defaults.esp32s3` получил `RX_BUFSIZE=0`, а сгенерированный `sdkconfig` остался с 64 (defaults применяются только при создании). Тогда `TXRX_BUFFERED=1`, и `tud_vendor_rx_cb` получает `buffer=NULL, bufsize=0` (vendor_device.c:351, 347-354): данные лежат во FIFO, наш обработчик кладёт в кольцо пустые пакеты, DAP не исполняет ничего ⇒ «CMD_INFO failed» на самом первом запросе. Устройство при этом перечисляется нормально. Это точно совпадает с симптомом. Чистая сборка в этом дереве (свежий sdkconfig, RX=0) собирается.

## Что сделано (main/usb_dap.c)
1. `#error` при `CFG_TUD_VENDOR_TXRX_BUFFERED` — сборка со старым sdkconfig теперь падает громко и называет лечение («удалить sdkconfig»), а не даёт немой прибор.
2. Цикл `tud_vendor_write` ограничен `SEND_TIMEOUT_TICKS=200` — раньше при закрытой IN/ушедшем хосте задача DAP зависала навсегда; теперь предупреждение и сброс ответа.
3. Диагностика: первые 8 обменов печатаются (`dap #n: rx N B cmd 0x.. -> resp N B, sent N`) — по логу сразу видно, доходит ли запрос и уходит ли ответ.
Кольцо пакетов и `DAP_PACKET_COUNT=8` не тронуты; хост-тест `sh tests/host/run.sh` — GREEN. Тест на `#error` не писался: это проверка препроцессора на сборке.

## Проверка
- `idf.py set-target esp32s3 && idf.py build` (IDF v6.1) → rc=0.
- Подмодуль `components/corsacOTA` нужен: `git submodule update --init components/corsacOTA`.

## Для лида на стенде
1. `rm sdkconfig` (или `idf.py fullclean`), `idf.py set-target esp32s3`, пересобрать, залить; убедиться по `grep VENDOR sdkconfig`: `RX_BUFSIZE=0`.
2. Смотреть монитор: строки `dap #1..8` — если `rx` не появляется, приём не работает (тогда нужны логи); если `rx` есть, а `sent 0`/`tx timeout` — проблема IN.
3. Если и со свежим sdkconfig молчит — пришлите журнал загрузки; следующий шаг — проверка `usbd_edpt_claim`/мьютекса между ядрами.
