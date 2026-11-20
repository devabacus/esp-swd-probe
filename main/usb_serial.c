#include "usb_serial.h"

void usb_serial_from_mac(const uint8_t mac[6], char out[USB_SERIAL_LEN + 1])
{
    (void)mac;
    out[0] = '\0'; // заглушка: тест обязан покраснеть
}
