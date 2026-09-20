# Willem V30

Current development version: `0.2.0-dev`

Small, auditable MS-DOS tools for classic LPT-connected Willem EPROM
programmers, targeting the NEC V20/V30 and 8086 instruction set.
The DOS program is a headerless `.COM` built in the tiny memory model, keeping
code, data, heap, and stack within one 64 KiB segment.

The initial hardware target is a Willem PCB5.0E operating in its PCB3B
compatibility mode. The initial device targets are:

- К573РФ5/2716: dedicated 2 KiB read-only path
- 2764/27C64: read, blank-check, and verify
- ST M2764A: gated two-stage Fast Programming and verification
- AT28C64: read, blank-check, program, and verify

Physical ZIF placement, compatibility selection, power precautions, and the
first-read gate are documented in [`docs/hardware.md`](docs/hardware.md).

## Development strategy

The classic Willem port protocol will be derived primarily from the GPL-2.0
[Geepro](https://github.com/danielg4/geepro) driver and checked against traces
from the original DOS Willem software. A virtual programmer with a known 8 KiB
ROM image will be used before testing real hardware.

Real-hardware work is staged conservatively:

1. Validate read-only operation using a known Juku 2764.
2. Validate address and data integrity against the expected ROM image.
3. Enable programming only after read operation and device-specific rails are
   proven.

The final DOS distribution will use 8.3 filenames and CRLF text files.

Every real-hardware run writes a human-readable `WILLEM.LOG` while also showing
the same operational messages on screen. A compact full LPT trace can be
enabled for hardware diagnosis and emulator comparison; see
[`docs/logging.md`](docs/logging.md).
Logs at or above 24 KiB are archived under numbered DOS 8.3 names before the
next run, preventing the 16-bit append failure observed on the Pocket8086.
The exact Geepro revision, function mapping, read sequence, and an upstream
DIP-table discrepancy are recorded in
[`docs/geepro-audit.md`](docs/geepro-audit.md).

## DOS commands

The build supports read, blank-check, and verify directly. AT28C64 writing is
present but remains locked until the host validator creates `WRITE.OK` from two
matching known-Juku physical reads.

```text
WILLEM RRF5   OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R2764  OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R27256 OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R27512 OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]
WILLEM W28C64 ROM.BIN [378] /WRITE
WILLEM DM2764A        [378] /VPP
WILLEM WM2764A ROM.BIN [378] /WRITE
```

The optional LPT base is hexadecimal and defaults to `378`. Every operation
prints and logs a visual 12-switch DIP diagram. Geepro's mask maps bit 0 to
physical switch 1; follow the numbering and the `ON` mark printed on the DIP
bank. `RRF5` always creates exactly 2048 bytes and has no corresponding
blank, verify, or write command. Verification images for the other devices
must be exactly 8192 bytes.

`R27256` reads exactly 32768 bytes from a 27256/27C256 using the EPROM
read sequence with VPP off. Use Normal 28-pin routing, DIP mask `1B3h`
(ON: 1,2,5,6,8,9), and bottom alignment with two empty rows at the lever.
At conservative timing, allow approximately eight minutes per read; use a
DOSRAVI execution timeout of at least 900 seconds. Build `dosravi-27256-ce-fix-v2` corrects the original R27256 CE polarity
error (27256 requires pin17 low, unlike 2764). Earlier captures showed output
echo; the corrected reader eliminated it on the same second chip, yielding
all FF. Other corrected captures included an FF/F7-only chip. A
known-programmed 27256 control and repeat verification remain pending.

Read commands accept audited runtime timing tables through `/PROFILE:name`.
The ordered experimental sequence is `conservative,address2,oe2,latch2,`
`balanced,address1,oe1,latch1,fast,powerfast`; each adjacent table changes
exactly one dimension. Omitting the option selects `legacy`, which preserves the original
read delays. Exact parameters, build ID, and the local PIT-driven elapsed time
are written as `DOSRAVI_PROFILE` and `DOSRAVI_METRIC` records. These profiles
are experimental until physical ten-read acceptance establishes a guarded
baseline; see [`docs/timing-profiles.md`](docs/timing-profiles.md).

`W28C64` additionally requires an explicit `/WRITE` and a valid `WRITE.OK` in
the current DOS directory. The normal distribution intentionally contains no
token. A successful `validate_read.py --unlock WRITE.OK ...` run creates it;
copy it to the DOS disk only after reviewing the reported identities. The
writer keeps VPP off, skips already matching bytes, uses the manufacturer's
SDP protected-write sequence, and performs a complete post-write verification.
`/TRACE` is rejected during writes to preserve the 150 us command timing.
Interrupts are masked only across each four-load SDP burst. Each byte receives
at most three total attempts, with retry and late-completion counts logged so
marginal used EEPROMs remain visible rather than being silently accepted.
The final log also contains a machine-readable `DOSRAVI_WRITE_METRIC` with
separate PIT-driven programming and full-verification times, byte/retry counts,
image CRC-32, and build ID.

`WM2764A` is intentionally specific to ST's `M2764A` (including `M2764AF1`),
not a generic 2764 command. It requires a separate `M2764A.OK` whose first line
is `WILLEM-M2764A-WRITE-GATE-1`, refuses any nonblank byte before enabling
VPP, then follows ST's Fast Programming Algorithm at externally selected and
measured 6.0 V VCC and 12.5 V VPP: up to 25 PIT-timed 1 ms pulses with byte
verify, followed by one `3*n` ms overprogram pulse. VCC is applied before VPP;
shutdown removes VPP before VCC. `/TRACE` is forbidden.

The board cannot change from programming VCC to read VCC in software, so a
successful command is only a **program phase**, never final success. It powers
down and records `final_5v_verify_required=1`. Remove programmer power, set
normal 5 V VCC/VPP=VCC read configuration, repower, then run `V2764` against
the same image. `DM2764A /VPP`, with an empty socket, provides the mandatory
meter pauses before the first physical attempt. See
[`docs/hardware.md`](docs/hardware.md).

The reference PCB5.0E completed this gate and four physical M2764AF1 writes on
2026-08-26. All four later passed independent normal-rail verification; the
exact measurements, image identities, pulse counts, and interrupted-run safety
observations are recorded in
[`docs/m2764a-physical-acceptance.md`](docs/m2764a-physical-acceptance.md).

After two independent blank reads, a nonuniform control-chip read, and the
empty-socket meter test, create the reviewed token with:

```sh
python3 tools/create_m2764a_gate.py --output M2764A.OK \
  --blank-read BLANK1.BIN --blank-read BLANK2.BIN \
  --control-read CONTROL.BIN --chip-marking M2764AF1 \
  --vcc 6.00 --vpp 12.50 --ambient 25
```

The normal gate rejects programming VCC below 5.75 V. A deliberate first-chip
experiment measured narrowly below that limit may use `--allow-marginal-vcc`
only for 5.70..<5.75 V. The exact value and
`VCC_STATUS=MARGINAL_EXPERIMENTAL_USER_ACCEPTED` are then preserved in the
token; this does not make the rail data-sheet compliant.

For a routine follow-up attempt, `--writer-blank-scan-only` explicitly omits
the separate host reads. `WM2764A` still scans all 8192 bytes with VPP off and
aborts before enabling programming voltage on any non-`FF` byte. The gate
records `HOST_BLANK_READS=0`; do not use this option when independent archival
blank-read evidence is required.

## Build and test

```sh
make test       # portable virtual Willem with known 8 KiB contents
make dos        # build the 8086 tiny-model WILLEM.COM
make dos-test   # run all six diagnostic paths as 8086 and 80186
make dist       # create the validated DOS directory in build/dist
```

`make test` also exercises append-only trace decoding, interrupted-run
recovery, corruption rejection, and the physical-read acceptance gate.

The distribution builder rejects non-8.3 filenames and verifies that every
text file contains CRLF rather than bare LF line endings. `HWSETUP.TXT` keeps
dangerous physical-board details prominently gated. The recorded reference
PCB5.0E setup is physically verified, but another clone still requires its own
selector identity and empty-socket rail measurements before M2764A writing.

After two physical reads, enforce the hardware gate on the modern host with:

```sh
python3 tools/validate_read.py --unlock WRITE.OK \
    EXPECTED.BIN READ1.BIN READ2.BIN
```

The tool rejects wrong sizes, non-repeatable reads, disagreement with the known
image, and repeatable all-zero/all-FF stuck-bus captures.

### 27512 / 27C512 reads

`WILLEM R27512 BIOS.BIN 378 /PROFILE:conservative` reads all 64 KiB using
DIP `1D4h` (ON 3,5,7,8,9), Normal 28-pin routing, 5 V VCC, and VPP off.
See `docs/hardware.md` for placement. Build `dosravi-27512-read-v1` streams
8 KiB chunks with a 32-bit offset and a continuous CRC16, avoiding the DOS
64 KiB segment limit. Use DOSRAVI EXEC timeout 1800 seconds; a conservative
read takes approximately 15 minutes. Only the read command is implemented
for this device.

Physical 27512 validation: one 65536-byte capture identified Cirrus Logic
GD-5422 VGA BIOS 1.00d, with a valid 32 KiB option-ROM checksum and an all-zero
upper half. Host CRC16 `39AF` matched the DOS reader. Repeatability has not
been checked. [Capture and evidence](https://github.com/ddanila/dumps/tree/main/unknown/cirrus-gd5422-27512-20260921).
