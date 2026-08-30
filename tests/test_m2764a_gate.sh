#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

dd if=/dev/zero bs=8192 count=1 status=none | tr '\000' '\377' >"$work/A.BIN"
cp "$work/A.BIN" "$work/B.BIN"
dd if=/dev/zero of="$work/CONTROL.BIN" bs=8192 count=1 status=none
printf '\252' | dd of="$work/CONTROL.BIN" bs=1 seek=123 conv=notrunc status=none

python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" \
    --blank-read "$work/A.BIN" --blank-read "$work/B.BIN" \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 6.01 --vpp 12.49 --ambient 24.0
grep -q $'^WILLEM-M2764A-WRITE-GATE-1\r$' "$work/M2764A.OK"
grep -q $'^VCC_STATUS=IN_SPEC\r$' "$work/M2764A.OK"
grep -q $'^HOST_BLANK_READS=2\r$' "$work/M2764A.OK"

if python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 6.01 --vpp 12.49 --ambient 24.0; then
    echo "missing host blank evidence unexpectedly accepted without opt-in" >&2
    exit 1
fi
[[ ! -e "$work/M2764A.OK" ]]

python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" --writer-blank-scan-only \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 6.01 --vpp 12.49 --ambient 24.0
grep -q $'^BLANK_CHECK=WRITER_VPP_OFF_FULL_SCAN_ONLY\r$' "$work/M2764A.OK"
grep -q $'^HOST_BLANK_READS=0\r$' "$work/M2764A.OK"

if python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" \
    --blank-read "$work/A.BIN" --blank-read "$work/B.BIN" \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 5.74 --vpp 12.66 --ambient 24.0; then
    echo "marginal VCC unexpectedly accepted without explicit opt-in" >&2
    exit 1
fi
[[ ! -e "$work/M2764A.OK" ]]

python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" \
    --blank-read "$work/A.BIN" --blank-read "$work/B.BIN" \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 5.74 --vpp 12.66 --ambient 24.0 --allow-marginal-vcc
grep -q $'^VCC=5.740\r$' "$work/M2764A.OK"
grep -q $'^VCC_STATUS=MARGINAL_EXPERIMENTAL_USER_ACCEPTED\r$' "$work/M2764A.OK"

printf '\000' | dd of="$work/B.BIN" bs=1 seek=0 conv=notrunc status=none
if python3 "$root/tools/create_m2764a_gate.py" \
    --output "$work/M2764A.OK" \
    --blank-read "$work/A.BIN" --blank-read "$work/B.BIN" \
    --control-read "$work/CONTROL.BIN" --chip-marking M2764AF1 \
    --vcc 6.01 --vpp 12.49 --ambient 24.0; then
    echo "mismatched blank evidence unexpectedly created gate" >&2
    exit 1
fi
[[ ! -e "$work/M2764A.OK" ]]
echo "M2764A physical-evidence gate tests passed"
