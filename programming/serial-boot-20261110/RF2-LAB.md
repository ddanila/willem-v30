# КС573РФ2 unattended programming investigation — 2026-11-10

**Finding: the installed chip/programmer combination ignores address bit A9
(offset 0200h). Byte programming is repeatable; the unresolved fault is the
cause of the A9 alias remains unresolved. Later measurements below expose
read-supply and timing concerns, so this is not proof of a defective chip.**

The owner explicitly authorized destructive experiments on the current installed
КС573РФ2, using cells still containing FF. All experiments ended with VPP and VCC
off. This chip now contains diagnostic data, not a Robotron boot ROM.

## Physical results

| Experiment | Result |
|---|---|
| One-byte preflight | A single C3 pulse at 0000 also reads C3 at 0200; all other bytes FF |
| Linear, reverse and permuted scans | Identical complete readbacks |
| 0, 20 and 100 µs delay after each C port write | No change to the alias |
| FE at 0001; FD at 0202; FB at 0004 | Each appears at both addresses differing by 0200, including writes originating with A9 high |
| Distinct markers for every other address bit | All ten other address bits distinguish locations; only A9 mirrors |
| Byte verification with programming VPP on and off | Identical values for every marker |
| CE disabled, VPP off | 1024 bytes 06, 1023 bytes 04, one byte 00: programmer bus echo, not ROM data |
| Bulk varied-pattern fill | 1005 additional single nominal 50 ms pulses (duration questioned below); no failed byte or retry; existing 11 markers preserved; 8 distinct FF cells left untouched |
| Full post-fill verification | Exact expected 2048-byte mirrored image |
| Independent old RD2716 reader | Exact match after a fresh power cycle |
| Separate 8086 assembly address shifter | Exact match, despite much faster address clocks |
| 100 µs paced, permuted post-fill reader | Exact match |

The final image contains 240 different byte values. For each of A0–A8 and A10,
all 2048 address pairs differing by that bit contain different test values.
For A9, all 2048 pairs are identical. Thus the observed address space has
1024 distinct locations, mirrored across 0200h, rather than 2048 independent
locations. This is a measurement of the installed setup, not a claim that a
КС573РФ2 is specified as a 1 KiB device.

Canonical final diagnostic image: `evidence/RF2LAB.BIN` (2048 bytes), SHA-256:

```
eb8d23f22bd1a15359ad3030b35895857ee58904678b24ded1adb2fa36c06669
```

The CE-disabled experiment reproduces the earlier suspicious RF5 read pattern.
It demonstrates that such a capture can mean the ROM is not driving the bus;
it does not by itself establish a defective chip. The emulator's disabled-bus
model returns FF and does not model this analogue/floating-bus echo.

## Software and emulator checks

The actual DOS COM writer was run under DOSBox-X with an intercepted ppdev LPT
port and a virtual Willem/EPROM model (regular-file backing, no hardware).

- Healthy model: all 1739 non-FF Robotron bytes programmed, full image exact;
  no rail, address-clock, or data-during-pulse errors.
- Injected A6 or A9 alias: new preflight stopped after exactly one pulse.
- Actual DOS experiment utility: marker writes, nonblank refusal, full reads,
  assembly reads, CE-disabled reads, and safe shutdown pass on healthy/aliased models.
- Full folded-pattern fill: passes on an A9-aliased model with exactly the
  expected number of pulses and an exact full readback.
- Actual bcc-compiled core: all 2048 addresses tested on 8086 and 80186;
  alias preflight accepts an isolated byte and rejects an A6 mirror.
- Fixed pulse assembly: Unicorn checks positive polarity, PIT wrap, interrupt
  state and registers. Existing full DOS regression matrix also passes.

The first attempt's missing 32-bit K&R address cast was a real software bug,
fixed in v2 and covered by a regression that rejects the faulty call. It is
separate from the subsequent A6/A9 alias observations. The model tests cannot
prove physical pin voltages or electrical loading.

## What remains electrically unresolved

