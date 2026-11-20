#!/bin/sh
# Хост-тест чистой части RTT (без ESP-IDF): sh tests/host/run.sh
set -e
d=$(cd "$(dirname "$0")" && pwd)
cc -Wall -Wextra -Werror -o "${TMPDIR:-/tmp}/test_rtt_parse" "$d/test_rtt_parse.c" "$d/../../main/rtt_parse.c"
"${TMPDIR:-/tmp}/test_rtt_parse"
