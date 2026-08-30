# M2764A physical acceptance

Status: **four physical M2764AF1 devices programmed and independently verified
on the recorded PCB5.0E setup**.

## Setup and scope

The 2026-08-26 run used a 2015 Willem PCB5.0E in its physically established
PCB3B mode, a Pocket8086 LPT host, DIP mask `12Bh` (switches 1, 2, 4, 6, and 9
ON), and a bottom-aligned 28-pin M2764AF1 with two empty ZIF rows at the lever.
The programmer used its 12 V barrel supply with USB programmer power removed.
Ambient temperature was 21 C.

Empty-socket measurements from chip pin 14 / ZIF16 ground were 5.74 V at chip
pin 28 / ZIF30 and 12.66 V at chip pin 1 / ZIF3. The VPP value is inside the
12.2..12.8 V gate. VCC is 10 mV below the data-sheet-derived 5.75 V lower gate,
so every programming token explicitly recorded
`VCC_STATUS=MARGINAL_EXPERIMENTAL_USER_ACCEPTED`. Physical success does not
reclassify 5.74 V as an in-spec or generally recommended setting. Normal-read
verification used the separately selected and previously measured 5.07 V
setting with programming VPP disabled and VPP tied to the read rail.

The images were the standard EktaSoft 3.7 ROM pair:

| Socket | SHA-256 | CRC-32 | Programmed | FF skipped |
| --- | --- | --- | ---: | ---: |
| D15 low | `d6c4ec7418f05e5761ef450e6ee36fb2579d65d9cbf87dce265eaf1c0d077596` | `B26F5080` | 7,890 | 302 |
| D16 high | `35b348ae7c88dc8cb24d1bc9d62a06212fdc2c2f601eddf8e00b233893d92817` | `B184E253` | 6,657 | 1,535 |

## Results

Two D15 and two D16 devices completed the two-stage workflow. Every changed
byte verified after its first 1 ms pulse (`max_initial=1`), received exactly
one `3*n` ms overprogram pulse, and needed no retry. Each program run covered
all 8,192 bytes, completed the full high-voltage comparison, and ended with
VPP and VCC off. The program/high-voltage-verify measurements were:

| Device | Program time | High-voltage verify | Final 5 V trusted read |
| --- | ---: | ---: | ---: |
| D15 copy 1 | 271,590 ms | 112,035 ms | 112,035 ms, exact |
| D16 copy 1 | 229,295 ms | 112,035 ms | 112,035 ms, exact |
| D15 copy 2 | 271,535 ms | 112,035 ms | 111,980 ms, exact |
| D16 copy 2 | 229,350 ms | 112,035 ms | 112,035 ms, exact |

The independent final reads matched the trusted SHA-256 values above and set
`final_5v_verify_passed=true`. A high-voltage comparison alone was never
reported as final success.

## Safety observations

- A D16 candidate with a nonblank byte was rejected by the complete VPP-off
  scan before programming voltage could be enabled.
- A later D16 run was interrupted by loss of the laptop session. It was not
  treated as success. Subsequent scans found residual programmed bytes; the
  writer continued to refuse VPP until a longer second UV erase produced a
  complete all-FF scan.
- The successful retries still required new one-use `M2764A.OK` evidence and
  the separate 5 V trusted read.

This acceptance validates the implemented ST M2764A flow and the recorded
reference setup. It does not cover another manufacturer’s 2764 algorithm,
another board clone, moving selectors while powered, or omitting per-device
blank and final-verification gates.
