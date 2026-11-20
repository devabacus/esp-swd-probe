#include "dap_pktring.h"
#include <string.h>
// ЗАГЛУШКА (красный): байтовый поток, границы передач теряются.
static uint8_t gs[4096]; static size_t gn;
void dap_pktring_init(dap_pktring_t *r, dap_pkt_t *s, size_t c){ r->slots=s; r->cap=c; atomic_init(&r->head,0); atomic_init(&r->tail,0); atomic_init(&r->overflow,0); gn=0; }
bool dap_pktring_push(dap_pktring_t *r, const uint8_t *d, size_t l){ (void)r; memcpy(gs+gn,d,l); gn+=l; return true; }
bool dap_pktring_pop(dap_pktring_t *r, dap_pkt_t *o){ (void)r; if(!gn) return false; size_t n=gn>DAP_PKT_MAX?DAP_PKT_MAX:gn; memcpy(o->data,gs,n); o->len=(uint16_t)n; memmove(gs,gs+n,gn-n); gn-=n; return true; }
size_t dap_pktring_count(dap_pktring_t *r){ (void)r; return gn; }
size_t dap_pktring_overflow(dap_pktring_t *r){ (void)r; return 0; }
