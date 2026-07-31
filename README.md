# Willem V30

Current version: `0.0.2`

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
WILLEM R2764  OUT.BIN [378] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]
WILLEM W28C64 ROM.BIN [378] /WRITE [/TRACE]
```

The optional LPT base is hexadecimal and defaults to `378`. Every operation
prints and logs a visual 12-switch DIP diagram. Geepro's mask maps bit 0 to
physical switch 1; follow the numbering and the `ON` mark printed on the DIP
bank. Verification images must be exactly 8192 bytes.

`W28C64` additionally requires an explicit `/WRITE` and a valid `WRITE.OK` in
the current DOS directory. The normal distribution intentionally contains no
token. A successful `validate_read.py --unlock WRITE.OK ...` run creates it;
copy it to the DOS disk only after reviewing the reported identities. The
writer keeps VPP off, skips already matching bytes, D7-polls every changed
byte, and performs a complete post-write verification.

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
