// Чистый разбор блока управления SEGGER RTT и кольцевого буфера «up 0».
// Без зависимостей от ESP-IDF и SWD: собирается и проверяется на хосте (tests/host).
#ifndef RTT_PARSE_H
#define RTT_PARSE_H

#include <stddef.h>
#include <stdint.h>

#define RTT_SIGNATURE_LEN 16      // "SEGGER RTT" + дополнение нулями
#define RTT_CB_HEADER_LEN 24      // ID[16] + MaxNumUp + MaxNumDown
#define RTT_UP_DESC_LEN 24        // sName, pBuffer, SizeOfBuffer, WrOff, RdOff, Flags
#define RTT_MAX_BUFFERS 16        // здравый предел числа буферов (ложное срабатывание подписи)
#define RTT_WROFF_OFFSET 12       // смещение WrOff внутри дескриптора
#define RTT_RDOFF_OFFSET 16       // смещение RdOff внутри дескриптора

typedef struct {
    uint32_t buf_addr;   // адрес кольца в памяти цели
    uint32_t size;       // размер кольца
    uint32_t wr_off;
    uint32_t rd_off;
} rtt_up_t;

// Ищет подпись на 4-байтовых границах. Возвращает смещение или -1.
int rtt_find_signature(const uint8_t *buf, size_t len);

// Разбор заголовка блока + дескриптора up 0 из len >= RTT_CB_HEADER_LEN+RTT_UP_DESC_LEN байт.
// 0 — успех; -1 — мало данных; -2 — нет подписи; -3 — число буферов вне 1..RTT_MAX_BUFFERS
// (ложная подпись); -4 — дескриптор вне здравого смысла (размер 0, смещения за кольцом).
int rtt_parse_cb(const uint8_t *cb, size_t len, rtt_up_t *out);

// Сколько байт ждёт чтения; -1 — смещения некорректны (wr/rd >= size или size == 0).
// Полный буфер у RTT — size-1 (писатель не догоняет RdOff), пустой — wr == rd.
int32_t rtt_ring_avail(uint32_t wr, uint32_t rd, uint32_t size);

typedef struct {
    uint32_t off1, len1;   // первый непрерывный отрезок (смещение от начала кольца)
    uint32_t len2;         // второй отрезок — с нуля, если читаемое обёрнуто через конец
    uint32_t new_rd;       // RdOff после прочтения
} rtt_plan_t;

// План чтения не более max_bytes (0 = без предела). 0 — успех (len1+len2 может быть 0), -1 — ошибка.
int rtt_ring_plan(uint32_t wr, uint32_t rd, uint32_t size, uint32_t max_bytes, rtt_plan_t *out);

// Сборщик строк: копит байты до '\n', на готовой строке зовёт emit (без '\n' и без '\r').
typedef void (*rtt_line_cb)(const char *line, size_t len, void *ctx);
typedef struct {
    char *buf;
    size_t cap;      // ёмкость буфера; при переполнении строка отдаётся как есть
    size_t len;
    rtt_line_cb emit;
    void *ctx;
} rtt_lines_t;

void rtt_lines_init(rtt_lines_t *l, char *buf, size_t cap, rtt_line_cb emit, void *ctx);
void rtt_lines_feed(rtt_lines_t *l, const uint8_t *data, size_t n);
// Отдать накопленное без '\n' (по таймауту, чтобы строка без перевода не застревала).
void rtt_lines_flush(rtt_lines_t *l);

#endif
