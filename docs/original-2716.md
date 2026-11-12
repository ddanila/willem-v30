# Original Willem DOS 2716 comparison — 2026-11-10

The A9 mirror persists when the installed RF2 is accessed using the original
DOS application's address, read-latch and programming-control sequences.
This investigation does **not** identify a missing software command that fixes
it. The strongest remaining lead is the physical A9 path; a voltage measurement
at the chip is needed to distinguish the programmer output from chip decoding.

## Original executable and virtual capture

Downloaded the [vendor's DOS 0.996 package](https://www.mcumall.com/downloads/download.asp?DownloadID=28).
`WILEP996.EXE` itself displays **0.992D, Oct. 23 2001**.
ZIP SHA-256: `af312597c8ddcc1c822c432d960f295454d451748bbcaf2f434110ce7bcfa15d`.
EXE SHA-256: `6fbd7bd1fc5e8149fb0342c83b5180caf160d1fb52b17d5af517ec718f1888c3`.
The original executable was not modified. Proprietary binaries remain local in
`dosravi/sessions/original-willem-dos-20261110/`, not in this repository.

DOSBox-X routes LPT1 through `tests/willem_ppdev.c` to a regular-file virtual
chip. No original-software emulator process has a physical LPT endpoint.
`WL_RAW_LOG` records every ppdev data/control/status operation and host timestamp.
`WL_SAMPLE_ORACLE=1` supplies successive bits of the selected virtual chip byte
at status reads. This lets the original execute its verification/retry logic;
it is deliberately **not an electrical 4014/4021 latch model**. The default
model remains available for existing driver tests. Address-register outputs
are now modeled as live shift-register outputs, rather than incorrectly
waiting for an OE transition before updating the socket address.

Selected Device > 27xxx > **2716 PCB3**, then File > LoadBin > PATTERN.BIN,
then Action > Program/Test SRAM. Repeated with **2716**. Both profiles sent
seven pulses for the seven non-FF input bytes, verified each, and completed a
full 2048-byte verification matching the input. The sparse input deliberately
uses 0000, 0001, 0040, 0200, 0201, 0400 and 07FF. The two profile names did not
produce an A9 remapping in these traces.

The original's per-byte sequence (raw PC control-register values) is:

1. Shift 11 address bits using D0 clock and D1 serial data, with D2 high.
2. CONTROL `0C`: VCC/VPP on, CE low, OE low.
3. CONTROL `0E`: disable chip outputs, retain VCC/VPP and CE low.
4. Write the data byte to the data port.
5. CONTROL `06`, then `0E`: positive CE programming pulse.
6. Data `00`; CONTROL `0F`, then `0D`: programming VPP off, enable outputs.
7. Data `06,02,00,04`, then eight status samples separated by `00,04` clocks.
8. After all bytes, perform a separate full verification at CONTROL `0D`.

Thus the original agrees with our positive pulse polarity and directly shifts
A9; it does not hold CE high while reading. Unlike the original DOS program,
our original writer verifies bytes while VPP remains enabled. The bounded
comparison helper follows the original's VPP-off verification sequence.

The GUI displays a 55 ms pulse, but host-timestamp pulse intervals in this
3000-cycle DOSBox run were about 41–43 ms. These are emulator observations,
not physical qualification of the vendor's timing. Physical experiments used
our independently tested PIT2 50 ms pulse, not a copied CPU-speed delay loop.

## Physical experiments on the installed RF2

The chip already contained C3 readable at 0000 and 0200 after the prior
single-byte trial. The owner explicitly authorized further unattended experiments
on this installed chip. No chip or jumper was moved during this investigation.

`ORG2716.COM`, built from `tools/original_2716_lab.c`, reproduces the captured
address/read and write-control sequences with conservative software settling
pauses. It is an independent bounded diagnostic, **not the original executable
running on the Pocket8086**, and is not an exact timing replay. All operations
switch VPP off before VCC and save read files only after shutdown.

| Experiment | Result |
|---|---|
| Original-style read, 11 address clocks, normal read VPP | C3 at 0000 and 0200 only; exactly matches the old reader |
| Same read with programming VPP enabled and CE held low | Same complete 2048-byte image; no programming pulse |
| Original-style programming of FF cell 0201 with A5 | One 50 ms pulse; immediate VPP-off verification reads A5 |
| Fresh-power normal read following that pulse | C3 at 0000/0200 and A5 at 0001/0201; all other bytes FF |
| Full 24-clock addresses with the original latch sequence | Identical to the preceding 11-clock read |
| Final independent conservative RD2716 read | Same complete image as both original-style reads |

Final diagnostic image SHA-256:
`8079a4bd4408ef26a5ac1faf7bdfce143fdadd2b7bf1214ae480b5ad153ea466`.
Only **one additional programming pulse** was sent in this investigation.
The current chip is not blank and does not contain a bootable Robotron image.
All six physical sessions exited zero and reported both rails off.

Raising VPP for verification did not remove the mirror. That does not make the
previous 4.30 V normal read level acceptable or rule out every supply transient.
The measured 24.7 V programming-VPP hold was with CE inactive; it was not an
oscilloscope measurement during a programming pulse.

## Remaining hardware lead and next discriminating measurement

The [PCB5.0 schematic and its author's modifications](https://jopl.eu/willeprog_adapters.php)
show A9 reaching ZIF26 through D9, with R22 (2.2 kohm) pulling the socket-side
node down and D8 providing another path from the identification-voltage circuit.
The author explicitly documents a D9 bypass for excessive voltage drop during
3.3 V operation. This is evidence of a special A9 path, **not proof that the
same modification is required for this 5 V board**. Earlier owner photos show
CD4015-family address registers and the corresponding diode/resistor area;
photos cannot establish the present pin voltage or continuity.

The next useful bench check is a loaded voltage comparison with VPP programming
off: set address 0000, then 0200, and measure **ZIF26 / RF2 pin22** against
**ZIF16 / RF2 pin12**. Compare the high level before and after D9 if necessary.
The [RF2 data sheet](https://static.chipdip.ru/lib/818/DOC054818967.pdf) specifies
an address high of at least 2.4 V and low at most 0.4 V. A weak CD4015 output,
diode/drop/loading or continuity fault could explain a chip-dependent A9
failure. A valid measured A9 transition would shift suspicion toward the chip
or other electrical conditions. Do not bypass diodes or change pull-downs
blindly: that circuit also participates in identification-voltage routing.

No unattended software test can measure this analog node through the existing
LPT interface. Repeating full-image burns would not separate these causes.

## Reproduction and evidence

Evidence is under `programming/serial-boot-20261110/evidence/original-2716/`:
compressed raw traces, decoded summaries, selected-profile screenshots, sparse
input, physical execution IDs/stdout/hashes and the final diagnostic capture.
The settings screenshots are from old original software, not new instructions
for the owner's PCB5.0E clone.

Build and check the diagnostic:

```
make build/dos/ORG2716.COM
python3 tests/test_original_2716_lab.py
python3 tools/check_original_2716_trace.py \
  programming/serial-boot-20261110/evidence/original-2716/pcb3-clean.log.gz \
  programming/serial-boot-20261110/evidence/original-2716/PATTERN.BIN
```

The same checker accepts `plain-program.log.gz`. It verifies all programmed
addresses/data, all per-byte verification replies, the complete final scan,
read control levels and final rail shutdown. Captured status replies come from
the explicit oracle; successful virtual programming does not qualify a real
chip's voltages or timing.


Original-app launch recipe (private `LAB` directory containing the downloaded
application under `dos/`, a 2048-byte FF `chip.bin`, and a regular empty `port`):

```
cc -shared -fPIC -O2 tests/willem_ppdev.c -o "$LAB/model.so"
cp /usr/bin/dosbox-x "$LAB/dosbox-x"
DISPLAY=:94 SDL_VIDEODRIVER=x11 SDL_AUDIODRIVER=dummy \
LD_PRELOAD="$LAB/model.so" WL_ENDPOINT="$LAB/port" \
WL_IMAGE="$LAB/chip.bin" WL_LOG="$LAB/model.log" \
WL_RAW_LOG="$LAB/raw.log" WL_SAMPLE_ORACLE=1 WL_ADDRESS_BITS=0 \
"$LAB/dosbox-x" -fastlaunch -set 'sdl videodriver=x11' \
  -set 'sdl output=surface' -set 'cpu cycles=3000' \
  -set "parallel parallel1=reallpt realport:$LAB/port" \
  -set 'parallel parallel2=disabled' -set 'parallel parallel3=disabled' \
  -c "mount c $LAB/dos" -c 'c:' -c 'WILEP996.EXE'
```

Start a private Xvfb display first; `:94` was used for this experiment.
Copy PATTERN.BIN into `dos/` before mounting it. Use the menu sequence above,
then quit normally to finish the trace. A regular-file endpoint is mandatory;
the model refuses to initialize its chip emulation for a physical device node.
Do not substitute a real `/dev/parport*` path.

Regression validation: `make port-model-test` passed the healthy full-image
1739-pulse write, A6/A9 one-pulse aborts, the experiment commands, cold-read
probe tests and the 1016-pulse folded-pattern fill. The new original-style
helper tests passed with healthy and injected-A9 models. Both archived original
trace checkers passed, and the final physical read matched the saved capture.

## Filling a partially programmed diagnostic chip

`ORG2716 F IMAGE.BIN B 0` accepts an exact 2048-byte image and the same
`RF2.OK` 5 V/25 V gate. Numeric arguments are hexadecimal: B means 11
address bits; the last argument must be zero. It scans the whole chip before
programming, refusing any existing non-FF byte that differs from the image.
Matching bytes (including FF) are skipped; only FF cells receive one nominal
PIT2 50 ms pulse, with no retry. Each byte is verified after programming VPP
is removed using the original DOS sequence. A power cycle and complete
read-mode verification follow. Failure paths switch both rails off.

This intentionally does not support further programming an already non-FF
byte, even where its remaining one-bits might permit the desired value.
It is a sacrificial-chip diagnostic, not a general adaptive EPROM algorithm.
Do not automatically replay an interrupted run; recover its status and inspect
the chip first. `tests/test_original_2716_fill.py` checks full completion with
preserved bytes, repeat-fill skipping, A6/A9 alias preflight refusal, failure to
program, and invalid image/mode/gate rejection through the DOS port model.
