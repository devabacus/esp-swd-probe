// Pure VTref arithmetic (no ESP-IDF includes): ADC reading -> millivolts of the
// target supply, and classification against thresholds. Host-testable.
// Thresholds are SETTINGS (Kconfig DAP_VTREF_*), never baked into the logic.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    VTREF_NONE = 0,   // no supply on the target (below none_below_mv)
    VTREF_1V8 = 1,    // ~1.8 V: only through a level shifter
    VTREF_3V3 = 2,    // ~3.3 V: SWD pins may be wired directly
    VTREF_OVER = 3,   // above the pin limit (5 V target): do NOT connect directly
} vtref_class_t;

typedef struct {
    uint32_t none_below_mv;  // < this: no power
    uint32_t direct_min_mv;  // >= this (and < over_mv): 3.3 V, direct
    uint32_t over_mv;        // >= this: too high for the 3.3 V pins
} vtref_thresholds_t;

// Raw ADC code -> mV at the pin, linear fallback when no calibration is available.
uint32_t vtref_raw_to_pin_mv(uint32_t raw, uint32_t raw_max, uint32_t fullscale_mv);

// mV at the pin -> mV of the target through a resistor divider
// (r_top: from the target rail to the pin, r_bottom: from the pin to GND).
// r_bottom == 0 means "no divider": result == pin_mv. Result saturates at 65535.
uint32_t vtref_pin_to_target_mv(uint32_t pin_mv, uint32_t r_top_ohm, uint32_t r_bottom_ohm);

vtref_class_t vtref_classify(uint32_t target_mv, const vtref_thresholds_t *thr);

// Short human text for the log line.
const char *vtref_class_name(vtref_class_t c);

#ifdef __cplusplus
}
#endif
