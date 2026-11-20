#ifndef __USB_DAP_H__
#define __USB_DAP_H__

/**
 * @brief CMSIS-DAP v2 over the native USB (WinUSB bulk) of the ESP32-S3.
 *
 * Enabled by CONFIG_DAP_TRANSPORT_USB. Starts TinyUSB and a task that runs
 * every request packet through DAP_ExecuteCommand().
 */
void usb_dap_start(void);

#endif
