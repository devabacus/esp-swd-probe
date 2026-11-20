// VTref: reads the target rail on an ADC1 pin (12 dB, curve-fitting calibration),
// divider/thresholds from Kconfig (DAP_VTREF_*), logs at start and every
// DAP_VTREF_LOG_PERIOD_MS, hands the value to the host via vendor command 0x80.
#include "sdkconfig.h"

#ifdef CONFIG_DAP_VTREF_ENABLE
#include "vtref.h"
#include "vtref_calc.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "vtref";

// ESP32-S3: ADC1 channel = GPIO - 1 (GPIO1..GPIO10).
#define VTREF_CHANNEL ((adc_channel_t)(CONFIG_DAP_PIN_VTREF - 1))
#define VTREF_RAW_MAX 4095  // 12 bit

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;  // NULL: linear fallback with DAP_VTREF_FULLSCALE_MV
static volatile uint32_t s_mv = 0;
static volatile uint8_t s_cls = 0;
static volatile bool s_valid = false;

static const vtref_thresholds_t s_thr = {
    .none_below_mv = CONFIG_DAP_VTREF_NONE_BELOW_MV,
    .direct_min_mv = CONFIG_DAP_VTREF_DIRECT_MIN_MV,
    .over_mv = CONFIG_DAP_VTREF_OVER_MV,
};

static uint32_t measure(int *raw_out) {
    uint32_t raw_sum = 0;
    int n = 0;
    for (int i = 0; i < CONFIG_DAP_VTREF_SAMPLES; i++) {
        int raw;
        if (adc_oneshot_read(s_adc, VTREF_CHANNEL, &raw) == ESP_OK) { raw_sum += (uint32_t)raw; n++; }
    }
    if (n == 0) { *raw_out = -1; return 0; }
    uint32_t raw = raw_sum / (uint32_t)n;
    *raw_out = (int)raw;
    uint32_t pin_mv;
    int cal_mv;
    if (s_cali && adc_cali_raw_to_voltage(s_cali, (int)raw, &cal_mv) == ESP_OK && cal_mv >= 0) {
        pin_mv = (uint32_t)cal_mv;
    } else {
        pin_mv = vtref_raw_to_pin_mv(raw, VTREF_RAW_MAX, CONFIG_DAP_VTREF_FULLSCALE_MV);
    }
    return vtref_pin_to_target_mv(pin_mv, CONFIG_DAP_VTREF_DIV_TOP_OHM, CONFIG_DAP_VTREF_DIV_BOTTOM_OHM);
}

static void update(bool log) {
    int raw;
    uint32_t mv = measure(&raw);
    if (raw < 0) { if (log) ESP_LOGW(TAG, "ADC read failed"); return; }
    vtref_class_t c = vtref_classify(mv, &s_thr);
    s_mv = mv;
    s_cls = (uint8_t)c;
    s_valid = true;
    if (log) ESP_LOGI(TAG, "VTref %u mV (raw %d, GPIO%d): %s", (unsigned)mv, raw, CONFIG_DAP_PIN_VTREF, vtref_class_name(c));
}

static void vtref_task(void *arg) {
    (void)arg;
    for (;;) {
        update(true);
        vTaskDelay(pdMS_TO_TICKS(CONFIG_DAP_VTREF_LOG_PERIOD_MS));
    }
}

void vtref_start(void) {
    adc_oneshot_unit_init_cfg_t ucfg = { .unit_id = ADC_UNIT_1 };
    if (adc_oneshot_new_unit(&ucfg, &s_adc) != ESP_OK) { ESP_LOGE(TAG, "ADC1 init failed"); return; }
    adc_oneshot_chan_cfg_t ccfg = { .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    if (adc_oneshot_config_channel(s_adc, VTREF_CHANNEL, &ccfg) != ESP_OK) { ESP_LOGE(TAG, "ADC channel config failed"); return; }
    adc_cali_curve_fitting_config_t kcfg = { .unit_id = ADC_UNIT_1, .chan = VTREF_CHANNEL,
                                              .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12 };
    if (adc_cali_create_scheme_curve_fitting(&kcfg, &s_cali) != ESP_OK) {
        s_cali = NULL;
        ESP_LOGW(TAG, "no ADC calibration, linear fallback (%d mV full scale)", CONFIG_DAP_VTREF_FULLSCALE_MV);
    }
    update(true);  // first line at start
#if CONFIG_DAP_VTREF_LOG_PERIOD_MS > 0
    xTaskCreate(vtref_task, "vtref", 3072, NULL, 1, NULL);
#endif
}

void vtref_get(uint16_t *mv, uint8_t *cls) {
    if (!s_valid && s_adc) update(false);
    *mv = (uint16_t)(s_mv > 65535 ? 65535 : s_mv);
    *cls = s_cls;
}
#endif /* CONFIG_DAP_VTREF_ENABLE */
