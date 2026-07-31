WILLEM V30 DOS UTILITY
======================

VERSION 0.0.2

This is an 8086-compatible .COM program for a classic parallel-port Willem
programmer.  It targets a PCB5.0E set to PCB3B compatibility mode.  It runs
under MS-DOS 6.22 and is suitable for the NEC V30.

This build clocks the complete 24-bit Willem address-register chain, including
when reading an 8 KiB 2764 or 28C64.

CURRENT SAFETY STAGE
--------------------

This build can read, blank-check, and verify 2764/27C64 and AT28C64 devices.
AT28C64 writing is locked until a known Juku 2764 has been read correctly on
the real programmer and the host validator has created WRITE.OK.

COMMANDS
--------

WILLEM R2764  OUT.BIN [378] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]
WILLEM W28C64 ROM.BIN [378] /WRITE [/TRACE]

The LPT address is hexadecimal and defaults to 378.  A verify image must be
exactly 8192 bytes.  Result 0 means success, 1 means an error, and 2 means a
blank-check or verify mismatch.

The write command requires both /WRITE and WRITE.OK in the current directory.
The normal kit deliberately has no WRITE.OK.  On the modern host, create it
only after two independent known-ROM reads pass:

  python3 tools/validate_read.py --unlock WRITE.OK EXPECTED READ1 READ2

Copy that token to the DOS disk.  The writer skips matching bytes, polls each
changed byte, verifies all 8192 bytes, and never enables VPP.

LOG FILES
---------

Every run prints messages and appends them to WILLEM.LOG.  Existing logs are
never erased.  /TRACE additionally appends every port operation to WTRACE.BIN.
One 8 KiB trace is several megabytes, so use /TRACE for diagnosis and first
hardware validation, not every routine read.

Read HWSETUP.TXT before connecting or powering real hardware.

Source: https://github.com/ddanila/willem-v30
