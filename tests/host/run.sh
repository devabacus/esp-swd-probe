#!/bin/sh
# Хост-тесты: sh tests/host/run.sh (кольцо пакетов DAP, серийный номер USB из MAC)
set -e
d=$(cd "$(dirname "$0")/../.." && pwd)
cc -std=c11 -Wall -Wextra -Werror -I"$d/main" "$d/tests/host/test_pktring.c" "$d/main/dap_pktring.c" -o "${TMPDIR:-/tmp}/test_pktring"
"${TMPDIR:-/tmp}/test_pktring"
cc -std=c11 -Wall -Wextra -Werror -I"$d/main" "$d/tests/host/test_usb_serial.c" "$d/main/usb_serial.c" -o "${TMPDIR:-/tmp}/test_usb_serial"
"${TMPDIR:-/tmp}/test_usb_serial"
