#include "vtref_calc.h"

uint32_t vtref_raw_to_pin_mv(uint32_t raw, uint32_t raw_max, uint32_t fullscale_mv) {
    if (raw_max == 0) return 0;
    if (raw > raw_max) raw = raw_max;
    return (uint32_t)(((uint64_t)raw * fullscale_mv + raw_max / 2) / raw_max);
}

uint32_t vtref_pin_to_target_mv(uint32_t pin_mv, uint32_t r_top_ohm, uint32_t r_bottom_ohm) {
    uint64_t mv = pin_mv;
    if (r_bottom_ohm != 0) {
        mv = (mv * ((uint64_t)r_top_ohm + r_bottom_ohm) + r_bottom_ohm / 2) / r_bottom_ohm;
    }
    return mv > 65535 ? 65535 : (uint32_t)mv;
}

vtref_class_t vtref_classify(uint32_t target_mv, const vtref_thresholds_t *thr) {
    if (target_mv < thr->none_below_mv) return VTREF_NONE;
    if (target_mv >= thr->over_mv) return VTREF_OVER;
    if (target_mv >= thr->direct_min_mv) return VTREF_3V3;
    return VTREF_1V8;
}

const char *vtref_class_name(vtref_class_t c) {
    switch (c) {
    case VTREF_NONE: return "no power";
    case VTREF_1V8:  return "1.8V - only through a level shifter";
    case VTREF_3V3:  return "3.3V - direct";
    case VTREF_OVER: return "too high - do NOT connect directly";
    }
    return "?";
}
