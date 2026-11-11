# Kaypro V3 3.0.0 — AMD AM2732A-DC

`KAYPROV3.BIN` is the exact 4096-byte emulator-qualified Kaypro V3 3.0.0
ROM from vintage inventory, without duplication or padding. Source revision:
`164f4973fb0bfec1ecc40b4e40db434f490d66b5` in `ddanila/cpm-plus-kaypro`.
SHA-256: `b101752648933fb78918d2131ca6e15932719badce20ac1469be71aad59ec408`.

V3 adds local serial settings and diagnostics, keyboard testing, CRC/nonce
discovery and checked serial-setting handoff to CP/M. Default framing is
19200 8O1. It requires the matched V3 platform, disk and launcher described
in `evidence/source-manifest.json` and the
[candidate release](https://github.com/ddanila/cpm-plus-kaypro/releases/tag/rom-v3.0.0-candidate.1).
The prior chip labelled KP2NETv1 contains the older firmware.

The owner installed an AMD AM2732A-DC and authorized programming if its VPP
requirement is 21 V. The AMD 1984 memory data book, printed pages 6-22/6-23,
specifies 21 V and one 50 ms active-low CE pulse, matching our unchanged
PG2732A.COM. Its historical D2732A gate/name is retained; the actual AMD
marking is recorded here. The prior measured 21 V setup was retained; no new
loaded rail measurement is claimed.

[AMD data book](https://archive.decromancer.ca/bitsavers.org/components/amd/_dataBooks/1984_AMD_Bipolar_MOS_Memories_Data_Book.pdf).

Initial independent read: 4096 FF bytes; execution 3128897123 exited 0 and
shut down both rails. The blank binary remains in local DOSRAVI sessions.

## Completed programming

The writer passed its full blank check and first-byte alias scan, programmed
all 3618 non-FF bytes with one 50 ms pulse each, and verified all 4096 bytes
after power-cycling. Execution 4072183558 exited zero.

A separate RD2732.COM read, execution 303614877, also exited zero and matched
the complete V3 image byte-for-byte. **VPP and VCC are off.** The physical
chip label has not yet been reported. Installation and testing in PC003 remain
pending; this result does not establish a fix for the prior cold-start issue.
