# Willem V30

Small, auditable MS-DOS tools for classic LPT-connected Willem EPROM
programmers, targeting the NEC V20/V30 and 8086 instruction set.

The initial hardware target is a Willem PCB5.0E operating in its PCB3B
compatibility mode. The initial device targets are:

- 2764/27C64: read, blank-check, and verify first
- AT28C64: read, blank-check, program, and verify

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
