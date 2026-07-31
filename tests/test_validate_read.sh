#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d /tmp/willem-read-check.XXXXXX)
trap 'rm -rf "$work"' EXIT

# A deterministic nonuniform 8 KiB image.
python3 - "$work/EXPECTED.BIN" <<'PY'
import pathlib
import sys
pathlib.Path(sys.argv[1]).write_bytes(bytes(((i * 73) ^ (i >> 3) ^ 0xA5) & 0xFF for i in range(8192)))
PY
cp "$work/EXPECTED.BIN" "$work/READ1.BIN"
cp "$work/EXPECTED.BIN" "$work/READ2.BIN"
python3 "$root/tools/validate_read.py" \
    "$work/EXPECTED.BIN" "$work/READ1.BIN" "$work/READ2.BIN" >/dev/null

# A repeatable but stuck bus must not pass merely because both reads agree.
dd if=/dev/zero of="$work/STUCK.BIN" bs=8192 count=1 status=none
if python3 "$root/tools/validate_read.py" \
    "$work/EXPECTED.BIN" "$work/STUCK.BIN" "$work/STUCK.BIN" >/dev/null; then
    echo "stuck image incorrectly passed" >&2
    exit 1
fi

# One changed byte must be diagnosed as a mismatch.
cp "$work/EXPECTED.BIN" "$work/BAD.BIN"
printf '\000' | dd of="$work/BAD.BIN" bs=1 seek=123 conv=notrunc status=none
if python3 "$root/tools/validate_read.py" \
    "$work/EXPECTED.BIN" "$work/READ1.BIN" "$work/BAD.BIN" >/dev/null; then
    echo "mismatched image incorrectly passed" >&2
    exit 1
fi

echo "physical-read acceptance tests passed"
