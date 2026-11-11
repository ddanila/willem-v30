# Serial boot EPROM preparation — 2026-11-10

Prepared images and subsequent physical work are recorded below. Kaypro and
Robotron D2732A programming now have matching independent readbacks; target
machine boot tests remain pending.

| Target | DOS filename | EPROM | Bytes | Serial protocol |
|---|---|---|---:|---|
| Robotron PC1715M/W | `R1715MW2.BIN` | K573RF5 / standard 2716 | 2048 | W2 monitor, 9600 8N1 |
| Robotron PC1715M/W (doubled) | `R1715M4K.BIN` | Intel D2732A-3, 21 V | 4096 | W2 monitor, 9600 8N1 |
| Kaypro 2 PC81-240A / PC003 | `KAYPRO2.BIN` | Intel D2732A-3, 21 V | 4096 | KP2 bootstrap, 19200 8O1 |

All filenames fit DOS 8.3. The original 2 KiB Robotron and 4 KiB Kaypro images were copied without
truncation, repetition, byte swapping or additional padding. `R1715M4K.BIN`
is the later, explicitly doubled Robotron image for a 2732A. `SHA256SUMS` and
`manifest.json` identify them and their source revisions. ROM source licenses
are included in `LICENSE-ROM-SOURCES`.

The Robotron image is Ravi's current **W2 diagnostic/upload/run monitor** for
1715M/W. It is not the plain PC1715 stock-ROM serial protocol or the CP/M
repository's CI test ROM. It reproduces the checked-in binary exactly, including
its build fingerprint and final CRC16. It can receive code without a floppy,
keyboard, display, or a working main-RAM stack; it is not CP/M itself.

The Kaypro image is the repository's **KP2 replacement boot ROM**. Its matched
CP/M platform and disk were built alongside it; their sizes and hashes are in
`evidence/kaypro-manifest.json`. Use the matching artifacts from
`/home/ddanila/fun/cpm-plus-kaypro/build/platform.cim` and
`/home/ddanila/fun/cpm-plus-kaypro/out/kaypro.img` with that repository's launcher.
The owner confirms that a 2732 is required. Do not truncate this image for a
2716. Programming and independent full readback passed on 2026-11-11;
booting the physical Kaypro remains untested.

## Verification

From this directory, run `sha256sum -c SHA256SUMS`.

Robotron rebuild:

```
cd /home/ddanila/fun/ravi
PYTHONPATH=src python3 -m ravi.backends.robotron1715w.build --output build/programming-20261110/R1715MW2.BIN
cmp build/programming-20261110/R1715MW2.BIN src/ravi/backends/robotron1715w/rom.bin
```

Kaypro rebuild: `git submodule update --init --recursive` followed by `make all`
in `cpm-plus-kaypro`. Boot-protocol and driver checks run through the native
board model using the pinned dac-emulation revision. Reports are in `evidence/`.
These checks establish software behavior, not physical programming success.

## Willem repair and remaining steps

Owner report: missing **R11** was populated with **15.4 kΩ**, and the adjacent
trimmer was adjusted to measure **25.0 V at the ZIF VPP contact**. This was in
the known default/uninitialised LPT state, which asserts VPP. It does not indicate
a switching fault or establish the switched-off and loaded rail voltages.

For RF5, retain special 2716 routing (J1/J2 ZIF-side pairs), DIP ON 1,2,6,8,9,
J9/J10 toward DB25, 5 V VCC and 25 V programming VPP. With the lever at the top,
chip pin 1 sits at ZIF5; four rows are empty at the lever end. Ground is ZIF16,
VCC ZIF28, VPP ZIF25. Move chips and jumpers with power removed.

The current preparation branch includes the empty-socket diagnostic:

```
WILLEM D2716 378 /VPP
```

It requires an empty socket and actual meter readings before each local `YES`.
The owner confirmed the post-repair switched-rail measurements during this
programming session. No automated D2716 measurement is claimed.

The custom programmer now provides `W2716` / `WRF2` / `WRF5` programming.
The Kaypro D2732A-3 now has a separately qualified 21 V writer, `PG2732A.COM`;
see the completed programming record below. It uses active-low CE, unlike RF2.

