# RF2 / T48 W4 investigation — 2026-11-11

The owner left an RF2 installed in the USB-connected T48 and authorized
unattended diagnosis and programming of Robotron 1715M W4 4.0.0.
The full marking of this specimen and its relationship to earlier Willem
specimens are unconfirmed. No physical label has been assigned here.

## Result

**Successfully programmed and verified.** The controlled one-byte trial and
1869-byte completion run both passed. The completion run took 177.45 seconds.
Two fresh reads (direct reader with pull-downs and custom PROM reader with
pull-ups) match the complete W4 SHA-256 exactly. Pin-driver resets succeeded.
The stock native algorithm still returns all FF after this successful burn.
See `manifest.json` for pulse and internal ADC ranges.

## Finding

The stock MiniPro `M2716@DIP24` algorithm reported FF for every byte, both
before and after a write that failed verification at address zero. That
result was misleading: an independent, slow pin-controlled read found C3
at address zero, exactly the intended first byte. The stock write had
programmed that byte. The remaining 2047 bytes read FF through the working
path. Do not use the stock all-FF read as proof this chip is erased.

The pin-controlled read explicitly supplies VCC=5 V to chip pins 24 and 21
(read VPP), GND to pin 12, and drives CE/pin 18 and OE/pin 20 low. With
outputs enabled, pull-up and pull-down reads agree. With both enables high
and pull-downs selected, all data bits read zero, confirming that the enabled
results are driven by the chip rather than a floating bus.

This localizes the observed false verification to the native T48 2716
read/verify path for this RF2. It does not establish whether its precise
cause is firmware timing, read-mode VPP routing, or another algorithm detail.
A scope measurement or vendor algorithm trace is still needed to separate
those possibilities. It does not establish that all T48/2716 combinations
are affected, or explain the earlier Willem address-mirroring failures.

## Workaround and validation

`rf2-direct.c` uses the T48's existing low-level pin controls instead of the
native 2716 algorithm. It uses the standard 2716 pinout, checks the programmer
serial, reads the entire chip before writing, rejects incompatible programmed
bits, skips correct bytes, and verifies each changed byte after one nominal
50 ms positive CE pulse. It stops on a mismatch without retrying. VCC stays
at nominal 5 V; read VPP is explicitly connected to that supply. Programming
VPP is disconnected before read VPP is reconnected, avoiding a rail short.

The nominal direct VPP setting is 24.37 V from MiniPro's raw DAC table; it was
chosen to keep the actual internal ADC reading within the RF2's 23.5–25.5 V
programming specification. This is not a calibrated chip-pin measurement.
The native `--vpp 25` read setup reported 25.75 V at the internal generator;
that alone does not tell us the voltage at pin 21 during a pulse. Every direct
programming setup checks VCC and VPP against the specification before pulsing.

The first direct trial programmed address 0001 from FF to 06 and verified it;
the host-observed pulse window was 50.142 ms. Exit 3 for this trial means the
requested one-byte limit was reached with the rest of the image still pending.
It is not a verification failure.

`mock-t48.c` is a host-only state-machine double, not an electrical emulator.
It checks pulse sequencing/duration, disallows simultaneous VCC/VPP routing,
and verifies cleanup. Tests cover full programming, a one-byte limit, data
requiring UV erasure, injected verify failure, and out-of-range VPP. The latter
two stop after one and zero pulses respectively. Actual hardware evidence is
recorded separately in the logs and manifest.

## Software fixes and reproducibility

Base: [whaslbeck/minipro](https://github.com/whaslbeck/minipro), commit
`cfe2171ca1b23399558b8d2a603a2ac3fa3faf5d`. This fork supplies the T48 contact
test, which passed. No firmware update or empty-socket self-test was performed.
The connected firmware is 00.1.03 (0x103); MiniPro expects 01.1.39 (0x127).

`minipro.patch` contains the diagnostic profile and logging plus three fixes:

- `-o pulse=...` fell through into an unrelated option chain and was rejected;
  make that chain `else if`. The earlier actual write used `--pulse 50000`,
  so this parsing bug did not cause its false verification.
- Switching to pull-downs releases outputs but failed to invalidate cached
  output levels. Restore the invalidation, as already done for pull-ups.
  The disabled-output control test failed before this fix and passed after it.
- The PROM layer supplied 40-element buffers to T48 functions accessing 56
  entries. Increase those buffers to 56. Diagnostic results retained as
  acceptance evidence were repeated after the buffer fix.

Apply the patch to the exact base, then run `build.sh PATH_TO_MINIPRO` with
libusb development headers available. In this session the headers were
extracted locally from Ubuntu's `libusb-1.0-0-dev`; no system installation was
needed. Run `test-mock.py PATH_TO_RF2_MOCK PATH_TO_W4_BIN` without hardware.

Read-only invocation: `rf2-direct read OUTPUT.bin`.
Programming invocation: `rf2-direct write W4.BIN OUTPUT.bin MAX_CHANGED_BYTES`.
The binary is intentionally restricted to this T48 serial. This is a bench
workaround, not a generally qualified EPROM programmer. The software reset
runs on ordinary errors and handled interrupts; a killed host or unplugged USB
cannot provide a guaranteed hardware pulse timeout. The reported pulse window
is host timing, not an oscilloscope measurement.

## Sources consulted

- [MiniPro source](https://gitlab.com/DavidGriffith/minipro): T48 protocol,
  voltage encoding, programming status handling and custom PROM reads.
- [T48 contact-test fork](https://github.com/whaslbeck/minipro): diode-based
  contact test and low-level pin controls.
- [T48 reverse-engineering notes](https://github.com/danusha2345/xgecu-t48-reverse/blob/main/docs/PROTOCOL.md):
  cross-check of voltage ADC and pin-control commands.
- [Xgpro 13.16 vendor package mirror](https://github.com/Kreeblah/XGecu_Software/tree/master/Xgpro/13):
  local inspection of original `InfoIC2Plus.dll` and bundled T48 manual.
  Intel M2716 records use protocol 0x0b, variant 0, 2048-byte capacity,
  32-byte write blocks, voltage code 0xf1 (25 V), and default pulse 500.
  This confirms database parameters, not a captured runtime USB sequence.
- [RF2 family datasheet](https://static.chipdip.ru/lib/818/DOC054818967.pdf):
  RF2 read/program voltage ranges and pinout.

Original source ROM: inventory `PC001-robotron-1715m/evidence/20261111-w4-candidate/rom-w4.bin`.
It is exactly 2048 bytes and is not doubled for RF2. SHA-256:
`2a60fcc34fd3782696ced9ba7fd64382d991cffa133a6d30e25930e51ae0a50b`.
This programming task does not establish a successful W4 boot on the Robotron.
