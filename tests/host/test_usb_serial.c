#include <stdio.h>
#include <string.h>
#include "usb_serial.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void t_format_is_mac_hex_upper(void)
{
    const uint8_t mac[6] = {0x30, 0x30, 0xF9, 0x66, 0x84, 0x50};
    char s[USB_SERIAL_LEN + 1];
    memset(s, 'x', sizeof s);
    usb_serial_from_mac(mac, s);
    CHECK(strcmp(s, "3030F9668450") == 0);
    CHECK(strlen(s) == USB_SERIAL_LEN);
}

static void t_two_boards_differ(void)
{
    // swd-probe-devkit (E0:72:A1:F5:D2:84) и swd-probe-devkit-2 (30:30:F9:66:84:50) — два программатора на одном ПК
    const uint8_t a[6] = {0xE0, 0x72, 0xA1, 0xF5, 0xD2, 0x84};
    const uint8_t b[6] = {0x30, 0x30, 0xF9, 0x66, 0x84, 0x50};
    char sa[USB_SERIAL_LEN + 1], sb[USB_SERIAL_LEN + 1];
    usb_serial_from_mac(a, sa);
    usb_serial_from_mac(b, sb);
    CHECK(strcmp(sa, "E072A1F5D284") == 0);
    CHECK(strcmp(sa, sb) != 0);
    CHECK(strcmp(sa, "000001") != 0);
}

int main(void)
{
    t_format_is_mac_hex_upper();
    t_two_boards_differ();
    if (fails) { printf("ПРОВАЛ: %d\n", fails); return 1; }
    printf("OK usb_serial\n");
    return 0;
}
