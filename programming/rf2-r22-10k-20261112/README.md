# RF2 A9 test after R22 replacement — 2026-11-12

The owner replaced R22 from 2.2 kohm to 10 kohm and requested a programming
test of A9. Standard 2716 routing: J1/J2 ZIF-side, DIP 1A3, J9/J10 toward
DB25, chip pin1 at ZIF5. Owner reported programming VPP good before inserting
this RF2. Exact marking and identity relative to earlier specimens are unknown.

Two initial conservative reads returned all 2048 bytes FF. These alone could
not test address decoding. The existing bounded original-sequence helper
passed its emulator checks for healthy and injected A9-alias models before use.

1. One nominal PIT2 50 ms pulse wrote A5 at 0200. Immediate verification passed.
2. A fresh-power full read found A5 only at 0200; every other byte was FF.
3. One nominal PIT2 50 ms pulse wrote 5A at 0000. Immediate verification passed.
4. Fresh full reads using both ORG2716 and the separate conservative RRF5
   reader found exactly 0000=5A and 0200=A5, with all other bytes FF.

All operations reported safe rail shutdown. No retries or further programming
pulses were issued. The chip is now a diagnostic pattern, not blank or a boot ROM.
Raw stdout retains DOS CRLF and its inaccurate target clock; host date is above.
Original transport records remain in DOSRAVI sessions named
`rf2-r22-10k-20261112-*`; initial blank captures remain in rom-programmers session
`rf2-r22-10k-read-20261112`.

**Conclusion:** A9 distinguishes the tested address pair for both programming
and reading with R22=10k on this specimen. This supports the loading hypothesis,
but no same-chip 2.2k/10k A/B comparison or loaded A9 voltage measurement was
performed. It does not establish the cause of earlier A6 mirroring, qualify all
RF2 specimens, or demonstrate a complete boot-ROM burn. No measured pulse
waveform or new read-mode VPP voltage was obtained.
