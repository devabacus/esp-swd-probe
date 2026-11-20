#!/usr/bin/env python3
"""Сторож порта ESP32-S3 без железа: разбирает исходники и отказывает на трёх найденных на стенде 20.11 ошибках.

(а) DAP_SPI_Deinit под S3 не включает тактирование выделенного GPIO (clk_en_dedicated_gpio) — ноги SWD не двигаются;
(б) PIN_SWDIO_OUT_ENABLE/DISABLE под S3 пусты — направление SWDIO не переключается;
(в) DAP_PACKET_COUNT > 1 при чтении vendor-FIFO потоком (tud_vendor_read) — запросы склеиваются
    («CMSIS-DAP command mismatch»). Разбиение по передачам отмечается в main/usb_dap.c маркером
    S3_PORT_PER_TRANSFER_READ.

Запуск: python tools/check_s3_port.py [корень_репозитория]; код 0 зелено, 1 нарушение, 2 нет исходников.
"""
import re
import sys
from pathlib import Path

S3_COND = re.compile(r"^\s*#\s*(?:el)?if\b.*CONFIG_IDF_TARGET_ESP32S3")
ANY_COND = re.compile(r"^\s*#\s*(?:elif|else|endif)\b")
CLOCK_RE = re.compile(r"_dedic_gpio_ll_enable_bus_clock\s*\(\s*true\s*\)|clk_en_dedicated_gpio\s*=\s*1")


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//[^\n]*", "", text)


def function_body(text, header_re, after=None):
    """Тело первой функции, чей заголовок подходит под header_re (не раньше позиции after)."""
    m = header_re.search(text, after or 0)
    if not m:
        return None, -1
    start = text.index("{", m.end())
    depth = 0
    for i in range(start, len(text)):
        depth += (text[i] == "{") - (text[i] == "}")
        if depth == 0:
            return text[start + 1:i], m.start()
    return None, -1


def s3_branch(body):
    """Строки ветки `#elif defined CONFIG_IDF_TARGET_ESP32S3` до следующей директивы."""
    out, on = [], False
    for line in body.splitlines():
        if S3_COND.match(line):
            on = True
        elif on and ANY_COND.match(line):
            on = False
        elif on:
            out.append(line)
    return "\n".join(out)


def check_clock(root):
    text = (root / "components/DAP/source/spi_switch.c").read_text(encoding="utf-8")
    pos = 0
    for m in re.finditer(r"DAP_SPI_Deinit\s*\(\s*\)\s*\n?\s*\{", text):
        before = text[:m.start()].splitlines()
        cond = next((ln for ln in reversed(before) if re.match(r"\s*#\s*(?:el)?if", ln)), "")
        if "ESP32S3" in cond:
            body, _ = function_body(text, re.compile(r"DAP_SPI_Deinit\s*\(\s*\)"), m.start() - 1)
            if body and CLOCK_RE.search(strip_comments(body)):
                return []
            return ["(а) DAP_SPI_Deinit под S3 не включает тактирование выделенного GPIO "
                    "(_dedic_gpio_ll_enable_bus_clock(true) / clk_en_dedicated_gpio=1): ноги SWD не двигаются"]
    return ["(а) в spi_switch.c нет DAP_SPI_Deinit под CONFIG_IDF_TARGET_ESP32S3"]


def check_oen(root):
    text = (root / "components/DAP/config/DAP_config.h").read_text(encoding="utf-8")
    errs = []
    for name in ("PIN_SWDIO_OUT_ENABLE", "PIN_SWDIO_OUT_DISABLE"):
        body, _ = function_body(text, re.compile(r"void\s+" + name + r"\s*\(\s*(?:void)?\s*\)"))
        if body is None:
            errs.append(f"(б) {name} не найдена в DAP_config.h")
            continue
        code = strip_comments(s3_branch(body))
        if not re.sub(r"\s+", "", code):
            errs.append(f"(б) {name} под S3 пуста: направление SWDIO не переключается")
    return errs


def s3_packet_count(text):
    """Действующее DAP_PACKET_COUNT для S3: define внутри ветки S3, иначе общий."""
    lines = strip_comments(text).splitlines()
    in_s3 = False
    general = None
    for ln in lines:
        if S3_COND.match(ln):
            in_s3 = True
            continue
        if ANY_COND.match(ln):
            in_s3 = False
            continue
        m = re.match(r"\s*#\s*define\s+DAP_PACKET_COUNT\s+(\d+)", ln)
        if m:
            if in_s3:
                return int(m.group(1))
            general = general if general is not None else int(m.group(1))
    return general


def check_packets(root):
    cfg = (root / "components/DAP/config/DAP_config.h").read_text(encoding="utf-8")
    usb = (root / "main/usb_dap.c").read_text(encoding="utf-8")
    n = s3_packet_count(cfg)
    if n is None:
        return ["(в) DAP_PACKET_COUNT не найден в DAP_config.h"]
    stream = "tud_vendor_read" in strip_comments(usb) and "S3_PORT_PER_TRANSFER_READ" not in usb
    if n > 1 and stream:
        return [f"(в) DAP_PACKET_COUNT={n} для S3 при потоковом tud_vendor_read в main/usb_dap.c: "
                "запросы склеиваются («command mismatch»). Нужно DAP_PACKET_COUNT 1 или чтение по границам передач "
                "(маркер S3_PORT_PER_TRANSFER_READ)"]
    return []


def run(root):
    root = Path(root)
    try:
        return check_clock(root) + check_oen(root) + check_packets(root)
    except FileNotFoundError as e:
        print(f"нет исходника: {e.filename}")
        return None


def main(argv):
    root = argv[1] if len(argv) > 1 else Path(__file__).resolve().parent.parent
    errs = run(root)
    if errs is None:
        return 2
    for e in errs:
        print("ОТКАЗ:", e)
    print("check_s3_port: " + ("КРАСНЫЙ" if errs else "зелёный"))
    return 1 if errs else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
