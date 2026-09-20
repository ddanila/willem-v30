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
    dd if=/dev/zero of="$drive/RF5ZERO.BIN" bs=2048 count=1 status=none
    dd if=/dev/zero of="$drive/256ZERO.BIN" bs=32768 count=1 status=none
    dd if=/dev/zero of="$drive/512ZERO.BIN" bs=65536 count=1 status=none
    printf 'WILLEM-WRITE-GATE-1\r\n' >"$drive/WRITE.OK"
    printf 'WILLEM-M2764A-WRITE-GATE-1\r\n' >"$drive/M2764A.OK"
    printf '\r\n\r\n\r\n\r\n' >"$drive/ENTERS.TXT"
    printf '%s\r\n' \
        'WILLEM RRF5 RF5.BIN 378 /PROFILE:conservative' \
        'WILLEM R27512 R27512.BIN 378' \
        'WILLEM R27256 R27256.BIN 378' \
        'WILLEM R2764 R2764.BIN 378' \
        'WILLEM R2764 PROFILE.BIN 378 /PROFILE:conservative' \
        'WILLEM R28C64 R28C64.BIN 378 /PROFILE:powerfast' \
        'WILLEM V2764 ZERO.BIN 378' \
        'WILLEM V28C64 ZERO.BIN 378' \
        'WILLEM B2764 378' \
        'WILLEM B28C64 378' \
        'WILLEM W28C64 ZERO.BIN 378 /WRITE' \
        'WILLEM WM2764A ZERO.BIN 378 /WRITE' \
        'WILLEM D28C64 378 /TRACE < ENTERS.TXT' >"$drive/RUNTEST.BAT"

    local args=(
        dosbox-x -silent -fastlaunch
        -set "dosbox quit warning=false"
        -set "sdl output=surface"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "RUNTEST.BAT"
        -c "exit"
    )

    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 90s "${args[@]}" >"$drive/DOSBOX.OUT" 2>&1

    # Remove the generated authorization outside DOS, then prove a fresh DOS
    # session refuses the same write command before it touches programmer I/O.
    unlink "$drive/WRITE.OK"
    unlink "$drive/M2764A.OK"
    local locked_args=(
        dosbox-x -silent -fastlaunch
        -set "dosbox quit warning=false"
        -set "sdl output=surface"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM WRF5 ZERO.BIN 378 /WRITE"
        -c "WILLEM R2764 BADPROF.BIN 378 /PROFILE:unknown"
        -c "WILLEM W28C64 ZERO.BIN 378 /WRITE"
        -c "WILLEM WM2764A ZERO.BIN 378 /WRITE"
        -c "WILLEM DM2764A 378"
        -c "exit"
    )
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 30s "${locked_args[@]}" >>"$drive/DOSBOX.OUT" 2>&1

    [[ $(stat -c %s "$drive/RF5.BIN") == 2048 ]]
    [[ $(stat -c %s "$drive/R2764.BIN") == 8192 ]]
    [[ $(stat -c %s "$drive/PROFILE.BIN") == 8192 ]]
    [[ ! -e "$drive/BADPROF.BIN" ]]
    [[ $(stat -c %s "$drive/R28C64.BIN") == 8192 ]]
    cmp "$drive/RF5ZERO.BIN" "$drive/RF5.BIN"
    cmp "$drive/512ZERO.BIN" "$drive/R27512.BIN"
    cmp "$drive/256ZERO.BIN" "$drive/R27256.BIN"
    cmp "$drive/ZERO.BIN" "$drive/R2764.BIN"
    cmp "$drive/ZERO.BIN" "$drive/PROFILE.BIN"
    cmp "$drive/ZERO.BIN" "$drive/R28C64.BIN"
    cat "$drive"/WILL[0-9][0-9][0-9][0-9].LOG \
        "$drive/WILLEM.LOG" >"$drive/ALLLOG.TXT"
    [[ $(grep -ic 'Read complete: bytes=65536 CRC16-CCITT=1d0f' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -ic 'Required DIP mask=1d4h' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'Read complete: bytes=8192' "$drive/ALLLOG.TXT") == 3 ]]
    [[ $(grep -ic 'Read complete: bytes=32768 CRC16-CCITT=e1f0' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -ic 'Required DIP mask=1b3h' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'Read complete: bytes=2048' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'DOSRAVI_PROFILE name=conservative address_setup_us=4 oe_settle_us=4 input_latch_us=4 input_clock_us=4 power_on_ms=5 build_id=dosravi-27512-read-v1' "$drive/ALLLOG.TXT") == 2 ]]
    [[ $(grep -Ec 'DOSRAVI_METRIC read_ms=[1-9][0-9]* profile=conservative' "$drive/ALLLOG.TXT") == 2 ]]
    [[ $(grep -c 'DOSRAVI_PROFILE name=powerfast address_setup_us=1 oe_settle_us=1 input_latch_us=1 input_clock_us=1 power_on_ms=150 build_id=dosravi-27512-read-v1' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_METRIC read_ms=[1-9][0-9]* profile=powerfast' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'ERROR: unknown read profile <unknown>' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'ERROR: invalid command line' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'VERIFY PASSED: all 8192 bytes match ZERO.BIN' "$drive/ALLLOG.TXT") == 2 ]]
    [[ $(grep -c 'BLANK FAILED: mismatches=8192' "$drive/ALLLOG.TXT") == 2 ]]
    [[ $(grep -c 'Safe shutdown complete: VCC off, VPP off' "$drive/ALLLOG.TXT") == 13 ]]
    [[ $(grep -c 'Safe shutdown complete after blank check: VCC off, VPP off' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'physical read gate is locked' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c '12.5V write gate is locked' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'DM2764A requires explicit /VPP confirmation and an empty socket' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'WRITE PASSED: programmed=0 unchanged=8192 verified=8192' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_WRITE_METRIC program_ms=[0-9]+ verify_ms=[0-9]+ changed=0 unchanged=8192 retry_bytes=0 retries=0 late=0 image_crc32=[0-9A-Fa-f]{8} build_id=dosravi-27512-read-v1' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -Ec 'DOSRAVI_M2764A_METRIC program_ms=0 high_voltage_verify_ms=0 programmed=0 ff_skipped=0 initial_pulses=0 retry_bytes=0 retries=0 overprogram_pulses=0 max_initial=0 blank_mismatches=8192 image_crc32=[0-9A-Fa-f]{8} final_5v_verify_required=1 build_id=dosravi-27512-read-v1' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'DIP ON.*\[X\]\[X\]\[ \]\[X\]\[ \]\[X\]\[ \]\[ \]\[X\]' "$drive/ALLLOG.TXT") == 10 ]]
    [[ $(grep -c 'DIP ON.*\[X\]\[X\]\[ \]\[ \]\[ \]\[X\]\[ \]\[X\]\[X\]' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'Leave TWO complete rows empty at lever end' "$drive/ALLLOG.TXT") == 12 ]]
    [[ $(grep -c 'Leave FOUR rows empty at lever end' "$drive/ALLLOG.TXT") == 1 ]]
    [[ $(grep -c 'Diagnostic complete; power transition: safe shutdown begins' "$drive/ALLLOG.TXT") == 1 ]]
    [[ -s "$drive/WTRACE.BIN" ]]

    # A Pocket8086 session exposed the 16-bit append boundary after the text
    # log grew beyond 32 KiB. Prove that a large log is preserved and a fresh
    # current-run log is usable instead of silently losing evidence.
    dd if=/dev/zero of="$drive/WILLEM.LOG" bs=36000 count=1 status=none
    local rotate_args=(
        dosbox-x -silent -fastlaunch
        -set "dosbox quit warning=false"
        -set "sdl output=surface"
        -set "cpu cputype=$cpu"
        -set "cpu cycles=max"
        -set "midi mididevice=none"
        -c "mount c $drive"
        -c "c:"
        -c "WILLEM INVALID"
        -c "exit"
    )
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
        timeout 30s "${rotate_args[@]}" >>"$drive/DOSBOX.OUT" 2>&1
    find "$drive" -name 'WILL????.LOG' -size 36000c | grep -q .
    grep -q 'ERROR: invalid command line' "$drive/WILLEM.LOG"
    echo "DOSBox-X $cpu diagnostic matrix passed"
}

# DOSBox-X has no V30 core. 8086 proves the instruction baseline; 80186 is a
# useful faster-compatible execution check. Hardware-clocked PIT waits make
# signal timing independent of either emulated CPU's instruction throughput.
run_dosbox 8086
run_dosbox 80186
