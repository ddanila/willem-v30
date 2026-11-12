# Full 2716 diagnostic pattern

`generate.py` reproduces `A9TEST.BIN` (2048 bytes) using a SHA-256-derived
byte for each address, with 0000=5A and 0200=A5 preserved from the preceding
A9 experiment. `pattern.json` records its hash and address-bit coverage.
This is test data, not executable boot firmware.

The pattern contains all 256 byte values and eight distinct 256-byte pages.
For each address line A0–A10, at least 1016 of 1024 address pairs differing
only in that bit contain different bytes. It therefore detects stuck or
mirrored address lines on complete readback, including the former A6/A9 leads.
There are ten FF bytes; together with the two preserved bytes, twelve cells
need no programming pulse. Expected new pulses: 2036.

The extended original-sequence diagnostic command is:

```
ORG2716 F A9TEST.BIN B 0
```

It requires the existing RF2 5V/25V gate, rejects incompatible nonblank cells
before programming, skips matches, writes FF cells once each, verifies with
programming VPP off, and performs a complete power-cycled verification.
Physical results are recorded separately alongside this document.

## Physical result

The installed RF2 accepted all 2036 new bytes with one pulse per byte and no
retries. Twelve matching bytes were skipped, including the existing 5A/A5.
The writer's power-cycled full verification passed. Two fresh conservative
RRF5 reads then independently matched every byte of A9TEST.BIN, SHA-256
`cb1b834eee6c47e76e5bb2bcff418cc10725c1d59c02947ca5049272491f874a`.
All three operations reported VPP and VCC off on completion.

No A6 or A9 mirroring, or other readback mismatch, appeared in this full-chip
test with R22=10k. This qualifies the recorded pattern on this specimen and
setup, not every RF2 or the analog voltage margins. The chip is now reserved
as a diagnostic reference; changing its zero bits back to ones requires UV
erasure. The two original marker bytes are unchanged.

Software validation passed the original single-byte/read regressions, complete
fill and repeat-fill skipping, A6/A9-alias refusal, nonprogramming-chip abort,
invalid size/mode/gate rejection, and PIT2 pulse checks. The emulator validates
port transactions and sequencing, not analog chip behavior. Pulse lengths are
nominal; no scope measurement was made during the physical run.
