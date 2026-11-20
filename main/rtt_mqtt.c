#include "rtt_mqtt.h"

#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "components/DAP/include/swd_host.h"
#include "main/rtt_parse.h"
#include "main/wifi_handle.h"

static const char *TAG = "rtt_mqtt";

#define SCAN_CHUNK 1024           // читаем окно кусками; куски перекрываются на длину подписи
#define READ_MAX 512              // за один опрос — не больше (размер буфера устройства, не настройка)
#define LINE_BUF 256

typedef struct {
    bool enabled;
    uint32_t win_lo, win_hi;
    uint32_t poll_ms;
    char uri[96], user[48], pass[64];
} rtt_cfg_t;

static rtt_cfg_t s_cfg;
static esp_mqtt_client_handle_t s_mqtt;
static char s_topic[24];

// Настройки: Kconfig — запасные значения, NVS (namespace "rtt") переопределяет.
static void cfg_load(void) {
#ifdef CONFIG_RTT_MQTT_DEFAULT_ENABLE
    s_cfg.enabled = true;
#else
    s_cfg.enabled = false;   // запасное значение: режим выключен
#endif
    s_cfg.win_lo = CONFIG_RTT_SEARCH_START;
    s_cfg.win_hi = CONFIG_RTT_SEARCH_END;
    s_cfg.poll_ms = CONFIG_RTT_POLL_MS;
    snprintf(s_cfg.uri, sizeof s_cfg.uri, "%s", CONFIG_RTT_MQTT_URI);
    snprintf(s_cfg.user, sizeof s_cfg.user, "%s", CONFIG_RTT_MQTT_USER);
    snprintf(s_cfg.pass, sizeof s_cfg.pass, "%s", CONFIG_RTT_MQTT_PASSWORD);

    nvs_handle_t h;
    if (nvs_open("rtt", NVS_READONLY, &h) != ESP_OK) return;
    uint8_t en;
    if (nvs_get_u8(h, "en", &en) == ESP_OK) s_cfg.enabled = en != 0;
    nvs_get_u32(h, "win_lo", &s_cfg.win_lo);
    nvs_get_u32(h, "win_hi", &s_cfg.win_hi);
    nvs_get_u32(h, "poll_ms", &s_cfg.poll_ms);
    size_t n = sizeof s_cfg.uri;  nvs_get_str(h, "uri", s_cfg.uri, &n);
    n = sizeof s_cfg.user;        nvs_get_str(h, "user", s_cfg.user, &n);
    n = sizeof s_cfg.pass;        nvs_get_str(h, "pass", s_cfg.pass, &n);
    nvs_close(h);
    if (s_cfg.poll_ms < 10) s_cfg.poll_ms = 10;
}

static void emit_line(const char *line, size_t len, void *ctx) {
    (void)ctx;
    esp_mqtt_client_publish(s_mqtt, s_topic, line, (int)len, 0, 0);
}

// Ищет блок RTT в окне; 0 — нашёл и разобрал.
static int find_block(uint32_t *cb_addr, rtt_up_t *up) {
    static uint8_t chunk[SCAN_CHUNK];
    for (uint32_t a = s_cfg.win_lo; a < s_cfg.win_hi; a += SCAN_CHUNK - RTT_SIGNATURE_LEN) {
        uint32_t n = s_cfg.win_hi - a < SCAN_CHUNK ? s_cfg.win_hi - a : SCAN_CHUNK;
        if (!swd_read_memory(a, chunk, n)) return -1;
        int off = rtt_find_signature(chunk, n);
        if (off < 0) continue;
        uint8_t cb[RTT_CB_HEADER_LEN + RTT_UP_DESC_LEN];
        if (!swd_read_memory(a + off, cb, sizeof cb)) return -1;
        if (rtt_parse_cb(cb, sizeof cb, up) == 0) { *cb_addr = a + off; return 0; }
    }
    return -2;
}

static void rtt_task(void *arg) {
    (void)arg;
    static uint8_t rx[READ_MAX];
    static char line_buf[LINE_BUF];
    rtt_lines_t lines;
    rtt_lines_init(&lines, line_buf, sizeof line_buf, emit_line, NULL);
    uint32_t cb_addr = 0;
    rtt_up_t up = {0};
    bool have = false;
    bool idle_flushed = true;

    swd_init();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(s_cfg.poll_ms));
        if (!have) {
            if (!swd_init_debug()) continue;
            int r = find_block(&cb_addr, &up);
            if (r == 0) { have = true; ESP_LOGI(TAG, "RTT block at 0x%08x, up0 size %u", (unsigned)cb_addr, (unsigned)up.size); }
            else vTaskDelay(pdMS_TO_TICKS(1000));   // блока нет (цель не загрузилась) — не долбим SWD
            continue;
        }
        uint8_t wo[8];   // WrOff, RdOff идут подряд
        uint32_t d = cb_addr + RTT_CB_HEADER_LEN;
        rtt_plan_t p;
        if (!swd_read_memory(d + RTT_WROFF_OFFSET, wo, sizeof wo)) { have = false; continue; }
        uint32_t wr = wo[0] | wo[1] << 8 | wo[2] << 16 | (uint32_t)wo[3] << 24;
        uint32_t rd = wo[4] | wo[5] << 8 | wo[6] << 16 | (uint32_t)wo[7] << 24;
        if (rtt_ring_plan(wr, rd, up.size, READ_MAX, &p) != 0) { have = false; continue; }  // цель перезапущена
        uint32_t total = p.len1 + p.len2;
        if (total == 0) {
            if (!idle_flushed) { rtt_lines_flush(&lines); idle_flushed = true; }   // строка без '\n' не застревает
            continue;
        }
        if (!swd_read_memory(up.buf_addr + p.off1, rx, p.len1)) { have = false; continue; }
        if (p.len2 && !swd_read_memory(up.buf_addr, rx + p.len1, p.len2)) { have = false; continue; }
        if (!swd_write_word(d + RTT_RDOFF_OFFSET, p.new_rd)) { have = false; continue; }
        rtt_lines_feed(&lines, rx, total);
        idle_flushed = false;
    }
}

void rtt_mqtt_start(void) {
    cfg_load();
    if (!s_cfg.enabled) { ESP_LOGI(TAG, "off"); return; }
    if (s_cfg.uri[0] == '\0') { ESP_LOGE(TAG, "broker URI is empty, RTT-MQTT not started"); return; }
#if defined(CONFIG_DAP_TRANSPORT_USB)
    wifi_init();   // на USB-транспорте Wi-Fi иначе не поднимается
#endif
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(s_topic, sizeof s_topic, "rtt/%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_mqtt_client_config_t mc = {
        .broker.address.uri = s_cfg.uri,
        .credentials.username = s_cfg.user[0] ? s_cfg.user : NULL,
        .credentials.authentication.password = s_cfg.pass[0] ? s_cfg.pass : NULL,
    };
    s_mqtt = esp_mqtt_client_init(&mc);
    if (!s_mqtt) { ESP_LOGE(TAG, "mqtt init failed"); return; }
    esp_mqtt_client_start(s_mqtt);
    xTaskCreate(rtt_task, "rtt_mqtt", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "on: topic %s, window 0x%08x..0x%08x, poll %u ms", s_topic,
             (unsigned)s_cfg.win_lo, (unsigned)s_cfg.win_hi, (unsigned)s_cfg.poll_ms);
}