## Safe idle command executed after preparation

`tools/wloff.asm` builds with `nasm -f bin tools/wloff.asm -o build/dos/WLOFF.COM`.
This 8086 DOS utility is fixed to the established LPT address 378h and PCB3B
control mapping. It first sets raw control bit 0 to disable VPP while preserving
other bits, then writes CONTROL=0Bh (logical controls inactive) and DATA=00h.
It does not enable a rail, issue a programming pulse, or access a chip.

On 2026-11-10, `WLOFF.COM` was uploaded and executed through DOSRAVI at 57600
baud, execution ID 3966160539, exit code 0, elapsed 55 ms. Captured stdout confirms
the port writes. Session: `dosravi/sessions/willem-safe-off-20261110`.
A meter check of the resulting rail voltages remains an operator measurement;
this execution is not a completed D2716 switched-rail diagnostic.

## First RF5 read after repair

The owner confirmed both rails off after WLOFF, then installed a potentially blank
RF5. The existing RD2716.COM (CRC32 d4164322, matching the successful FACIT
2716 reader) completed a 2048-byte read, but returned 1023 bytes of 06h,
1021 bytes of 04h, and four zero bytes. This resembles programmer signal echo;
it does not establish that the chip is programmed or blank. No echo binary is
archived here. Local acquisition evidence remains in
`dosravi/sessions/rf5-candidate-20261110-read1`. Software logged VCC/VPP off
at completion. Recheck routing, DIP settings, chip placement and contacts before
using this read path as a programming gate.

The owner substituted a second chip, retaining the switch settings, and later
identified it as RF2 (not RF5). Its complete
2048-byte read returned FF at every address, consistent with a blank device and
different from every byte of the first capture. This does not independently
prove the present read path or explain the first chip's echo. Read time was
28.050 seconds; software logged both rails off. The all-FF binary remains only
in the local DOSRAVI session, not this repository. See
`evidence/rf5-second-chip-read.json` for provenance.

RF2 identification correction: the second all-FF device is RF2; the first device
remains RF5. The first RF5 is suspect, not proven defective: signal echo and a
contact/routing issue are not distinguished by an all-FF replacement read.
The RF2 data sheet specifies the standard 2K x 8 pinout, 5 V supply and 25 V
programming input: https://static.chipdip.ru/lib/818/DOC054818967.pdf .
The RF2/2716 writer has now been implemented. The owner confirmed the switched
rail measurements after the R11 repair ("I already measured, all good").
See the programming evidence for the eventual write/readback result.

## RF2 programming attempts

The first attempt passed its blank check, then stopped at address 0001.
Independent readback found C3 at 0000 and 06 at 036D; all other bytes were FF.
The new writer was at fault: its address call lacked an explicit 32-bit cast,
which the old K&R DOS compiler requires. That chip needs UV erasure because
036D must contain DD. The owner installed a replacement blank chip.

Build `dosravi-rf2-program-v2` fixes the call. A new regression test executes
the actual bcc-compiled core across all 2048 addresses on both 8086 and 80186;
it passes the fixed code and rejects the original defect. Pulse assembly,
write guards, and portable tests also pass. Attempt logs and readbacks are
preserved in `evidence/`; physical success requires a complete image match.

The replacement chip passed its blank check and verified through address 003F,
but failed at 0040 (C1 instead of DD). Independent readback shows identical
64-byte blocks at 0000 and 0040; C3 AND DD = C1 confirms the observed alias
pattern. Reseating produced an identical SHA-256. A6 is chip pin 2 / ZIF6.
The first chip distinguished A6, pointing more strongly to the replacement
chip/contact than to a permanent shared programmer fault; the exact cause
remains unresolved. Both attempts ended with VPP/VCC off. Neither chip contains
a usable boot image; affected chips require UV erasure before reuse.

## Third candidate: КС573РФ2

