# Geepro protocol audit

The Willem LPT core is a reduced C89 port of the GPL-2.0-or-later Geepro
implementation. This audit pins the upstream evidence to commit
[`a08efcaf6479730d552c5f96bad0a2a01bf0635f`](https://github.com/danielg4/geepro/tree/a08efcaf6479730d552c5f96bad0a2a01bf0635f)
so later upstream changes cannot silently alter the claimed provenance.

## Driver mapping

The relevant upstream files are
[`drivers/willem.cpp`](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/drivers/willem.cpp)
and
[`src/parport.cpp`](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/src/parport.cpp).

| This project | Geepro operation | LPT signal |
|---|---|---|
| `wl_vpp` | `willem_vpp_on/off` | control bit 0 / DB25 pin 1 |
| `wl_oe` | `willem_set_oe_pin` | control bit 1 / DB25 pin 14 |
| `wl_vcc` | `willem_vcc_on/off` | control bit 2 / DB25 pin 16 |
| `wl_we` | `willem_set_we_pin` | control bit 3 / DB25 pin 17 |
| `wl_set_address` | `willem_set_par_addr_pin` | D1 data, D0 clock, 24 bits MSB first |
| `wl_set_data` | `willem_set_par_data_pin` | data register D7..D0 |
| `wl_get_data` | `willem_get_par_data_pin` | D1/D2 latch-clock, ACK input |

Both implementations XOR control-register writes with `0Bh`, accounting for
the PC parallel port's electrically inverted control pins 1, 14, and 17. Both
read ACK as active-low and assemble the byte from D7 through D0.

Geepro initializes `addr_init_mask` to `800000h`, so every address update
clocks the board's full 24-stage chain even for smaller devices. The physical
PCB5.0E test caught an earlier mistaken `1000h` starting mask: it reproduced
the first 256 ROM bytes across all 32 pages because only A0 through A7 reached
their intended stages.

## 2764 read sequence

Geepro's `chips/27xx.cpp` uses `start_action(0, 1)` for a 2764, then for each
byte sets the address, selects OE, reads the serial data chain, and deselects
OE. `wl_begin_2764_read` and `wl_read_byte` reproduce those transitions. Both
finish with VPP off, VCC off, address zero, and data zero. The DOS port adds
hardware-clocked minimum delays but does not change the signal order.

## AT28C64 read and write

Geepro's
[`chips/28xx.cpp`](https://github.com/danielg4/geepro/blob/a08efcaf6479730d552c5f96bad0a2a01bf0635f/chips/28xx.cpp)
enables 8 KiB M28C64 read, verify, and blank-check through the same address and
readback primitives. Its 2864 write registration is commented out, so this
project does not claim that its AT28C64B write algorithm came from Geepro. The
writer instead follows the Microchip AT28C64B SDP protected-write sequence,
while reusing Geepro's proven Willem signal primitives.

## DIP configuration discrepancy

At the pinned commit, the checked-in runtime `drivers/willem.xml` specifies
`128h` for family `2864_128`, but `drivers/willem.xml.in` contains `12Bh` at
the corresponding location. They are not generated-equivalent: the repository
diff shows this as a real value difference. Physical control-pin measurements
on the PCB5.0E established that `12Bh` routes valid AT28C64 read and write
states, so the utility follows the source template and measured hardware:

| Device | Runtime mask | Switches ON |
|---|---:|---|
| 2764/27C64 | `12Bh` | 1, 2, 4, 6, 9 |
| M28C64/AT28C64 | `12Bh` | 1, 2, 4, 6, 9 |

The same pinned tree's `pixmaps/willem_28pin.png` shows a 28-pin package placed
against the ZIF end opposite the lever, not centered. The two unused physical
contact pairs (socket 1/32 and 2/31) are together at the lever end, and the
package notch faces those empty rows and the lever/pivot.

The physical read gate validates the 2764 path first. AT28C64 writing remains
locked until those reads are repeatable and match the known Juku image.
