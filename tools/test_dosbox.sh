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
cleanup() {
    local status=$?
    if ((status == 0)); then
        rm -rf "$work"
    else
        echo "DOSBox-X failure evidence retained in $work" >&2
    fi
}
trap cleanup EXIT

run_dosbox() {
    local cpu=$1
    local drive="$work/$cpu"
    mkdir -p "$drive"
    cp "$program" "$drive/WILLEM.COM"
    dd if=/dev/zero of="$drive/ZERO.BIN" bs=8192 count=1 status=none
    printf 'WILLEM-WRITE-GATE-1\r\n' >"$drive/WRITE.OK"
    printf '\r\n\r\n\r\n\r\n' >"$drive/ENTERS.TXT"

    local args=(
        dosbox-x -silent -fastlaunch
        -set "dosbox quit warning=false"
        -set "sdl output=surface"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM R2764 R2764.BIN 378"
        -c "WILLEM R2764 PROFILE.BIN 378 /PROFILE:conservative"
        -c "WILLEM R28C64 R28C64.BIN 378 /PROFILE:powerfast"
        -c "WILLEM V2764 ZERO.BIN 378"
        -c "WILLEM V28C64 ZERO.BIN 378"
        -c "WILLEM B2764 378"
        -c "WILLEM B28C64 378"
        -c "WILLEM W28C64 ZERO.BIN 378 /WRITE"
        -c "WILLEM D28C64 378 /TRACE < ENTERS.TXT"
        -c "exit"
    )

    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 90s "${args[@]}" >"$drive/DOSBOX.OUT" 2>&1

    # Remove the generated authorization outside DOS, then prove a fresh DOS
    # session refuses the same write command before it touches programmer I/O.
    unlink "$drive/WRITE.OK"
    local locked_args=(
        dosbox-x -silent -fastlaunch
        -set "dosbox quit warning=false"
        -set "sdl output=surface"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM R2764 BADPROF.BIN 378 /PROFILE:unknown"
        -c "WILLEM W28C64 ZERO.BIN 378 /WRITE"
        -c "exit"
    )
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 30s "${locked_args[@]}" >>"$drive/DOSBOX.OUT" 2>&1

    [[ $(stat -c %s "$drive/R2764.BIN") == 8192 ]]
    [[ $(stat -c %s "$drive/PROFILE.BIN") == 8192 ]]
    [[ ! -e "$drive/BADPROF.BIN" ]]
    [[ $(stat -c %s "$drive/R28C64.BIN") == 8192 ]]
    cmp "$drive/ZERO.BIN" "$drive/R2764.BIN"
    cmp "$drive/ZERO.BIN" "$drive/PROFILE.BIN"
    cmp "$drive/ZERO.BIN" "$drive/R28C64.BIN"
    [[ $(grep -c 'Read complete: bytes=8192' "$drive/WILLEM.LOG") == 3 ]]
    [[ $(grep -c 'DOSRAVI_PROFILE name=conservative address_setup_us=4 oe_settle_us=4 input_latch_us=4 input_clock_us=4 power_on_ms=5 build_id=dosravi-profiles-v2' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_METRIC read_ms=[1-9][0-9]* profile=conservative' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'DOSRAVI_PROFILE name=powerfast address_setup_us=1 oe_settle_us=1 input_latch_us=1 input_clock_us=1 power_on_ms=150 build_id=dosravi-profiles-v2' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_METRIC read_ms=[1-9][0-9]* profile=powerfast' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'ERROR: unknown read profile <unknown>' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'VERIFY PASSED: all 8192 bytes match ZERO.BIN' "$drive/WILLEM.LOG") == 2 ]]
    [[ $(grep -c 'BLANK FAILED: mismatches=8192' "$drive/WILLEM.LOG") == 2 ]]
    [[ $(grep -c 'Safe shutdown complete: VCC off, VPP off' "$drive/WILLEM.LOG") == 9 ]]
    [[ $(grep -c 'physical read gate is locked' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'WRITE PASSED: programmed=0 unchanged=8192 verified=8192' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_WRITE_METRIC program_ms=[0-9]+ verify_ms=[0-9]+ changed=0 unchanged=8192 retry_bytes=0 retries=0 late=0 image_crc32=[0-9A-Fa-f]{8} build_id=dosravi-profiles-v2' "$drive/WILLEM.LOG") == 1 ]]
    [[ $(grep -c 'DIP ON.*\[X\]\[X\]\[ \]\[X\]\[ \]\[X\]\[ \]\[ \]\[X\]' "$drive/WILLEM.LOG") == 9 ]]
    [[ $(grep -c 'Leave TWO complete rows empty at lever end' "$drive/WILLEM.LOG") == 9 ]]
    [[ $(grep -c 'Diagnostic complete; power transition: safe shutdown begins' "$drive/WILLEM.LOG") == 1 ]]
    [[ -s "$drive/WTRACE.BIN" ]]
    echo "DOSBox-X $cpu diagnostic matrix passed"
}

# DOSBox-X has no V30 core. 8086 proves the instruction baseline; 80186 is a
# useful faster-compatible execution check. Hardware-clocked PIT waits make
# signal timing independent of either emulated CPU's instruction throughput.
run_dosbox 8086
run_dosbox 80186
