# Experimental read timing profiles

`WILLEM.COM` accepts named, audited read-only timing tables. They do not expose
arbitrary delay values and do not alter write pulses, SDP loads, completion
polling, VPP policy, or write authorization gates.

| Profile | Address settle (us) | OE settle (us) | Input latch (us) | Input clock (us) |
|---|---:|---:|---:|---:|
| `legacy` | 0 | 1 | 1 | 1 |
| `conservative` | 4 | 4 | 4 | 4 |
| `address2` | 2 | 4 | 4 | 4 |
| `oe2` | 2 | 2 | 4 | 4 |
| `latch2` | 2 | 2 | 2 | 4 |
| `balanced` | 2 | 2 | 2 | 2 |
| `address1` | 1 | 2 | 2 | 2 |
| `oe1` | 1 | 1 | 2 | 2 |
| `latch1` | 1 | 1 | 1 | 2 |
| `fast` | 1 | 1 | 1 | 1 |

Power-on stabilization remains fixed across the sweep: 5 ms for 2764/27C64
and 200 ms for AT28C64. Adjacent experimental rows change one dimension only.
The `legacy` row is a compatibility reference and is not part of that adjacent
one-variable sweep.

Address settling is applied once after all 24 cascaded address bits have been
shifted and before the socket data path is selected. Latch timing applies to
the input-chain load sequence; clock timing applies around each sampled bit.

Example:

```dos
WILLEM R2764 READ01.BIN 378 /PROFILE:conservative
```

Every read logs the selected row's exact values and compile-time build ID,
followed by `DOSRAVI_METRIC read_ms=N profile=NAME`. The elapsed time is local
to the DOS machine and uses the PIT-channel-0-driven BIOS clock (55 ms
resolution); serial deployment and artifact retrieval are excluded.

Profile names are deliberately closed. A physical sweep starts at
`conservative`, stops at the first mismatch, repeats `conservative` to detect
setup drift, and then reruns the row immediately slower than the fastest 10/10
row. No table is a production recommendation until those physical trials and
the documented safe-shutdown checks pass.
