#!/usr/bin/env python3
"""Напряжение цели (VTref) с esp-swd-probe: мВ и вывод «3,3 В напрямую / 1,8 В через сдвиг / нет питания».

Прошивка отвечает на vendor-команду CMSIS-DAP 0x80: ответ `80 <мВ lo> <мВ hi> <класс>`
(класс: 0 нет питания, 1 1,8 В, 2 3,3 В, 3 выше предела). Пороги — настройки прошивки
(Kconfig DAP_VTREF_*), здесь их нет: класс приходит готовым.

Транспорт — OpenOCD (`cmsis-dap cmd`), pyusb не нужен:
    python tools/vtref.py                       # openocd из PATH, VID:PID 303a:4001
    python tools/vtref.py --vid-pid 303a:4001 --openocd /path/to/openocd
    python tools/vtref.py --raw-response "80 e6 0c 02"   # без железа: разобрать ответ
Код выхода: 0 — измерено (3 «выше предела» тоже 0), 1 — прошивка не отвечает/не поддерживает, 2 — нет OpenOCD.
"""
import argparse
import re
import shutil
import subprocess
import sys

VENDOR_ID = 0x80
VERDICTS = {
    0: "нет питания",
    1: "1,8 В — только через сдвиг уровня",
    2: "3,3 В — напрямую",
    3: "выше 3,6 В — не подключать напрямую",
}


class NotSupported(Exception):
    pass


def parse_response(data: bytes):
    """ответ vendor-команды -> (мВ, класс)"""
    if len(data) < 4 or data[0] != VENDOR_ID:
        raise NotSupported("ответ не похож на VTref (старая прошивка без команды 0x80?): " + data.hex(" "))
    return data[1] | (data[2] << 8), data[3]


def extract_response(text: str) -> bytes:
    """найти в выводе OpenOCD строку из одних hex-байтов, начинающуюся с 80"""
    for line in reversed(text.splitlines()):
        line = line.strip()
        if re.fullmatch(r"(?:[0-9a-fA-F]{2}\s*)+", line) and line.lower().startswith("80"):
            return bytes.fromhex(re.sub(r"\s+", "", line))
    raise NotSupported("в выводе OpenOCD нет ответа команды 0x80:\n" + text.strip()[-400:])


def verdict(cls: int) -> str:
    return VERDICTS.get(cls, "неизвестный класс %d" % cls)


def format_line(mv: int, cls: int) -> str:
    volts = ("%.2f" % (mv / 1000)).replace(".", ",")
    return "%d мВ (%s В): %s" % (mv, volts, verdict(cls))


def query_openocd(openocd: str, vid: int, pid: int, timeout: float) -> str:
    cmd = [openocd,
           "-c", "adapter driver cmsis-dap",
           "-c", "cmsis-dap vid_pid 0x%04x 0x%04x" % (vid, pid),
           "-c", "transport select swd",
           "-c", "init",
           "-c", "cmsis-dap cmd 80",
           "-c", "shutdown"]
    p = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    # OpenOCD пишет в stderr логи, результат команды — в stdout/stderr: берём всё
    return p.stdout + "\n" + p.stderr


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--openocd", default="openocd")
    ap.add_argument("--vid-pid", default="303a:4001", help="hex, как в Kconfig DAP_USB_VID/PID (303a:4001)")
    ap.add_argument("--timeout", type=float, default=15.0)
    ap.add_argument("--raw-response", help="готовый ответ в hex (без железа), например '80 e6 0c 02'")
    a = ap.parse_args(argv)
    try:
        if a.raw_response is not None:
            data = bytes.fromhex(a.raw_response.replace(" ", ""))
        else:
            if shutil.which(a.openocd) is None:
                print("нет OpenOCD (%s): поставьте или укажите --openocd" % a.openocd, file=sys.stderr)
                return 2
            vid, pid = (int(x, 16) for x in a.vid_pid.split(":"))
            data = extract_response(query_openocd(a.openocd, vid, pid, a.timeout))
        print(format_line(*parse_response(data)))
        return 0
    except NotSupported as e:
        print("VTref недоступен:", e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
