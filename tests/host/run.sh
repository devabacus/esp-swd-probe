#!/bin/sh
# Host tests (no ESP-IDF): gcc + pytest-free python unittest.
set -e
cd "$(dirname "$0")/../.."
mkdir -p /tmp/swdprobe-host
cc -Wall -Wextra -Werror -Icomponents/DAP/include tests/host/test_vtref_calc.c \
   components/DAP/source/vtref_calc.c -o /tmp/swdprobe-host/test_vtref_calc
/tmp/swdprobe-host/test_vtref_calc
python3 -I -m unittest discover -s tests/host -p 'test_*.py' -t tests/host
