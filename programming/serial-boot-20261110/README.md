# Serial boot EPROM preparation — 2026-11-10

Prepared images and subsequent physical work are recorded below. Programming
results remain pending until a complete readback matches.

| Target | DOS filename | EPROM | Bytes | Serial protocol |
|---|---|---|---:|---|
| Robotron PC1715M/W | `R1715MW2.BIN` | K573RF5 / standard 2716 | 2048 | W2 monitor, 9600 8N1 |
| Kaypro 2 PC81-240A / PC003 | `KAYPRO2.BIN` | 2732; exact marking pending | 4096 | KP2 bootstrap, 19200 8O1 |

Both filenames fit DOS 8.3. These complete chip images were copied without
truncation, repetition, byte swapping or additional padding. `SHA256SUMS` and
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
2716. Physical acceptance remains pending.

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
The Kaypro 2732 still needs an identified part and a separately qualified writer;
do not inherit the RF2 voltage or pulse polarity for that chip.

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
