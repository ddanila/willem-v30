#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/.." && pwd)
program="$root/build/dos/WILLEM.COM"

if [[ ! -f "$program" ]]; then
    echo "missing $program; run make dos first" >&2
    exit 1
fi
command -v dosbox-x >/dev/null

work=$(mktemp -d /tmp/willem-dos-test.XXXXXX)
trap 'rm -rf "$work"' EXIT

run_dosbox() {
    local cpu=$1
    local drive="$work/$cpu"
    mkdir -p "$drive"
    cp "$program" "$drive/WILLEM.COM"
    dd if=/dev/zero of="$drive/ZERO.BIN" bs=8192 count=1 status=none
    printf 'WILLEM-WRITE-GATE-1\r\n' >"$drive/WRITE.OK"

    local args=(
        dosbox-x -fastlaunch
        -set "dosbox quit warning=false"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM R2764 R2764.BIN 378"
        -c "WILLEM R28C64 R28C64.BIN 378"
        -c "WILLEM V2764 ZERO.BIN 378"
        -c "WILLEM V28C64 ZERO.BIN 378"
        -c "WILLEM B2764 378"
        -c "WILLEM B28C64 378"
        -c "WILLEM W28C64 ZERO.BIN 378 /WRITE"
        -c "exit"
    )

    if [[ -n ${DISPLAY:-} ]]; then
        timeout 90s "${args[@]}" >"$drive/DOSBOX.OUT" 2>&1
    else
        timeout 90s xvfb-run -a "${args[@]}" >"$drive/DOSBOX.OUT" 2>&1
    fi

    # Remove the generated authorization outside DOS, then prove a fresh DOS
    # session refuses the same write command before it touches programmer I/O.
    unlink "$drive/WRITE.OK"
    local locked_args=(
        dosbox-x -fastlaunch
        -set "dosbox quit warning=false"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM W28C64 ZERO.BIN 378 /WRITE"
        -c "exit"
    )
    if [[ -n ${DISPLAY:-} ]]; then
        timeout 30s "${locked_args[@]}" >>"$drive/DOSBOX.OUT" 2>&1
    else
        timeout 30s xvfb-run -a "${locked_args[@]}" >>"$drive/DOSBOX.OUT" 2>&1
    fi

    [[ $(stat -c %s "$drive/R2764.BIN") == 8192 ]]
    [[ $(stat -c %s "$drive/R28C64.BIN") == 8192 ]]
    cmp "$drive/ZERO.BIN" "$drive/R2764.BIN"
    cmp "$drive/ZERO.BIN" "$drive/R28C64.BIN"
    [[ $(grep -c 'Read complete: bytes=8192' "$drive/WILLEM.LOG") == 2 ]]
    [[ $(grep -c 'VERIFY PASSED: all 8192 bytes match ZERO.BIN' "$drive/WILLEM.LOG") == 2 ]]
    [[ $(grep -c 'BLANK FAILED: mismatches=8192' "$drive/WILLEM.LOG") == 2 ]]
    [[ $(grep -c 'Safe shutdown complete: VCC off, VPP off' "$drive/WILLEM.LOG") == 7 ]]
    [[ $(grep -c 'physical read gate is locked' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'WRITE PASSED: programmed=0 unchanged=8192 verified=8192' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'DIP ON.*\[X\]\[X\]\[ \]\[X\]\[ \]\[X\]\[ \]\[ \]\[X\]' "$drive/WILLEM.LOG") == 3 ]]
    [[ $(grep -c 'DIP ON.*\[ \]\[ \]\[ \]\[X\]\[ \]\[X\]\[ \]\[ \]\[X\]' "$drive/WILLEM.LOG") == 4 ]]
    echo "DOSBox-X $cpu diagnostic matrix passed"
}

# DOSBox-X has no V30 core. 8086 proves the instruction baseline; 80186 is a
# useful faster-compatible execution check. Hardware-clocked PIT waits make
# signal timing independent of either emulated CPU's instruction throughput.
run_dosbox 8086
run_dosbox 80186