A9 is **chip pin 22 / ZIF contact 26**, using zero-based address numbering.
The [RF2 family data sheet](https://static.chipdip.ru/lib/818/DOC054818967.pdf)
uses one-based address labels and calls this same pin **A10**. It specifies
КС573РФ2 as 2K × 8.

The unattended tests cannot distinguish a failed chip input/decoder from a
programmer output that is not reaching a valid logic level under this chip's
load. Visual contact inspection alone does not measure that signal. Earlier
chips showed different behaviour: the first RF2 distinguished A9 and A6,
while the second candidate aliased A6. That weighs against a permanently stuck
shared programmer output, but does not prove which component is defective.

A bounded, read-voltage-only meter helper is ready in `RF2EXP.COM`:

```
RF2EXP A 0000 1E 0
RF2EXP A 0200 1E 0
```

All numeric arguments are hexadecimal; each command holds its address for
about 30 seconds with 5 V VCC, programming VPP off and ROM output disabled,
then switches both rails off. These hold commands have been emulated but were
**not run on hardware unattended**, because nobody was taking the measurement.
With black probe on ZIF16, measure chip22/ZIF26: first should be low, second
high. If levels are correct at the chip, its input/decoder is suspect. If not,
compare the same test with the chip removed to separate input loading from the
programmer path. No additional programming pulse can substitute for that check.

## Reproduction and evidence

`make experiment` builds the bounded diagnostic utility. It supports:

- `R file pace order`: read only; order 0 linear, 1 reverse, 2 permuted,
  3 CE disabled, 4 assembly address shifter.
- `W address value pace`: one pulse only if the byte is FF; no retry.
- `F image pace 200`: experimental A9-folded fill; mirror-consistent 2048-byte
  image required, existing bytes must match or be FF; no programming of
  already matching bytes. This is not a normal boot-ROM writer.
- `A address seconds pace`: bounded meter hold, no programming VPP.

`make port-model-test` runs the full DOS LPT model and diagnostic tests.
`make dos-test` covers the CLI/core regressions; `make pulse-test` requires
Unicorn. Local tests used `/tmp/willem-fast512-venv/bin/python` for Unicorn.

`evidence/rf2-lab-sessions.json` records the physical execution IDs, stdout and
file hashes. Full serial logs remain in `/home/ddanila/fun/dosravi/sessions/`.
The first experimental reader reported a file-close error despite producing a
complete matching image; following the established dev86 close convention fixed
that reporting issue. Later read sessions exited zero and matched it exactly.
The CE-disabled echo binary remains local, not in the firmware collection.

## Later control: another U2716C35

The owner subsequently replaced the diagnostic chip with a loose U2716C35,
possibly from Robotron (unconfirmed). The old RD2716 reader and the separate
assembly shifter produced identical 2048-byte nonblank dumps, SHA-256
`b97cfb4b68b56690c27730daaa347a8780579c8aa75d0169979cc63e33044b34`.
A6 pairs differ in 1508/2048 comparisons and A9 pairs in 2032/2048;
all eleven address bits distinguish data. No programming pulses were sent.

This control strengthens the chip-dependent input/decode or loading explanation
and weighs against a permanently stuck shared programmer output. It still does
not measure the failed RF2's pin voltage. The control ROM was compared against
134 distinct references, including known Robotron keyboards and system ROMs;
no exact or embedded match was found. Its acquisition and comparison evidence
are preserved in [ddanila/dumps](https://github.com/ddanila/dumps/tree/main/unknown/u2716c35-20261110).
Both rails were switched off after that control read. The owner subsequently
installed three additional RF5 chips for read-only checks. The owner later
clarified that a blank RF2, not RF5 #3, was installed for the measurements below.


## Loaded supply measurement and timing concern

With a blank RF2 installed (owner correction after the initial report), the owner measured **VCC 4.99 V at ZIF28** and
**read-mode VPP 4.30 V at ZIF25**, both relative to ground ZIF16.
This is the same chip type as the earlier programming candidates; the owner
has not identified it as one particular earlier physical specimen. Session
directory names retain the original, mistaken RF5 attribution.
The approximately 0.69 V difference is consistent with a diode drop, but its
physical cause has not been established. The [RF2 specification](https://static.chipdip.ru/lib/818/DOC054818967.pdf)
requires 4.75–5.25 V for the read level on pin 21. This observation therefore
requires resolving the read supply before treating RF2 verification as qualified;
it does not by itself explain the A9 alias or invalidate the nonblank control dump.

Two `RF2EXP.COM A 0000 3C 0` read-voltage holds completed with exit zero and
`SAFE OFF: VPP off, VCC off`:

| Local DOSRAVI session | Execution ID | Reported elapsed time |
|---|---:|---:|
| rf5-vcc-measure-20261110-1 | 1167612904 | 31625 ms |
| rf5-vcc-measure-20261110-2 | 836151594 | 31570 ms |

The first hold produced a 5.00 V VCC measurement; the subsequent zero-VPP
report came after shutdown and is not a powered measurement. The second hold
produced the simultaneous 4.99/4.30 V readings above. Neither command enabled
programming VPP or sent programming pulses. Both rails were off at completion.

The nominal 60-second hold lasting about 31.6 seconds exposes a timing concern.
`dos_wait_us` treats raw PIT channel-0 decrements as 1.193182 MHz clock ticks.
In PIT mode 3 the counter decrements by two per input clock, per the Intel 8253
datasheet. For that mode, 1200 waits of 62500 raw decrements predict about
31.43 seconds plus overhead, consistent with the measured holds. The 2716 pulse
routine likewise waits 59659 raw decrements: in mode 3 this is approximately
25 ms, not its intended 50 ms. The existing M2764A pulse routine also uses raw
counter differences and needs the same audit.

This is a source-level mode-3 timing defect with strong physical timing evidence;
the Pocket8086 timer mode and actual CE pulse width have not been independently
measured. Existing pulse emulation modeled raw decrements as clock ticks, and
the full LPT model checked sequencing/data rather than physical pulse duration.
Those tests do not qualify pulse width. Timing correction and validation remain
pending; no new programming was performed following these findings. The currently installed blank RF2 is being used for rail measurements; no
programming pulses are authorized by the request to measure programming VPP.

### Loaded programming-VPP measurement

On the currently installed blank RF2, the owner measured **VCC 5.0 V and
programming VPP 24.7 V** during the requested no-pulse hold. This is within
RF2's specified programming voltage range. It does not resolve the previously
measured 4.30 V read-mode VPP or the raw-PIT programming pulse timing defect.

`tools/vpphold.asm` builds a dedicated LPT378 PCB3B meter helper. CE/PGM stays
low, OE high, and data FF throughout the powered interval. VCC settles before
VPP is enabled; shutdown disables VPP before VCC. A BIOS-tick hold avoids the
known raw-PIT timing issue. `tests/test_vpphold.py` executes the actual binary
in Unicorn, checking rail order, inactive CE, hold duration and tick wrap.

Physical run: `rf2-vpp-measure-20261110-1`, execution 3307434228, exit zero,
30250 ms. The helper reported VPP/VCC off and no programming pulses. Binary
SHA-256: `e7537251a7b6b982e0cec90912a38d2159493c4a97dc961e7e2ac0876c3ce146`.
This was a rail measurement, not a programming or blank-verification operation.

### Corrected 2716 timing and new-chip cold-read experiment

The owner authorized a new blank RF2: first verify all FF, then program the
Robotron image's first byte C3 at 0000, switch both rails off, and read in a
separate operation before deciding whether to continue.

The 2716 pulse now uses **PIT2 mode 0**, explicitly loaded with 59659 input
clocks (approximately 50 ms). This avoids interpreting BIOS PIT0 mode-3 raw
counts as input clocks. CE rises before the timer gate opens and falls after
terminal count. IRQ flags and port61 are restored. The helper owns the speaker
timer and leaves PIT2 in mode 0; it does not reprogram PIT0. The old general
`dos_wait_us` and M2764A pulse paths still need separate correction; this change
qualifies the 2716 pulse, not every delay in the program.

`T2PROBE.COM` exercises the exact timer path 100 times without LPT access.
The host-clock qualification took 5.7876 s including protocol/output download,
consistent with 100 × 50 ms waits. DOSRAVI's target-reported time was only
2915 ms; it must not be used as wall-clock pulse qualification while these
repeated waits mask interrupts. This is aggregate software timing evidence,
not an oscilloscope measurement of the physical CE waveform.

Unicorn executes the actual assembly with a clocked mode-0 PIT2 model, checking
pulse polarity/duration, gate sequencing, register/IRQ preservation and the
no-LPT entry. The actual bcc-built diagnostic was tested in the DOSBox LPT model
with healthy, A6-aliased and A9-aliased chips. `P` performs a full blank scan,
sends exactly one pulse, switches rails off, and does no immediate readback.
A second attempt on the now-nonblank chip is refused. A separate read reveals
any injected aliases. Core C tests and actual DOS 8086/80186 address/gate tests
also passed.

Physical blank read `rf2-new-20261110-blank1` (execution 4244218639) returned
2048/2048 FF with the older conservative reader. The subsequent command
`RF2P50.COM P 0 C3 0` repeated the full blank guard and sent exactly one pulse,
then reported both rails off without immediate verification (execution
995219095). Detailed identities, hashes and stdout are in
`evidence/rf2-pit2-probe.json`.

**Result: A9 mirroring persists.** Separate conservative power-cycle read
`rf2-new-20261110-coldread1` (execution 2383684433, exit zero) returned C3 at
0000 and 0200, with every other byte FF. Diagnostic capture `evidence/RF2P50.BIN`
has SHA-256 `53f876473a44a4fb6d699beb535d42d7eb02b40481a32c0066d4d728e8647390`.
This equals the earlier one-byte A9-mirrored capture, despite a new chip,
corrected pulse timer and no immediate verification. No further pulses were
sent and no full-image write was attempted. Both rails are off.

The old half-length pulse is therefore insufficient to explain the observed
mirror. A9 signal/loading, device decoding and the out-of-spec 4.30 V read-mode
VPP remain unresolved; a software dump does not establish that two physical
cells were programmed. The current chip now has programmed data and must not
be described as blank.
