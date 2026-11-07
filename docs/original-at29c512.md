# Original Willem AT29C512 read comparison, 2026-11-07

The original Willem 0.97ja application confirms a control-level error in
`dosravi-at29c512-read-v1`: it reused the 27512 startup, holding DB25 pin 17
low. The AT29C512 requires that signal high with DIP634. Version
`dosravi-at29c512-we-fix-v2` establishes it before enabling VCC, preserves it
during the read, and disables VCC before clearing it at shutdown.

## Experiment

Downloaded [0.97ja from the vendor](https://www.mcumall.com/downloads/download.asp?DownloadID=7)
and extracted its Inno Setup archive. The [vendor manual](https://mcumall.com/support/DualPoweredWillemUserGuide.htm)
recommends 0.97ja or earlier for PCB3B compatibility mode.

- ZIP SHA256: `2df05d39707ddff23150242e1b6a4989db39e6baddb185221fbc3169159d0aa1`
- Unmodified `EpromM51.exe` SHA256: `0aa39ff1c5e1850a7800081f0874c4da72d26f5eda3526f8aeaf0b35f2d15d88`
- Wine 10.0, private prefix, Xvfb; replacement IO.DLL built from
  `tools/original_willem_io.c`. No physical LPT endpoint is opened.
- Selected **PCB3**, **Flash AT29C/29EExxx > AT29C512 (128)**, then **Action > Read**.
  The displayed capacity is FFFF, sector size 128, DIP ON 3,5,6,10,11.
- The stub supplies BUSY loopback from D0 for the hardware-presence check,
  and constant ACK=0 for dummy data. The application reports **Chip read OK**.
  This is transaction capture, not a full electrical/chip emulator or firmware dump.

A clean single-read capture contains 5,046,656 port operations: 196,638 control
writes, 4,325,728 data writes, and 524,290 status reads (two presence-test
samples plus 65,536 * 8 data samples). The decoder confirms the low 16 address
bits run from 0000 through FFFF exactly once and all 524,288 data samples have
raw CONTROL **05h**. The trace includes startup and shutdown.

Trace SHA256 (uncompressed CRLF):
`64020f6ba63c97b5d6578e9dc4670b3ae6a94dfebe2cd6b59b940ced9f70a9af`.
Local evidence: `~/fun/dosravi/sessions/willem-original-at29c512-20261107/`;
compressed raw trace, original archive/executable, settings/completion screenshots,
and the corrected DOS build. Proprietary binaries are not included in this repo.
An earlier exploratory capture had overlapping GUI read actions; it is excluded
from these results and retained separately in the temporary lab directory.

## What differs

| Read phase | Original 0.97ja | Old R29C512 | Corrected R29C512 |
|---|---:|---:|---:|
| Shift address / sample data, raw 37Ah | 05 | 0D | 05 |
| Select socket data path, raw 37Ah | 07 | 0F | 07 |
| DB25 pin 17 | High | Low | High |

The [PCB5.0 schematic](https://jopl.eu/files/Willeprog_PCB50_Schematics_Modified.pdf)
shows switch 11 joining the CE control net to WE' (ZIF and ordinary PLCC32
pin 31). Switch 8 is open in DIP634, leaving chip CE' low via its pulldown.
The schematic labels the host signal CE; that does not make it the selected
chip's CE. Raw control bit 3 is inverted at DB25 pin 17. The
[Atmel datasheet](https://www.allelcoelec.pt/datasheets.26/AT29C512-15JI.pdf)
requires /WE high, /CE low, /OE low for reading. This explains why the
27512 startup cannot be reused merely because both devices contain 64 KiB.
It is a strong candidate explanation for the BF captures, not proof that no
other physical issue remains.

The original emits 16 address clocks per byte after initialization. We retain
our physically validated 24-clock implementation: 16 clocks can leave upper
registers carrying previous-address bits. AT29C512 only uses A0-A15, but this
observation does not validate 16-clock shifting for other profiles/clones.
The latch sequences also differ in redundant edges; this change only corrects
/WE and does not claim byte-for-byte trace parity or equivalent wall-clock timing.

## Reproduce

Extract the original package privately, compile `tools/original_willem_io.c`
with a 32-bit MinGW compiler, and replace the extracted application's IO.DLL
with that synthetic DLL. Run from its directory using an isolated Wine prefix:

```
DISPLAY=:93 WINEPREFIX=/tmp/willem-lab-prefix WINEDEBUG=-all \
  WINEDLLOVERRIDES=io=n wine EpromM51.exe
```

Use an existing isolated X display, select the profile above, invoke Read once,
wait for Chip read OK, then close the application to flush `lpt-trace.log`.
The synthetic DLL never forwards port operations to hardware.

```
python3 tools/check_original_at29_trace.py /path/to/lpt-trace.log
```

The checker also accepts gzip traces. `make test` includes a DIP634 flash model
that rejects the old startup, checks all 65,536 addresses, and detects /WE low
while powered. `tests/test_fastaddr.py` compares 3,132 assembly/C traces,
including both pin17 states. DOSBox tests exercise the compiled 8086/80186
read commands. Physical verification of the corrected AT29C512 build remains
pending; no new chip read was performed during this investigation.
