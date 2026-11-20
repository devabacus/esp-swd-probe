// Серийный номер программатора для USB и CMSIS-DAP: заводской MAC платы (eFuse), см. usb_serial.h.
#include "usb_serial.h"
#include "esp_mac.h"

const char *usb_serial_get(void)
{
    static char s[USB_SERIAL_LEN + 1];
    if (s[0] == '\0') {
        uint8_t mac[6] = {0};
        if (esp_efuse_mac_get_default(mac) != ESP_OK) {
            return "000000000000";
        }
        usb_serial_from_mac(mac, s);
    }
    return s;
}
