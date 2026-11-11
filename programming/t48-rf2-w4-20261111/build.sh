#!/usr/bin/env bash
set -euo pipefail
# Pass a checkout of whaslbeck/minipro at cfe2171ca1b23399558b8d2a603a2ac3fa3faf5d.
# Apply minipro.patch there first; libusb development headers are required.
reader_source=$(realpath "${1:?minipro source checkout required}")
record_dir=$(cd -- "$(dirname -- "$0")" && pwd)
make -C "$reader_source" -j4 "SHARE_INSTDIR=$reader_source"
objects=()
for object in "$reader_source"/src/*.o; do
    [[ $(basename "$object") == main.o ]] || objects+=("$object")
done
gcc -g -Wall -Wextra -Werror -I"$reader_source/src" \
    "$record_dir/rf2-direct.c" "${objects[@]}" -l:libusb-1.0.so.0 -lz \
    -o "$reader_source/rf2-direct"
gcc -Wall -Wextra -Werror -I"$reader_source/src" \
    "$record_dir/rf2-direct.c" "$record_dir/mock-t48.c" -o "$reader_source/rf2-mock"
