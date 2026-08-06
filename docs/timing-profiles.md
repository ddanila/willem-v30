# Experimental read timing profiles

`WILLEM.COM` accepts named, audited read-only timing tables. They do not expose
arbitrary delay values and do not alter write pulses, SDP loads, completion
polling, VPP policy, or write authorization gates.

| Profile | Address settle (us) | OE settle (us) | Input latch (us) | Input clock (us) | 2764 power (ms) | 28C64 power (ms) |
|---|---:|---:|---:|---:|---:|---:|
| `legacy` | 0 | 1 | 1 | 1 | 5 | 200 |
| `conservative` | 4 | 4 | 4 | 4 | 5 | 200 |
| `address2` | 2 | 4 | 4 | 4 | 5 | 200 |
| `oe2` | 2 | 2 | 4 | 4 | 5 | 200 |
| `latch2` | 2 | 2 | 2 | 4 | 5 | 200 |
| `balanced` | 2 | 2 | 2 | 2 | 5 | 200 |
| `address1` | 1 | 2 | 2 | 2 | 5 | 200 |
| `oe1` | 1 | 1 | 2 | 2 | 5 | 200 |
| `latch1` | 1 | 1 | 1 | 2 | 5 | 200 |
| `fast` | 1 | 1 | 1 | 1 | 5 | 200 |
| `powerfast` | 1 | 1 | 1 | 1 | 4 | 150 |

Adjacent experimental rows change one dimension only. The final `powerfast`
row is the sole power-stabilization reduction and remains read-only. The
`legacy` row is a compatibility reference and is not part of the adjacent
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
