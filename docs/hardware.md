# PCB5.0E physical setup

This document separates settings supported by schematics/manual evidence from
settings that must be checked on the exact physical board. Willem PCB5.0E
clones are not completely uniform.

## Confirmed common settings

Both supported devices are 28-pin JEDEC parts placed in the 32-pin ZIF socket
with an offset of two socket positions:

```text
32-pin ZIF contact:  1  2  3  4 ... 29 30 31 32
28-pin chip pin:     -  -  1  2 ... 27 28  -  -
```

Thus socket contacts 1, 2, 31, and 32 remain empty. The chip notch points in
the same direction as the ZIF/silkscreen notch. This placement is corroborated
electrically by the manual's ZIF address map: A12 is on socket contact 4,
whereas A12 is pin 2 on both a 2764 and an AT28C64.

For both read-only device families:

- use the normal 28-pin EPROM/EEPROM routing, not the special 2716, 2732,
  2816, 28F, 29C, or large-EPROM jumper alternatives;
- select 5 V VCC;
- leave VPP in the normal/no-programming condition; the utility additionally
  holds the software-controlled VPP switch off throughout read/check/verify;
- place the A19/large-device routing jumper in its normal parked position;
- use exactly one board power source.

The 2764/27C64 DIP mask is `12Bh` (ON: 1, 2, 4, 6, 9). The AT28C64 mask is
`128h` (ON: 4, 6, 9). Mask bit 0 maps to numbered switch 1 in Geepro.

## PCB3B compatibility selector

The utility implements the PCB3B serial address/data protocol. A documented
PCB5.0E uses the six-pin J4 block labelled `5.0E <-> 3B`; both shunts are moved
to the right-hand pair of columns for 3B. Contemporary user reports confirm
that arrangement on KEE PCB5.0E boards.

Do not rely on direction alone. At least one later clone has the selection
reversed. Before the first run, confirm that the exact board's silkscreen marks
the selected side `3B`, or compare a clear board photograph with the J4 traces.
Never move these shunts while either the programmer or computer is powered.

## Power and cabling

The parallel cable does not power the programmer. The board manual permits
either USB 5 V power or a 9/12 V supply through the DC jack, selected by the
board power-source jumper. Never attach USB power and the DC supply together.
With USB power, VCC is fixed at 5 V. With DC input, set the VCC selector to 5 V.

A correct external supply does not feed its 9/12 V into the Pocket8086 LPT
connector; the board regulates its rails and shares logic ground through the
parallel cable. The dangerous cases are wrong DC polarity/voltage, selecting
the wrong power source, attaching both sources, moving jumpers live, or using
a non-straight-through parallel cable.

Safe connection sequence for the first test:

1. Shut down the Pocket8086 and remove programmer power.
2. Set J4/PCB3B, normal routing, 5 V VCC, DIP switches, and power-source jumper.
3. Insert the chip with all four unused ZIF contacts visible as described.
4. Attach a straight-through DB-25 male-to-female cable and secure it.
5. Attach only the selected programmer supply, then boot the Pocket8086.
6. Run the read command; after it returns and reports safe shutdown, shut down
   before moving the chip, jumpers, or cables.

## First real-hardware gate

Read the known original Juku 2764 without any write-capable command:

```text
WILLEM R2764 JUKUROM.BIN 378 /TRACE
```

Preserve `JUKUROM.BIN`, `WILLEM.LOG`, and `WTRACE.BIN`. Perform at least two
separate reads, compare them byte-for-byte, and compare against the known Juku
image on the modern host. The repository supplies the strict acceptance test:

```sh
python3 tools/validate_read.py EXPECTED.BIN READ1.BIN READ2.BIN
```

It requires exact 8192-byte sizes, rejects uniform stuck-bus captures, compares
both reads byte-for-byte, compares the first read with the expected image, and
prints CRC16-CCITT plus SHA-256 identities. AT28C64 programming remains
disabled until it prints `READ GATE: PASSED`.

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
- [PCB5.0E J4 field report](https://www.retrobrewcomputers.org/n8vem-gg-archive/html-2011/Apr/msg00267.html)
- [Report of a clone with reversed selector behavior](https://modelrail.otenko.com/2020/07)
