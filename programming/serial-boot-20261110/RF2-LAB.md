# КС573РФ2 unattended programming investigation — 2026-11-10

**Finding: the installed chip/programmer combination ignores address bit A9
(offset 0200h). Byte programming is repeatable; the unresolved fault is the
physical A9 path, not a demonstrated random programming failure.**

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
| Bulk varied-pattern fill | 1005 additional single 50 ms pulses; no failed byte or retry; existing 11 markers preserved; 8 distinct FF cells left untouched |
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
Both rails are off; the U2716C35 control chip is now the last installed chip.
