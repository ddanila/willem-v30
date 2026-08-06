# Willem V30

Current development version: `0.1.0-dev`

Small, auditable MS-DOS tools for classic LPT-connected Willem EPROM
programmers, targeting the NEC V20/V30 and 8086 instruction set.
The DOS program is a headerless `.COM` built in the tiny memory model, keeping
code, data, heap, and stack within one 64 KiB segment.

The initial hardware target is a Willem PCB5.0E operating in its PCB3B
compatibility mode. The initial device targets are:

- 2764/27C64: read, blank-check, and verify first
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
3. Enable AT28C64 programming only after read operation is proven.

The final DOS distribution will use 8.3 filenames and CRLF text files.

Every real-hardware run writes a human-readable `WILLEM.LOG` while also showing
the same operational messages on screen. A compact full LPT trace can be
enabled for hardware diagnosis and emulator comparison; see
[`docs/logging.md`](docs/logging.md).
The exact Geepro revision, function mapping, read sequence, and an upstream
DIP-table discrepancy are recorded in
[`docs/geepro-audit.md`](docs/geepro-audit.md).

## DOS commands

The build supports read, blank-check, and verify directly. AT28C64 writing is
present but remains locked until the host validator creates `WRITE.OK` from two
matching known-Juku physical reads.

```text
WILLEM R2764  OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]
WILLEM W28C64 ROM.BIN [378] /WRITE
```

The optional LPT base is hexadecimal and defaults to `378`. Every operation
prints and logs a visual 12-switch DIP diagram. Geepro's mask maps bit 0 to
physical switch 1; follow the numbering and the `ON` mark printed on the DIP
bank. Verification images must be exactly 8192 bytes.

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
unconfirmed physical-board details prominently gated; it must not be treated
as final hardware instructions until the PCB5.0E jumpers, socket position, and
power procedure have been verified.

After two physical reads, enforce the hardware gate on the modern host with:

```sh
python3 tools/validate_read.py --unlock WRITE.OK \
    EXPECTED.BIN READ1.BIN READ2.BIN
```

The tool rejects wrong sizes, non-repeatable reads, disagreement with the known
image, and repeatable all-zero/all-FF stuck-bus captures.
