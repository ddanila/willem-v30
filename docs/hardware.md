# PCB5.0E physical setup

This document separates settings supported by schematics/manual evidence from
settings that must be checked on the exact physical board. Willem PCB5.0E
clones are not completely uniform.

## Observed on the user's board

The July 2026 photographs show a board marked `2015 PCB 5.0E`. The main ZIF
socket is empty, and the 12-way DIP bank is currently set to the documented
2764/27C64 pattern: switches 1, 2, 4, 6, and 9 toward the bank's printed `ON`
side. The paired three-pin headers `J9` and `J10` form the compatibility
selector. Physical ROM reads proved that this clone's modes are reversed
relative to its setup manual: both shunts toward DB25 select the PCB3B serial
protocol required by this utility. The photographs do not yet reveal the power-source selector legend
with enough confidence. Do not move that jumper based on these observations
alone. The user reports that a 12 V barrel adapter powers the board and permits
the existing software to access the inserted 2764, whereas the unpowered board
does not. The adapter's current rating and polarity remain to be recorded from
its label.

## Confirmed 28-pin settings

The 2764 and AT28C64 are 28-pin JEDEC parts placed at the end of the 32-pin
ZIF opposite its lever/pivot. They are not centered. Leave the first two
physical contact pairs at the lever/notch end empty:

```text
lever/notch end
ZIF contact pair:    1/32   2/31   3/30  ...  16/17
28-pin chip pair:     -/-    -/-   1/28  ...  14/15
                                             far end
```

Thus socket contacts 1/32 and 2/31 are the two empty rows, all at the lever
end. Chip pins 14/15 occupy the final socket row 16/17 at the opposite end.
The chip notch points toward the empty rows and the ZIF lever/pivot. This
placement is corroborated by the user's previously working placement, by
Geepro's `willem_28pin.png` socket artwork, and electrically by the manual's
ZIF address map: A12 is on socket contact 4, whereas A12 is pin 2 on both a
2764 and an AT28C64.

For both read-only device families:

- use the normal 28-pin EPROM/EEPROM routing, not the special 2716, 2732,
  2816, 28F, 29C, or large-EPROM jumper alternatives;
- select 5 V VCC;
- leave VPP in the normal/no-programming condition; the utility additionally
  holds the software-controlled VPP switch off throughout read/check/verify;
- place the A19/large-device routing jumper in its normal parked position;
- use exactly one board power source.

The 2764/27C64 and AT28C64 DIP mask is `12Bh` (ON: 1, 2, 4, 6, 9).
AT28C64 control-pin measurements confirmed the required idle, read, and write
levels with this setting. Mask bit 0 maps to numbered switch 1 in Geepro.

## 27256 / 27C256 read-only settings

Use the same Normal 28-pin J1/J2 routing and bottom-aligned placement as the
2764 above, with two empty rows at the lever end and the notch toward the lever.
Keep J9/J10 toward DB25, VCC at 5 V, and software VPP off.
Set DIP mask `1B3h`: ON 1,2,5,6,8,9; OFF 3,4,7,10,11,12.
This mask and placement come from the pinned Geepro `family=27256` entry.

`R27256` reads addresses `0000h` through `7FFFh` and saves exactly 32768 bytes
after safe shutdown. Output is transferred in 8 KiB chunks to avoid Dev86's
signed 16-bit stdio count boundary. Conservative hardware reads are expected
to take roughly eight minutes; set DOSRAVI `--exec-timeout 900` or longer.
The virtual-board test covers active-low CE and A13/A14, and DOSBox checks
the complete file. Use build `dosravi-27256-ce-fix-v2` or later: the original
R27256 implementation held CE high and produced programmer-output echo.
DIP1B3 and Normal routing remain the required settings.

## 27512 / 27C512 read-only settings

Use Normal 28-pin J1/J2 routing and the same bottom-aligned placement as
27256: two empty rows at the lever end, notch toward the lever. Keep J9/J10
toward DB25, 5 V VCC, and software programming VPP off.
DIP mask `1D4h`: ON 3,5,7,8,9; OFF 1,2,4,6,10,11,12.
Geepro's pinned `family=27512` entry supplies this mask; its `read_27512`
uses `start_action(0, 0)`, with CE low. A15 is chip pin 1.

