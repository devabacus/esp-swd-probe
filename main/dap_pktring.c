#include "dap_pktring.h"

#include <string.h>

void dap_pktring_init(dap_pktring_t *r, dap_pkt_t *slots, size_t cap)
{
    r->slots = slots;
    r->cap = cap;
    atomic_init(&r->head, 0);
    atomic_init(&r->tail, 0);
    atomic_init(&r->overflow, 0);
}

bool dap_pktring_push(dap_pktring_t *r, const uint8_t *data, size_t len)
{
    size_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
    size_t tail = atomic_load_explicit(&r->tail, memory_order_acquire);
    if (len > DAP_PKT_MAX || head - tail >= r->cap) {
        atomic_fetch_add_explicit(&r->overflow, 1, memory_order_relaxed);
        return false; // новый пакет теряем, уже принятые и их порядок не трогаем
    }
    dap_pkt_t *slot = &r->slots[head % r->cap];
    slot->len = (uint16_t)len;
    if (len) {
        memcpy(slot->data, data, len);
    }
    atomic_store_explicit(&r->head, head + 1, memory_order_release);
    return true;
}

bool dap_pktring_pop(dap_pktring_t *r, dap_pkt_t *out)
{
    size_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed);
    size_t head = atomic_load_explicit(&r->head, memory_order_acquire);
    if (head == tail) {
        return false;
    }
    *out = r->slots[tail % r->cap];
    atomic_store_explicit(&r->tail, tail + 1, memory_order_release);
    return true;
}

size_t dap_pktring_count(dap_pktring_t *r)
{
    return atomic_load(&r->head) - atomic_load(&r->tail);
}

size_t dap_pktring_overflow(dap_pktring_t *r)
{
    return atomic_load(&r->overflow);
}
