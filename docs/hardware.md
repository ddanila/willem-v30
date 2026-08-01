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

## Confirmed common settings

Both supported devices are 28-pin JEDEC parts placed at the end of the 32-pin
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
