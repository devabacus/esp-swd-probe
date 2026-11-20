# p967-c9: сторож порта S3 без железа

Ветка `test/s3-port-guards` от `origin/feat/s3-idf61` (5f7663f). PR не открывался.

Добавлено:
- `tools/check_s3_port.py` — разбор исходников; отказывает, если (а) `DAP_SPI_Deinit` под S3 не включает тактирование
  выделенного GPIO, (б) `PIN_SWDIO_OUT_ENABLE/DISABLE` под S3 пусты, (в) `DAP_PACKET_COUNT` > 1 при потоковом
  `tud_vendor_read` в `main/usb_dap.c` (обход — маркер `S3_PORT_PER_TRANSFER_READ` при чтении по границам передач).
  Комментарии при разборе отбрасываются: проверка в комментарии не засчитывается.
- `tools/test_check_s3_port.py` — 7 тестов: зелёное дерево и мутации по каждой проверке.
- `.github/workflows/s3-port.yml` — сторож + тесты, затем `idf.py build` esp32s3 на IDF v6.1.

## Красный на 55a39b8 (до правок), код 1
```
ОТКАЗ: (а) DAP_SPI_Deinit под S3 не включает тактирование выделенного GPIO ...
ОТКАЗ: (б) PIN_SWDIO_OUT_ENABLE под S3 пуста
ОТКАЗ: (б) PIN_SWDIO_OUT_DISABLE под S3 пуста
ОТКАЗ: (в) DAP_PACKET_COUNT=255 для S3 при потоковом tud_vendor_read в main/usb_dap.c ...
check_s3_port: КРАСНЫЙ
```
## Зелёный на 5f7663f, код 0
```
check_s3_port: зелёный
Ran 7 tests ... OK
```
Не проверено: сам CI-прогон `idf.py build` (в облаке без IDF 6.1 не гонялся).
