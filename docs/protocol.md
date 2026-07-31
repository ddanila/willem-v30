# Classic Willem PCB3B LPT protocol

This project targets the PCB5.0E board with its hardware-version selector in
PCB3B compatibility mode.

## Provenance

The initial protocol implementation is a clean, reduced port of the GPL-2.0
Geepro Willem driver, particularly `drivers/willem.cpp` and `src/parport.cpp`.
Geepro is copyright 2006 Krzysztof Komarnicki and contributors. See
<https://github.com/danielg4/geepro>.

The implementation will also be compared with I/O traces from the original
Willem DOS software before it is used to write a physical device.

## PC port registers

For LPT1 at base `378h`:

| Port | PC register | Use |
|---|---|---|
| `378h` | Data | Address shift clock/data, ROM data, readback clock |
| `379h` | Status | Serial ROM readback on ACK/DB-25 pin 10 |
| `37Ah` | Control | VPP, multiplexor/OE, VCC, and WE/CE/PGM |

Control-register bits 0, 1, and 3 are inverted between the PC register and
DB-25 pins 1, 14, and 17. Therefore logical connector values are XORed with
`0Bh` before writing `37Ah`. Bit 2, DB-25 pin 16, is not inverted.

## Signal assignments

| DB-25 pin | Register bit | Willem use |
|---|---:|---|
| 2 / D0 | data bit 0 | Address shift clock |
| 3 / D1 | data bit 1 | Address serial data |
| 4 / D2 | data bit 2 | Readback latch/shift clock |
| 2-9 | data bits 0-7 | Programming data when the multiplexor selects data |
| 10 / ACK | status bit 6 | Active-low serial ROM readback |
| 1 / STROBE | control bit 0 | VPP switch |
| 14 / AUTOFEED | control bit 1 | Address/data multiplexor and OE path |
| 16 / INIT | control bit 2 | VCC switch |
| 17 / SELECT | control bit 3 | WE/CE/PGM path selected by board DIP routing |

## Address and data read

For an 8 KiB device, addresses are shifted MSB-first from A12 through A0.
Geepro initializes its address mask to bit 12 (`1000h`). Each address bit is
placed on D1, then clocked with a high-to-low pulse on D0.

The ROM data bus is parallel inside the programmer but is read back through a
parallel-in/serial-out chain. The host latches it with D1/D2, then samples the
active-low ACK input eight times from D7 through D0 while pulsing D2.

## Timing portability

Sub-millisecond waits do not use instruction-counted delay loops. The DOS I/O
layer latches and reads PC PIT channel 0 and waits for elapsed ticks from its
fixed 1.193182 MHz input clock. Latching does not reprogram the timer. This
makes minimum signal spacing independent of whether the host is an Intel 8086,
NEC V30, or a faster compatible CPU. All current waits use this path, including
the 50 ms chunks used for the 28C64 power-on delay. Each chunk is shorter than
one approximately 54.9 ms PIT period, so 16-bit modulo elapsed-time arithmetic
remains unambiguous. No AT-only `INT 15h/AH=86h` BIOS service is required.

## Device settings found in Geepro

Geepro's `willem.xml` contains these 12-switch masks:

| Device family | DIP mask |
|---|---:|
| 2764/27128 | `12Bh` |
| M28C64/2864 | `128h` |

Geepro maps mask bit 0 to numbered switch 1, bit 1 to switch 2, and so on.
Consequently the settings displayed by the DOS utility are:

| Device family | Switches ON | Switches OFF |
|---|---|---|
| 2764/27C64 | 1, 2, 4, 6, 9 | 3, 5, 7, 8, 10, 11, 12 |
| AT28C64 | 4, 6, 9 | 1, 2, 3, 5, 7, 8, 10, 11, 12 |

The utility renders both the ON and OFF rows on screen and in `WILLEM.LOG`.
Follow the numbers moulded or printed on the DIP bank; move each lever toward
the bank's printed `ON` side only when the X appears in the rendered ON row.
The PCB5.0E socket position and other board jumpers still require confirmation
against the original Willem application or a reliable PCB5.0E manual.

## Safety gate

Read-only operation never enables VPP. Physical programming support remains
disabled until the virtual-board tests, original-software trace comparison,
and known-Juku-2764 read validation all pass.
