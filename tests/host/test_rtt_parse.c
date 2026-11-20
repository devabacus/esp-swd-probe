#include <stdio.h>
#include <string.h>
#include "../../main/rtt_parse.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void put32(uint8_t *p, uint32_t v) { p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24; }

static void make_cb(uint8_t *cb, uint32_t nup, uint32_t ndown, uint32_t bufaddr, uint32_t size, uint32_t wr, uint32_t rd) {
    memset(cb, 0, 48);
    memcpy(cb, "SEGGER RTT", 10);
    put32(cb + 16, nup); put32(cb + 20, ndown);
    put32(cb + 24, 0x1000); put32(cb + 28, bufaddr); put32(cb + 32, size);
    put32(cb + 36, wr); put32(cb + 40, rd);
}

static char got[8][64]; static int ngot;
static void emit(const char *l, size_t n, void *c) { (void)c; if (ngot < 8) { memcpy(got[ngot], l, n); got[ngot][n] = 0; ngot++; } }

int main(void) {
    uint8_t mem[256]; uint8_t cb[48]; rtt_up_t up;
    // подпись: найдена на границе 4, не найдена на неровной, не найдена в пустом
    memset(mem, 0xAA, sizeof mem); memcpy(mem + 64, "SEGGER RTT", 10); memset(mem + 74, 0, 6);
    CHECK(rtt_find_signature(mem, sizeof mem) == 64);
    memset(mem, 0xAA, sizeof mem); memcpy(mem + 65, "SEGGER RTT", 10); memset(mem + 75, 0, 6);
    CHECK(rtt_find_signature(mem, sizeof mem) == -1);
    CHECK(rtt_find_signature(mem, 10) == -1);
    memset(mem, 0, sizeof mem);
    CHECK(rtt_find_signature(mem, sizeof mem) == -1);
    // подпись у самого края окна (неполная) — не находится
    memset(mem, 0xAA, sizeof mem); memcpy(mem + 248, "SEGGER R", 8);
    CHECK(rtt_find_signature(mem, sizeof mem) == -1);

    // разбор блока
    make_cb(cb, 3, 3, 0x20001000, 1024, 10, 4);
    CHECK(rtt_parse_cb(cb, 48, &up) == 0);
    CHECK(up.buf_addr == 0x20001000 && up.size == 1024 && up.wr_off == 10 && up.rd_off == 4);
    CHECK(rtt_parse_cb(cb, 47, &up) == -1);
    cb[0] = 'X'; CHECK(rtt_parse_cb(cb, 48, &up) == -2);
    make_cb(cb, 0, 3, 0x20001000, 1024, 0, 0); CHECK(rtt_parse_cb(cb, 48, &up) == -3);
    make_cb(cb, 99, 3, 0x20001000, 1024, 0, 0); CHECK(rtt_parse_cb(cb, 48, &up) == -3);
    make_cb(cb, 3, 3, 0x20001000, 0, 0, 0); CHECK(rtt_parse_cb(cb, 48, &up) == -4);
    make_cb(cb, 3, 3, 0x20001000, 16, 16, 0); CHECK(rtt_parse_cb(cb, 48, &up) == -4);
    make_cb(cb, 3, 3, 0x20001000, 16, 0, 40); CHECK(rtt_parse_cb(cb, 48, &up) == -4);

    // кольцо: пусто, обычное, обёртка, полное
    CHECK(rtt_ring_avail(5, 5, 16) == 0);
    CHECK(rtt_ring_avail(9, 5, 16) == 4);
    CHECK(rtt_ring_avail(2, 14, 16) == 4);
    CHECK(rtt_ring_avail(4, 5, 16) == 15);   // полное: size-1
    CHECK(rtt_ring_avail(16, 0, 16) == -1);
    CHECK(rtt_ring_avail(0, 16, 16) == -1);
    CHECK(rtt_ring_avail(0, 0, 0) == -1);

    rtt_plan_t p;
    CHECK(rtt_ring_plan(9, 5, 16, 0, &p) == 0 && p.off1 == 5 && p.len1 == 4 && p.len2 == 0 && p.new_rd == 9);
    CHECK(rtt_ring_plan(2, 14, 16, 0, &p) == 0 && p.off1 == 14 && p.len1 == 2 && p.len2 == 2 && p.new_rd == 2);
    CHECK(rtt_ring_plan(5, 5, 16, 0, &p) == 0 && p.len1 == 0 && p.len2 == 0 && p.new_rd == 5);
    CHECK(rtt_ring_plan(4, 5, 16, 0, &p) == 0 && p.off1 == 5 && p.len1 == 11 && p.len2 == 4 && p.new_rd == 4);
    CHECK(rtt_ring_plan(2, 14, 16, 3, &p) == 0 && p.len1 == 2 && p.len2 == 1 && p.new_rd == 1);
    CHECK(rtt_ring_plan(2, 14, 16, 1, &p) == 0 && p.len1 == 1 && p.len2 == 0 && p.new_rd == 15);
    CHECK(rtt_ring_plan(2, 15, 16, 1, &p) == 0 && p.len1 == 1 && p.len2 == 0 && p.new_rd == 0);  // rd дошёл до конца → 0
    CHECK(rtt_ring_plan(20, 0, 16, 0, &p) == -1);

    // строки: склейка через куски, CRLF, пустая строка, переполнение, flush
    char lb[8]; rtt_lines_t l; ngot = 0;
    rtt_lines_init(&l, lb, sizeof lb, emit, NULL);
    rtt_lines_feed(&l, (const uint8_t *)"ab", 2);
    CHECK(ngot == 0);
    rtt_lines_feed(&l, (const uint8_t *)"c\r\nde\n\nxyz", 10);
    CHECK(ngot == 3 && !strcmp(got[0], "abc") && !strcmp(got[1], "de") && !strcmp(got[2], ""));
    rtt_lines_flush(&l);
    CHECK(ngot == 4 && !strcmp(got[3], "xyz"));
    rtt_lines_flush(&l); CHECK(ngot == 4);
    ngot = 0;
    rtt_lines_feed(&l, (const uint8_t *)"0123456789\n", 11);   // cap 8: режется на куски, ничего не теряется
    { char all[64] = ""; for (int i = 0; i < ngot; i++) strcat(all, got[i]); CHECK(!strcmp(all, "0123456789")); }

    printf(fails ? "FAILED: %d\n" : "OK\n", fails);
    return fails != 0;
}