`R27512` captures exactly 65536 bytes, addresses 0000h through FFFFh.
It streams 8 KiB chunks because a full image cannot fit in the COM segment;
OE is inactive between byte reads, including during file writes. The offset
and progress counters are 32-bit. CRC16 carries across chunks. On output
failure, the reader shuts down VCC/VPP and returns failure; any partial file
is not a successful dump. The read metric includes chunk file I/O and CRC.
Use build `dosravi-27512-read-v1` or later. Conservative reads take about
15 minutes; use DOSRAVI `--exec-timeout 1800`. `R27256` is not a substitute.

Sources:
- https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/drivers/willem.xml.in#L93-L102
- https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/chips/27xx.cpp#L336-L339

## К573РФ5 / 2716 read-only settings

К573РФ5 is a 24-pin, 2K x 8 2716-class EPROM. It must not use the normal
28-pin route above. With all power removed:

- keep J9+J10 in the already proven PCB3B position, both shunts toward DB25;
- set the address-routing headers J1 and J2 beside the ZIF to the manual's
  special `2716` pattern (both shunts on the ZIF-side pair), not `Normal`;
- set DIP mask `1A3h`: ON 1, 2, 6, 8, 9; OFF 3, 4, 5, 7, 10, 11, 12;
- retain 5 V VCC, the known barrel-power selection, USB disconnected, and the
  existing safe VPP-selector setting;
- place the 24-pin chip against the ZIF end opposite the lever, leaving four
  complete contact pairs empty at the lever/notch end; the chip notch faces
  those empty rows and the lever.

```text
lever/notch end
ZIF contact pair:    1/32  2/31  3/30  4/29  5/28 ... 16/17
24-pin chip pair:     -/-   -/-   -/-   -/-  1/24 ... 12/13
                                                       far end
```

Do not confuse J1/J2 beside the ZIF with the DB25-side J9/J10 compatibility
selector. Never move either pair while powered. `RRF5` is the RF5/RF2 read command:
it reads addresses `0000h` through `07FFh`, never enables software VPP, writes
exactly 2048 bytes to the output file, and performs a safe VCC/VPP shutdown
before saving the result.

## PCB3B compatibility selector (`J9` + `J10` on this board)

The utility implements the PCB3B serial address/data protocol. On the user's
2015 clone, the two adjacent three-pin headers `J9` and `J10` together form
the six-contact compatibility selector. They are between the LPT logic ICs
near the DB25 end of the board. Electrical testing overrides the manual and
labels for this particular board: both shunts must be toward DB25. Here each
`====` represents one shunt bridging two adjacent pins:

```text
Natural board orientation: DB25/LPT LEFT, power connectors RIGHT

PCB3B (tested/required):    DB25 <- J9  o====o  o -> RIGHT
                                    J10 o====o  o

256-byte alias (wrong):     DB25 <- J9  o  o====o -> RIGHT
                                    J10 o  o====o
```

The PCB5.0E setup manual calls the left position `0.98xx`/PCB35A and the right
position `0.97xx`/PCB3B. That description is wrong for this specimen. With the
shunts right, three independent reads returned the first 256 ROM bytes 32
times. With both shunts left, all 13 device address bits worked and the D15
dump matched the known EktaSoft serial-0031 family. Other PCB5.0E clones have
also been reported with the mode positions reversed, so copied labels or a
generic manual must not override a read-only address test.

Do not confuse `J9` + `J10` with the pair of closed jumpers next to the text
`OPEN=3.6V` beside the PLCC sockets. Those belong to the PLCC voltage/routing
area and are not the software-compatibility selector. Leave them closed for
this work.

Never move these shunts while either the programmer or computer is powered.

## Power and cabling

The parallel cable does not power the programmer. The board manual permits
either USB 5 V power or a 9/12 V supply through the DC jack, selected by the
board's separate two-shunt block marked `ADAPT` and `USB`. Never attach USB
power and the DC supply together. With USB power, VCC is fixed at 5 V. With DC
input, set the VCC selector to 5 V. This block has not yet been positively
located in the user's photographs; do not infer it from J9/J10.

