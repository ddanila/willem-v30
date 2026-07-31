WILLEM V30 DOS UTILITY
======================

This is an 8086-compatible .COM program for a classic parallel-port Willem
programmer.  It targets a PCB5.0E set to PCB3B compatibility mode.  It runs
under MS-DOS 6.22 and is suitable for the NEC V30.

CURRENT SAFETY STAGE
--------------------

This build can read, blank-check, and verify 2764/27C64 and AT28C64 devices.
It cannot write a chip.  Writing stays disabled until a known Juku 2764 has
been read correctly on the real programmer.

COMMANDS
--------

WILLEM R2764  OUT.BIN [378] [/TRACE]
WILLEM R28C64 OUT.BIN [378] [/TRACE]
WILLEM B2764          [378] [/TRACE]
WILLEM B28C64         [378] [/TRACE]
WILLEM V2764  ROM.BIN [378] [/TRACE]
WILLEM V28C64 ROM.BIN [378] [/TRACE]

The LPT address is hexadecimal and defaults to 378.  A verify image must be
exactly 8192 bytes.  Result 0 means success, 1 means an error, and 2 means a
blank-check or verify mismatch.

LOG FILES
---------

Every run prints messages and appends them to WILLEM.LOG.  Existing logs are
never erased.  /TRACE additionally appends every port operation to WTRACE.BIN.
One 8 KiB trace is several megabytes, so use /TRACE for diagnosis and first
hardware validation, not every routine read.

Read HWSETUP.TXT before connecting or powering real hardware.

Source: https://github.com/ddanila/willem-v30
