// Серийный номер USB программатора — из MAC платы (12 шестнадцатеричных цифр, прописные).
// Зачем: две платы с одной прошивкой на одном ПК должны различаться (OpenOCD «adapter serial», замки стендов);
// до 20.11 серийный был зашит строкой «000001» у всех (Weight-system docs/grabli.md, 20.11, 65532b58).
#pragma once
#include <stdint.h>

#define USB_SERIAL_LEN 12

// out — буфер не меньше USB_SERIAL_LEN + 1.
void usb_serial_from_mac(const uint8_t mac[6], char out[USB_SERIAL_LEN + 1]);

// Серийный номер этой платы (usb_serial_esp.c, только в прошивке): MAC из eFuse, строка живёт всё время работы.
const char *usb_serial_get(void);