For this board, retain the already working 12 V barrel configuration and leave
USB power disconnected. Do not change the power-source jumpers until their
`ADAPT`/`USB` markings are positively located. A successful access with the
existing software confirms that the selected barrel-input path supplies the
board; it does not by itself validate ROM data or the new LPT implementation.

A correct external supply does not feed its 9/12 V into the Pocket8086 LPT
connector; the board regulates its rails and shares logic ground through the
parallel cable. The dangerous cases are wrong DC polarity/voltage, selecting
the wrong power source, attaching both sources, moving jumpers live, or using
a non-straight-through parallel cable.

Safe connection sequence for the first test:

1. Shut down the Pocket8086 and remove programmer power.
2. Set J9+J10 to PCB3B, normal routing, 5 V VCC, DIP switches, and the
   power-source jumper.
3. Insert the chip with both unused contact rows at the lever end as described.
4. Attach a straight-through DB-25 male-to-female cable and secure it.
5. Attach only the selected programmer supply, then boot the Pocket8086.
6. Run the read command; after it returns and reports safe shutdown, shut down
   before moving the chip, jumpers, or cables.

## ST M2764AF1 programming hold point

Do not physically run `WM2764A` merely because its virtual tests pass. Before
the first chip, positively identify the board's VCC and VPP selectors and use
an empty ZIF socket with:

```text
WILLEM DM2764A 378 /VPP
```

At the pauses, meter from chip pin 14 / ZIF contact 16 ground. Require pin 28 /
ZIF30 to be 5.75..6.25 V and pin 1 / ZIF3 to be 12.2..12.8 V. Abort and remove
power if either is outside that range. Program only at 20..30 °C ambient, the
data sheet's 25±5 °C range. This diagnostic applies VCC before VPP
and removes VPP first, but `/VPP` and the empty-socket confirmation remain
operator interlocks. Never move a selector or chip while powered.

Only the exact ST `M2764A` algorithm is supported. With power removed, use the
same bottom-aligned 28-pin placement and DIP `12Bh`; select 6.0 V VCC and
12.5 V VPP. The write command requires `/WRITE`, `M2764A.OK`, a blank chip,
and an exact 8192-byte image:

Create that token only through `tools/create_m2764a_gate.py`, which requires
two independent all-FF reads, a nonuniform control-chip read to reject a stuck
bus, the complete M2764A marking, and the recorded meter values. It deletes a
stale token before validation and refuses measurements outside the data-sheet
ranges.

For a deliberately accepted first-chip experiment only, the gate generator can
record a measured 5.70..<5.75 V VCC with `--allow-marginal-vcc`. It marks the
token `VCC_STATUS=MARGINAL_EXPERIMENTAL_USER_ACCEPTED`; the value remains below
the data-sheet programming range and may cause programming or margin failure.
Values below 5.70 V or above 6.25 V remain blocked.

For a routine follow-up programming attempt, `--writer-blank-scan-only` may
replace the two separate host reads. This choice is recorded in the token and
relies on the writer's mandatory full VPP-off scan, which aborts before VPP on
the first non-`FF` byte. Retain the two-read mode when independent blank-read
evidence is part of the experiment.

```text
WILLEM WM2764A TARGET.BIN 378 /WRITE
```

It first reads every byte with VPP off and will not raise VPP unless all are
`FF`. It then performs ST Fast Programming and a full comparison at the
programming rails, powers down with VPP removed before VCC, and stops. That is
not the data sheet's final verification. Remove programmer power, return the
board to normal 5 V VCC with VPP=VCC/read configuration, repower, and run:

```text
WILLEM V2764 TARGET.BIN 378
```

Do not call the chip programmed successfully until this separate 5 V verify
passes. The reference board completed this gate on 2026-08-26 and programmed
four M2764AF1 devices; see
[`m2764a-physical-acceptance.md`](m2764a-physical-acceptance.md). That result
does not authorize an unmeasured clone or a different EPROM algorithm.

