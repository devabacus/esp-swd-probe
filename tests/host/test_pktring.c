#include <stdio.h>
#include <string.h>
#include "dap_pktring.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void t_two_short_requests_not_glued(void)
{
    dap_pkt_t s[8]; dap_pktring_t r; dap_pkt_t o;
    dap_pktring_init(&r, s, 8);
    const uint8_t a[] = {0x00, 0x01};          // DAP_Info
    const uint8_t b[] = {0x11, 0x00, 0x00, 0x2D}; // DAP_SWJ_Clock
    CHECK(dap_pktring_push(&r, a, sizeof a));
    CHECK(dap_pktring_push(&r, b, sizeof b));
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.len == 2); CHECK(memcmp(o.data, a, 2) == 0);
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.len == 4); CHECK(memcmp(o.data, b, 4) == 0);
    CHECK(!dap_pktring_pop(&r, &o));
}

static void t_order_and_wrap(void)
{
    dap_pkt_t s[4]; dap_pktring_t r; dap_pkt_t o;
    dap_pktring_init(&r, s, 4);
    for (uint8_t round = 0; round < 10; round++) {
        for (uint8_t i = 0; i < 3; i++) { uint8_t p[2] = {round, i}; CHECK(dap_pktring_push(&r, p, 2)); }
        for (uint8_t i = 0; i < 3; i++) { CHECK(dap_pktring_pop(&r, &o)); CHECK(o.len == 2 && o.data[0] == round && o.data[1] == i); }
    }
    CHECK(dap_pktring_count(&r) == 0);
}

static void t_overflow_drops_newest_keeps_old(void)
{
    dap_pkt_t s[2]; dap_pktring_t r; dap_pkt_t o;
    dap_pktring_init(&r, s, 2);
    uint8_t p1[1] = {1}, p2[1] = {2}, p3[1] = {3};
    CHECK(dap_pktring_push(&r, p1, 1)); CHECK(dap_pktring_push(&r, p2, 1));
    CHECK(!dap_pktring_push(&r, p3, 1));
    CHECK(dap_pktring_overflow(&r) == 1);
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.data[0] == 1);
    CHECK(dap_pktring_push(&r, p3, 1)); // место освободилось
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.data[0] == 2);
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.data[0] == 3);
}

static void t_full_and_oversize(void)
{
    dap_pkt_t s[2]; dap_pktring_t r; dap_pkt_t o;
    dap_pktring_init(&r, s, 2);
    uint8_t big[DAP_PKT_MAX + 1] = {0}, full[DAP_PKT_MAX];
    memset(full, 0xA5, sizeof full);
    CHECK(!dap_pktring_push(&r, big, sizeof big)); // не режем молча
    CHECK(dap_pktring_overflow(&r) == 1);
    CHECK(dap_pktring_push(&r, full, sizeof full));
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.len == DAP_PKT_MAX && o.data[63] == 0xA5);
}

static void t_zero_length_packet(void)
{
    dap_pkt_t s[2]; dap_pktring_t r; dap_pkt_t o;
    dap_pktring_init(&r, s, 2);
    CHECK(dap_pktring_push(&r, (const uint8_t *)"", 0)); // ZLP — тоже передача
    CHECK(dap_pktring_pop(&r, &o)); CHECK(o.len == 0);
}

int main(void)
{
    t_two_short_requests_not_glued(); t_order_and_wrap();
    t_overflow_drops_newest_keeps_old(); t_full_and_oversize(); t_zero_length_packet();
    printf(fails ? "RED: %d\n" : "GREEN\n", fails);
    return fails ? 1 : 0;
}
