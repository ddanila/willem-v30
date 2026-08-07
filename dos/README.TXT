WILLEM V30 DOS UTILITY
======================

VERSION 0.1.0-dev

This is an 8086-compatible .COM program for a classic parallel-port Willem
programmer.  It targets a PCB5.0E set to PCB3B compatibility mode.  It runs
under MS-DOS 6.22 and is suitable for the NEC V30.

This build clocks the complete 24-bit Willem address-register chain, including
when reading an 8 KiB 2764 or 28C64.

CURRENT SAFETY STAGE
--------------------

This build has a dedicated read-only K573RF5/2716 path.  It can also read,
blank-check, and verify 2764/27C64 and AT28C64 devices.
AT28C64 writing is locked until a known Juku 2764 has been read correctly on
the real programmer and the host validator has created WRITE.OK.

COMMANDS
--------

WILLEM RRF5   OUT.BIN [378] [/PROFILE:name] [/TRACE]
WILLEM R2764  OUT.BIN [378] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]
WILLEM W28C64 ROM.BIN [378] /WRITE [/TRACE]
WILLEM D28C64         [378] [/TRACE]

The LPT address is hexadecimal and defaults to 378.  RRF5 reads exactly 2048
bytes and has no blank, verify, or write command.  A verify image for the other
devices must be exactly 8192 bytes.  Result 0 means success, 1 means an error,
and 2 means a blank-check or verify mismatch.

The write command requires both /WRITE and WRITE.OK in the current directory.
The normal kit deliberately has no WRITE.OK.  On the modern host, create it
only after two independent known-ROM reads pass:

  python3 tools/validate_read.py --unlock WRITE.OK EXPECTED READ1 READ2

Copy that token to the DOS disk.  The writer skips matching bytes and uses the
AT28C64B manufacturer's SDP protected-write sequence for every changed byte:
1555/AA, 0AAA/55, 1555/A0, then address/data.  It waits beyond the specified
10 ms maximum write cycle, checks each byte, verifies all 8192 bytes, and never
enables VPP.  /TRACE is deliberately rejected for writes because trace file
overhead could violate the sequence's 150 us maximum byte-load interval.
The DOS build emits each four-load sequence through a compact direct-LPT
assembly routine specifically to meet that limit on the V30.
Hardware interrupts are masked only during each four-load command burst and
the previous interrupt state is restored immediately afterward.  A failed
byte gets one late read check and at most three total programming attempts.
The log reports retry bytes, total retries, and late completions; repeated
retries identify a marginal used EEPROM even when final verification passes.

D28C64 is a non-writing socket diagnostic.  REMOVE THE CHIP before running it.
It pauses while holding the 28C64 control pins in measurable states.  Follow
the on-screen instructions and record AT28C64 pin voltages.  With the chip in
its normal bottom-aligned position, the corresponding 32-pin ZIF contacts are:
VCC 28=30, /WE 27=29, /OE 22=24, /CE 20=22, and GND 14=16.  Use DIP 12Bh:
ON 1,2,4,6,9.

LOG FILES
---------

Every run prints messages and appends them to WILLEM.LOG.  Existing logs are
never erased.  /TRACE additionally appends every port operation to WTRACE.BIN.
One 8 KiB trace is several megabytes, so use /TRACE for diagnosis and first
hardware validation, not every routine read.

Read HWSETUP.TXT before connecting or powering real hardware.

Source: https://github.com/ddanila/willem-v30