On 2026-08-09, a misplaced J2 produced a repeatable-looking but invalid
AT28C64 dump with address/page aliasing. After programmer power was removed and
J2 corrected, two independent reads matched the pinned source image exactly.
Treat an unexpected aliased dump as setup failure: do not validate or write
from it, remove power, and recheck J2 against the board markings before reading
again. The observation does not establish a universal J2 position for clones.

## First real-hardware gate

Read the known original Juku 2764 without any write-capable command:

```text
WILLEM R2764 JUKUROM.BIN 378 /TRACE
```

Preserve `JUKUROM.BIN`, `WILLEM.LOG`, and `WTRACE.BIN`. Perform at least two
separate reads, compare them byte-for-byte, and compare against the known Juku
image on the modern host. The repository supplies the strict acceptance test:

```sh
python3 tools/validate_read.py --unlock WRITE.OK \
    EXPECTED.BIN READ1.BIN READ2.BIN
```

It requires exact 8192-byte sizes, rejects uniform stuck-bus captures, compares
both reads byte-for-byte, compares the first read with the expected image, and
prints CRC16-CCITT plus SHA-256 identities. It deletes any stale token before
checking and creates the CRLF `WRITE.OK` only on success. Copy that token to
the DOS directory to unlock `W28C64`; without it, the command exits before
opening a trace or touching LPT registers.

For the EKTA 3.7 pair already prepared in the Jukuravi DOS ROM kit, select the
reference matching the socket from which the physical chip was removed:

| Socket | DOS reference | CRC16-CCITT | SHA-256 |
|---|---|---:|---|
| D15 | `D15EK37.BIN` | `060D` | `d6c4ec7418f05e5761ef450e6ee36fb2579d65d9cbf87dce265eaf1c0d077596` |
| D16 | `D16EK37.BIN` | `BE00` | `35b348ae7c88dc8cb24d1bc9d62a06212fdc2c2f601eddf8e00b233893d92817` |

Do not guess the socket identity: record whether the known EPROM came from D15
or D16 before comparing it.

## Sources

