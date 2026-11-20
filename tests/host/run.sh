#!/bin/sh
# Хост-тест кольца пакетов DAP: sh tests/host/run.sh
set -e
d=$(cd "$(dirname "$0")/../.." && pwd)
cc -std=c11 -Wall -Wextra -Werror -I"$d/main" "$d/tests/host/test_pktring.c" "$d/main/dap_pktring.c" -o "${TMPDIR:-/tmp}/test_pktring"
"${TMPDIR:-/tmp}/test_pktring"
