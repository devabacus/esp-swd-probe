// Target voltage (VTref) measurement on an ADC pin. ESP32-S3 only.
#pragma once
#include <stdint.h>

// Start the ADC and the log task. Safe to call once, before usb_dap_start().
void vtref_start(void);

// Last measurement: target supply in mV (saturated to 65535) and class
// (vtref_class_t: 0 none, 1 1.8V, 2 3.3V, 3 over). Measures on demand if the
// task has not produced a value yet.
void vtref_get(uint16_t *mv, uint8_t *cls);