- [MCUmall Standard/Dual Powered Willem User Guide](https://mcumall.com/support/DualPoweredWillemUserGuide.htm)
- [Archived PDF of the same user guide](https://www.mikrocontroller.net/attachment/180082/Standard_Willem_Programmer_User_Guide.pdf)
- [PCB5.0E setup manual with J9/J10 and ADAPT/USB diagrams](https://supereyes.ru/img/instructions/WILLEM_PCB50E_models.pdf)
- [PCB5.0E J4 field report](https://www.retrobrewcomputers.org/n8vem-gg-archive/html-2011/Apr/msg00267.html)
- [Report of a clone with reversed selector behavior](https://modelrail.otenko.com/2020/07)
- [Pinned Geepro 2716 device entry](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/drivers/willem.xml)
- [Pinned Geepro 2716 read implementation](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/chips/27xx.cpp)
- [Intel 2716 data sheet](https://www.theoddys.com/acorn/semiconductor_datasheets/2716%20Intel.pdf)

## Empty-socket 2716 / РФ2 / РФ5 voltage diagnostic

`WILLEM D2716 378 /VPP` checks the manually selected 5 V VCC and 25 V
VPP with **no chip installed**. It does not program an EPROM or measure
voltages automatically. Keep J9/J10 toward DB25, J1/J2 on the ZIF-side
pairs, DIP 1A3h (ON 1,2,6,8,9), and the established barrel supply with USB
power disconnected. Set selectors only with power removed.

Viewed from above with the lever at the top, ZIF contacts run 1..16 down
left and 32..17 down right. Black probe: ZIF16 (chip ground pin12).
VCC: ZIF28 (chip24). VPP: ZIF25 (chip21). These are 2716-routing contacts,
not the contacts used by DM2764A.

The diagnostic requires a complete `YES` line at each step:

1. Confirm the empty socket and settings, then enable VCC only.
2. Measure ZIF28 and ZIF25: both should be 4.75..5.25 V. Confirm to enable VPP.
3. Record ZIF25 (target 25.0 V, 24.5..25.5 V) and ZIF28 (4.75..5.25 V).
   Confirm to remove programming VPP.
4. Check ZIF25 has returned to the 5 V read rail, then confirm to remove VCC.

Any other response or EOF exits through VPP-before-VCC shutdown, returning
failure. Use a non-YES response to stop normally; if the computer hangs or
voltage is wrong, remove programmer power. Do not use Ctrl-C as a rail-off
command. No programming pulses are issued. Never redirect prefilled YES
answers on real hardware: each confirmation follows an actual meter check.
The diagnostic waits for local keyboard input; DOSRAVI can launch it, but the
operator must answer at the laptop. This is an unloaded check, not evidence
that the converter maintains voltage under programming load.

Validation: `make dos test` and `python3 tests/test_2716_diag.py`.
The latter executes the actual COM on emulated 8086/80186 CPUs and checks
port traces for rail ordering, EOF at every pause, and rejection before I/O
without `/VPP`. Physical 25 V measurement remains pending.

### Owner measurement, 2026-11-03: excessive VPP before diagnostic

With the requested 2716 jumpers set, barrel supply connected, socket empty,
and plain DOS booted without programmer software, the owner observed the
VPP LED lit. Meter readings: **31 V between ZIF25 and ZIF16**, confirmed
measurement contacts; **VCC 5.03 V**. The linear regulator felt hot; no
measured temperature or current was supplied. No EPROM was installed and
D2716 had not been run. Advised removing programmer power.

The measured VPP exceeds the РФ2/РФ5 programming limit; do not insert a chip.
VCC is within 4.75..5.25 V. Boot-time LPT state may explain enablement, but
neither that cause nor the cause of the excessive voltage is established.
Recheck the exact selector arrangement and regulator circuit before proceeding.
A heatsink would not establish correct voltage regulation. The first live
check should establish both rail-off control and correct unloaded voltage;
loaded regulation remains unverified.

Follow-up unloaded readings from the owner, same session:

| VPP selector label | Measured VPP |
| --- | --- |
| 12 V | 12.66 V |
| 15 V | 15 V |
| 21 V | approximately 31 V |
| 25 V | approximately 31 V |

Research did not locate an exact matching fault report. The PCB3B converter
schematic reproduced by PY2BBS shows a 15 kohm feedback resistor R11 bypassed
by J6 for the lower settings and inserted for the higher settings. Thus an
open resistor/joint/track in that branch is a plausible explanation for both
high settings losing regulation while the low settings work. This is an
inference from a related circuit, not a confirmed component diagnosis or a
verified reference designation on this 2015 clone. First check selector
orientation/contact and map that feedback branch on the actual board, with
power disconnected. Do not replace components solely by the reference label.

Sources:
- https://www.py2bbs.qsl.br/download/willem_pcb3b_to_pcb45.pdf (page 5 schematic)
- https://www.ti.com/lit/ds/symlink/mc34063a.pdf (feedback comparator / divider)
- https://www.mikrocontroller.net/attachment/180082/Standard_Willem_Programmer_User_Guide.pdf (selected VPP should match the meter reading, approximately 0.5 V tolerance)

## 2732 read-only settings

With power removed, select the special **2732** J1/J2 pattern (not Normal
or 2716), DIP `1C0h`: ON 7,8,9; all others OFF. Keep J9/J10 toward DB25
on this tested clone, 5 V VCC, software VPP off, established barrel supply
selection and USB disconnected. Bottom-align the 24-pin chip: four empty
rows at the lever end, notch toward lever, chip pin 1 at ZIF5.

`R2732 FILE.BIN 378 /PROFILE:conservative` reads exactly 4096 bytes
(0000h–0FFFh), including A11, and shuts down VCC/VPP afterward. No 2732
write command is provided by that read-only build; the separate 21 V D2732A
writer is documented below. Build ID: `dosravi-2732-read-v1`. Software checks
cover full-address readback and DOS command handling; physical validation
is pending. Do not use R2716, which reads only 2048 bytes.

The DOS executable is 30288 bytes, CRC32 `bea303d3`, SHA256
`eece740b613644a470712eb1d550aafe45875ac9d3cf13b4ebbca4f87a9a8969`.
A local copy is saved as `build/dos/RD2732.COM` in the primary checkout.

Reference: Geepro's pinned
[2732 read controls](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/chips/27xx.cpp#L316-L319)
and [DIP mask](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/drivers/willem.xml.in#L66-L75).

## Accelerated 27C512 reads (2026-11-07)

Build `dosravi-27512-fastaddr-v1` enables the previously hardware-tested 8086
assembly address shifter for `R27512` with `/FASTADDR`:

```
RD512F1.COM R27512 CHIP.BIN 378 /PROFILE:conservative /FASTADDR
```

The 24 address clocks and port values are identical to the portable path;
CPU callback overhead is removed. Address/OE/data settling waits and the data
sampling algorithm are retained. VPP stays off, CE stays low, and all 65536
bytes including A15 are streamed. The option rejects other commands and tracing
before hardware I/O. No original Windows Willem trace was required: equivalence
is checked against the existing working protocol, not asserted against an
unobserved original-software trace.

Expected Pocket8086 read time is about six minutes, extrapolated from the
previous 131072-byte fast-address read (715935 ms); this 27C512 build has not
been physically timed yet. Allow EXEC timeout 1800 seconds and serial timeout
20 seconds for retrieval. Without /FASTADDR the portable path remains available.

Validation: `make test dos dos-test` and `python3 tests/test_fastaddr.py`
(the latter requires Unicorn). The address test executes actual 8086 code and
compares 1566 complete port-write traces including 0000/7FFF/8000/FFFF and
spread-out 16-bit addresses. DOSBox checks 65536-byte output on 8086/80186.
The optimization does not establish current physical setup or chip condition.

The old 32 KiB static buffer left insufficient COM stack/heap headroom after
adding code (caught by DOSBox before hardware use). This build streams R27256
through the same 8 KiB buffer, preserving its existing startup and portable
read path. R27256/R27512 full-file regression checks cover both streamed sizes.
`python3 tests/test_fastaddr_guard.py` checks option rejection under DOSBox.

Prepared executable: `RD512F1.COM`, 30796 bytes, CRC32 `522490ba`,
SHA256 `782daca83f37133092f10b34b0c94c2c29a903ad51c21a85a06bfd7a709ef936`.

## AT29C512, including PLCC32 (2026-11-07)

Read-only command `R29C512` (alias `RAT29C512`) captures 65536 bytes and accepts
`/PROFILE:conservative /FASTADDR`. No AT29C512 erase, program, unlock or product-ID
command is implemented. The data-sheet read condition is CE low, OE low and WE
high. Reading requires no command sequence. Expected time is approximately six
minutes on Pocket8086; physical verification remains pending.

Use Normal J1/J2, DIP634: ON 3,5,6,10,11; all others OFF. Keep J9/J10
toward DB25, 5 V VCC and programming VPP off. The original Willem 0.97ja
AT29C512/PCB3 profile displays this same switch setting. The published PCB5.0
schematic connects the ordinary **PLCC32** socket in parallel with ZIF32;
the **FWH/LPC** socket has different wiring and is not appropriate.
A JEDEC PLCC32-to-DIP32 adapter is another option; fill all 32 ZIF contacts
and orient its DIP pin 1 toward the lever. Match chip and socket pin-1 markings.

AT29C512 PLCC/DIP pins: 32 VCC, 16 GND, 31 /WE, 22 /CE, 24 /OE, 3 A15;
1,2,30 NC. In DIP634, switch 11 routes **DB25 pin 17 to /WE**. Hold that
host pin HIGH, unlike the 27512 profile. Switch 8 OFF leaves /CE low.
The accelerated address function and data sampling remain shared with R27512.

**Correction:** v1 incorrectly used the 27512 startup and held /WE low.
The previous claim that this startup kept pin31 inactive was wrong.
Do not use RD29V1.COM for further captures. Version
`dosravi-at29c512-we-fix-v2` has dedicated flash startup/shutdown and keeps
/WE high from before VCC-on until after VCC-off. See
[original-software experiment](original-at29c512.md) for trace evidence,
sources and the distinction between software validation and physical readback.

AT29C512 build `dosravi-at29c512-read-v1`: `RD29V1.COM`, 31264 bytes,
CRC32 `acd0b105`, SHA256
`d9e0ef92cd6cec65fe8a93b74610b4d8e070f78c3e0e78d40ebb88465934af46`. DOSBox 8086/80186 full-size
read checks passed but did not model flash /WE; this build is superseded.

Corrected local `RD29V2.COM`: 31336 bytes, SHA256
`937e232bd00ee32480c3ed29dd6378f3792888921b9ab7fb70fc7dd6901397e4`.
Uploaded and physically read on 2026-11-07 (359370 ms, 65536 bytes, exit 0,
VCC/VPP-off logged). The BF pattern disappeared, but the capture contains
64742 FF bytes and 794 anomalous 04/06 bytes exclusively at offsets
125..127 modulo 128. This is not validated firmware or a confirmed blank
chip; a remaining read-path issue is unresolved. Evidence: DOSRAVI session
`norma-vmd4-at29c512-20261107-read3-resume`.

## Standard 2716 / RF2 / RF5 programming

`W2716` (aliases `WRF2`, `WRF5`) uses the same special 2716 routing and
DIP `1A3`, with measured **5 V VCC and 25 V programming VPP**. The positive
50 ms pulse is specific to this family. See the root README for the write
gate and verification procedure. Do not use this command for a 2732 or the
three-supply TI TMS2716.

## Intel D2732A programming (21 V)

`make build/dos/PG2732A.COM` builds a dedicated, fixed-LPT378 writer. It reuses
`willem.c` for 24-bit address shifting and reading, and the PIT2 mode-0 timer
for a 50 ms **active-low** CE pulse. The 2716 entry retains its positive pulse.
This is specifically for the Intel 21 V D2732A, not arbitrary 2732 variants.
Use special 2732 J1/J2, DIP ON 7,8,9, J9/J10 toward DB25, VCC 5 V,
VPP 21 V. VPP/OE is chip20/ZIF24; VCC chip24/ZIF28; ground chip12/ZIF16.

After confirming the part, routing and measured supply, create `2732A.OK`
containing the single line `D2732A-5V-21V-50MS` (newline terminated).
Run `PG2732A IMAGE.BIN /WRITE21` with an exactly 4096-byte image. The program
checks the whole chip for FF, programs the first intended non-FF byte, and scans
all addresses for unexpected changes before continuing. It skips FF image
bytes, uses one pulse per programmed byte without retries, verifies each byte
with VPP off, then power-cycles and verifies all 4096 bytes. Any mismatch stops
with both rails off. No file or console operations occur while VPP is enabled.
Result codes: 1 arguments/files/gate, 2 nonblank, 3 initial probe mismatch,
4 byte mismatch, 5 final verification mismatch; zero means complete match.
An independent read with the established RD2732.COM should follow programming.

The Intel datasheet specifies 5 V ±5%, VPP 21 V ±0.5 V, and a 45–55 ms CE
pulse (50 ms nominal). Address/data and OE hold/setup requirements are respected
with conservative delays. The legacy PIT0 setup delays remain shorter than their
nominal arguments but comfortably exceed the 2 µs minimum; pulse width uses
PIT2, not that routine. The timer owns the speaker channel and restores port61.
Software cannot measure the actual rails or analog edge shape.

Sources: [Intel 2732A datasheet](https://www.ecbazis.hu/pdf/2732-intel.pdf),
[Geepro 2732 programming controls](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/chips/27xx.cpp).
Validation: `make 2732a-test` executes the actual DOS binary with a virtual
4096-byte socket, including full Kaypro programming, A6/A9/A11 aliases, a chip
that does not program, nonblank refusal, and invalid gate/image handling.
`tests/test_2716_pulse.py` uses Unicorn to verify both actual assembly pulse
polarities, duration, preserved registers/flags and speaker state.
