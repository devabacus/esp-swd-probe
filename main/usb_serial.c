#include "usb_serial.h"

void usb_serial_from_mac(const uint8_t mac[6], char out[USB_SERIAL_LEN + 1])
{
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) {
        out[2 * i] = hex[mac[i] >> 4];
        out[2 * i + 1] = hex[mac[i] & 0x0F];
    }
    out[USB_SERIAL_LEN] = '\0';
}
