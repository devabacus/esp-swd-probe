#include "rtt_parse.h"

#include <string.h>

static const uint8_t kSig[RTT_SIGNATURE_LEN] = {'S','E','G','G','E','R',' ','R','T','T',0,0,0,0,0,0};

static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int rtt_find_signature(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i + RTT_SIGNATURE_LEN <= len; i += 4) {
        if (memcmp(buf + i, kSig, RTT_SIGNATURE_LEN) == 0) return (int)i;
    }
    return -1;
}

int rtt_parse_cb(const uint8_t *cb, size_t len, rtt_up_t *out) {
    if (len < RTT_CB_HEADER_LEN + RTT_UP_DESC_LEN) return -1;
    if (memcmp(cb, kSig, RTT_SIGNATURE_LEN) != 0) return -2;
    uint32_t nup = rd32(cb + 16), ndown = rd32(cb + 20);
    if (nup < 1 || nup > RTT_MAX_BUFFERS || ndown > RTT_MAX_BUFFERS) return -3;
    const uint8_t *d = cb + RTT_CB_HEADER_LEN;
    rtt_up_t u = {rd32(d + 4), rd32(d + 8), rd32(d + RTT_WROFF_OFFSET), rd32(d + RTT_RDOFF_OFFSET)};
    if (u.size == 0 || u.wr_off >= u.size || u.rd_off >= u.size) return -4;
    *out = u;
    return 0;
}

int32_t rtt_ring_avail(uint32_t wr, uint32_t rd, uint32_t size) {
    if (size == 0 || wr >= size || rd >= size) return -1;
    return (int32_t)((wr + size - rd) % size);
}

int rtt_ring_plan(uint32_t wr, uint32_t rd, uint32_t size, uint32_t max_bytes, rtt_plan_t *out) {
    int32_t avail = rtt_ring_avail(wr, rd, size);
    if (avail < 0) return -1;
    uint32_t n = (uint32_t)avail;
    if (max_bytes && n > max_bytes) n = max_bytes;
    uint32_t to_end = size - rd;
    out->off1 = rd;
    out->len1 = n < to_end ? n : to_end;
    out->len2 = n - out->len1;
    out->new_rd = (rd + n) % size;
    return 0;
}

void rtt_lines_init(rtt_lines_t *l, char *buf, size_t cap, rtt_line_cb emit, void *ctx) {
    l->buf = buf; l->cap = cap; l->len = 0; l->emit = emit; l->ctx = ctx;
}

static void emit_line(rtt_lines_t *l) {
    size_t n = l->len;
    if (n > 0 && l->buf[n - 1] == '\r') n--;
    l->emit(l->buf, n, l->ctx);
    l->len = 0;
}

void rtt_lines_feed(rtt_lines_t *l, const uint8_t *data, size_t n) {
    for (size_t i = 0; i < n; i++) {
        char c = (char)data[i];
        if (c == '\n') { emit_line(l); continue; }
        if (l->len >= l->cap) emit_line(l);   // переполнение: отдаём кусок как есть
        l->buf[l->len++] = c;
    }
}

void rtt_lines_flush(rtt_lines_t *l) {
    if (l->len > 0) emit_line(l);
}