The owner supplied the full marking КС573РФ2 and confirmed another blank chip
installed. Build `dosravi-rf2-program-v3` adds an initial alias check: program
only the first intended non-FF image byte, switch VPP off, and scan all 2048
addresses. Only that location may be non-FF. This adds no arbitrary test data
and does not repeat the first programming pulse. A mismatch aborts before
further programming. The actual DOS-compiled core accepts an isolated byte and
rejects simulated A6 mirroring on both 8086 and 80186.

The [RF2 family data sheet](https://static.chipdip.ru/lib/818/DOC054818967.pdf)
also explicitly lists КС573РФ2; the existing family profile is retained.

The owner then authorized unattended destructive experiments on the installed
chip. [The complete investigation](RF2-LAB.md) isolates stable A9 aliasing:
1005 additional one-pulse writes and three independent full readbacks passed
when addressing the 1024 distinct locations. The exact electrical cause still
requires measuring chip22/ZIF26. Both rails are off.

## Three additional РФ5 chips, read only

The owner supplied three further РФ5 chips, numbered 1–3 in reading order.
Each returned exactly 2048 FF bytes in both the existing conservative reader
and the separate assembly reader. All six executions exited zero and ended
with VPP/VCC off. No programming pulses were sent. Blank captures cannot
establish address-line independence or programming suitability. The binaries
remain local; [read metadata](evidence/rf5-three-reads.json) records all six
execution IDs. Chip #3 is the last installed chip, with both rails off.

## Kaypro D2732A-3 programmed — 2026-11-11

Owner-applied physical chip label: **KP2NETv1**.

The owner installed a D2732A-3 marked for 21 V programming, confirmed the
2732 settings, and measured VPP at 21 V. The established RD2732.COM reader
returned 4096 FF bytes. The dedicated writer repeated the full blank check,
programmed the first intended byte and scanned for aliases, then programmed
the remaining image with one PIT2-timed 50 ms negative CE pulse per non-FF
byte. All 898 pulses succeeded; all 4096 bytes matched after power-cycling.

A separate RD2732.COM read then matched KAYPRO2.BIN byte-for-byte:
`6133c46b1b31a8cc1d93d2428b716399c9c787d1ee6c5e3b785f67853e569633`.
Both rails were switched off after verification. The chip now contains the
KP2 serial boot ROM (19200 8O1); operation in the physical Kaypro is untested.
This successful 2732A burn does not settle the earlier RF2 A9 electrical fault.
No R22 replacement was reported.

[Evidence and writer identity](evidence/d2732a/manifest.json) include execution
IDs, source image and binary hashes, blank-read metadata, write report, and
the independent readback. Actual DOS emulation passed full-image programming,
A6/A9/A11 alias rejection, failed-chip and nonblank refusal, and file/gate
guards. Pulse assembly tests cover timing, both polarities, and saved state;
the old 2716 path and portable core checks also passed.

## Robotron 1715M W2 in D2732A-3 — 2026-11-11

A second D2732A-3 was installed with the same 21 V programming setup used for
KP2NETv1. `R1715M4K.BIN` is exactly `R1715MW2.BIN` concatenated with itself:
0000–07FF and 0800–0FFF each contain the complete 2048-byte W2 monitor, including
its original checksum. Serial framing is **9600 8N1**.

The independent initial read returned 4096 FF bytes. The unchanged, qualified
PG2732A.COM then passed its own blank check, first-byte alias scan, all 3478
single-pulse writes, and complete power-cycle verification. A separate read
with RD2732.COM matched all 4096 bytes; each half also matches the original
2 KiB image independently. Both rails are off. Owner-applied physical chip label: **1715MNETv1**. Actual booting in the Robotron remains untested.

Full-image SHA-256:
`35504bdb70052987701b7f26069b83dd7ab4b96927893e572f1ddd1bbb544750`.
[Programming and readback evidence](evidence/robotron-d2732a/manifest.json).

The 2732 uses pin21 as A11 where a standard 2716 uses its read-mode VPP input.
Duplicating the image supplies the same code for either stable A11 logic level;
[Dataman documents this doubled-image arrangement](https://www.dataman.com/pages/dataman-s4-programming-the-2716).
This does not resolve the earlier RF2 programming fault.
