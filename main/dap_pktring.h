/**
 * @file dap_pktring.h
 * @brief Кольцо пакетов DAP: одна USB-передача = один элемент, границы не теряются.
 *
 * Один производитель (задача TinyUSB) и один потребитель (задача DAP) — без блокировок.
 * Чистый C, без FreeRTOS: тестируется на хосте (tests/host).
 */
#pragma once

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DAP_PKT_MAX 64U // нативный USB S3: полноскоростной bulk, пакет ≤ 64 байт

typedef struct {
    uint16_t len;
    uint8_t data[DAP_PKT_MAX];
} dap_pkt_t;

typedef struct {
    dap_pkt_t *slots;
    size_t cap;
    atomic_size_t head;     // сколько пакетов записано (растёт всегда)
    atomic_size_t tail;     // сколько прочитано
    atomic_size_t overflow; // сколько пакетов отброшено из-за переполнения
} dap_pktring_t;

void dap_pktring_init(dap_pktring_t *r, dap_pkt_t *slots, size_t cap);
// Кладёт ОДИН пакет целиком. false — кольцо полно или len > DAP_PKT_MAX (пакет не принят, счётчик overflow растёт).
bool dap_pktring_push(dap_pktring_t *r, const uint8_t *data, size_t len);
// Забирает самый старый пакет целиком; false — пусто.
bool dap_pktring_pop(dap_pktring_t *r, dap_pkt_t *out);
size_t dap_pktring_count(dap_pktring_t *r);
size_t dap_pktring_overflow(dap_pktring_t *r);
