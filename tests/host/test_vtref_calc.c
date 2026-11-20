// Host test for components/DAP/source/vtref_calc.c. Run: tests/host/run.sh
#include <stdio.h>
#include <string.h>
#include "vtref_calc.h"

static int fails;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); fails++; } } while (0)

// Deliberately NOT the Kconfig defaults, so the test proves thresholds are parameters.
static const vtref_thresholds_t T = { .none_below_mv = 900, .direct_min_mv = 2500, .over_mv = 3700 };

int main(void) {
    // raw -> pin mV (linear fallback)
    CHECK(vtref_raw_to_pin_mv(0, 4095, 3100) == 0);
    CHECK(vtref_raw_to_pin_mv(4095, 4095, 3100) == 3100);
    CHECK(vtref_raw_to_pin_mv(2048, 4095, 3100) == 1550);   // 1550.4, round to nearest
    CHECK(vtref_raw_to_pin_mv(9999, 4095, 3100) == 3100);   // clamp
    CHECK(vtref_raw_to_pin_mv(10, 0, 3100) == 0);           // raw_max 0: no division by zero

    // divider
    CHECK(vtref_pin_to_target_mv(1650, 0, 0) == 1650);          // no divider
    CHECK(vtref_pin_to_target_mv(1650, 10000, 0) == 1650);      // bottom 0 = no divider
    CHECK(vtref_pin_to_target_mv(1650, 10000, 10000) == 3300);  // 1:1
    CHECK(vtref_pin_to_target_mv(1000, 20000, 10000) == 3000);  // 1:3
    CHECK(vtref_pin_to_target_mv(1000, 0, 10000) == 1000);      // top 0: ratio 1
    CHECK(vtref_pin_to_target_mv(60000, 100000, 1000) == 65535); // saturates

    // classification, boundaries
    CHECK(vtref_classify(0, &T) == VTREF_NONE);
    CHECK(vtref_classify(899, &T) == VTREF_NONE);
    CHECK(vtref_classify(900, &T) == VTREF_1V8);
    CHECK(vtref_classify(1800, &T) == VTREF_1V8);
    CHECK(vtref_classify(2499, &T) == VTREF_1V8);
    CHECK(vtref_classify(2500, &T) == VTREF_3V3);
    CHECK(vtref_classify(3300, &T) == VTREF_3V3);
    CHECK(vtref_classify(3699, &T) == VTREF_3V3);
    CHECK(vtref_classify(3700, &T) == VTREF_OVER);
    CHECK(vtref_classify(5000, &T) == VTREF_OVER);
    // thresholds really move the result
    vtref_thresholds_t U = { .none_below_mv = 2000, .direct_min_mv = 3000, .over_mv = 3400 };
    CHECK(vtref_classify(1800, &U) == VTREF_NONE);
    CHECK(vtref_classify(2800, &U) == VTREF_1V8);
    CHECK(vtref_classify(3300, &U) == VTREF_3V3);
    CHECK(vtref_classify(3400, &U) == VTREF_OVER);

    // names
    CHECK(strlen(vtref_class_name(VTREF_NONE)) > 0);
    CHECK(strcmp(vtref_class_name(VTREF_NONE), vtref_class_name(VTREF_3V3)) != 0);
    CHECK(strcmp(vtref_class_name(VTREF_1V8), vtref_class_name(VTREF_OVER)) != 0);

    if (fails) { printf("%d failed\n", fails); return 1; }
    printf("test_vtref_calc: OK\n");
    return 0;
}
