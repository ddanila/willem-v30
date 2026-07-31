# Real-hardware logging

Every DOS invocation must create logs directly; shell redirection is neither
required nor relied upon.

## Human-readable session log

`WILLEM.LOG` is opened in append mode and is never truncated automatically.
It receives the same operational messages printed on screen. Every invocation
is enclosed in timestamped `BEGIN RUN` and `END RUN` markers, and every log
line carries the DOS date and time, including hundredths of a second when DOS
provides them. Each run records at least:

- utility build/version and startup time;
- requested operation, device type, input/output filename, and LPT base;
- expected DIP mask and a visual numbered ON/OFF diagram;
- initial and final raw data/status/control register values;
- power-state transitions, especially confirmation that VPP remains off for
  all read-only operations;
- progress, byte count, and checksum for reads;
- the first eight mismatches plus the total mismatch count for checks;
- every error and the final safe-shutdown state;
- exit/result code.

The program always attempts safe shutdown and logs it before closing the file.
It flushes the log after errors and before/after every power-state transition,
so evidence should survive even if the operator must reset a stuck machine.

## Full port trace

An optional `WTRACE.BIN` also opens in append mode and records every LPT
transaction without flooding the screen. Each run begins with a versioned
header containing its timestamp and a run identifier. A compact fixed-size
record contains:

- monotonically increasing operation number;
- operation (`OUT`, `IN`, or delay);
- port/register selector;
- byte value or delay duration.

The trace is intended for emulator comparison and fault diagnosis. Text output
for every shift-register edge would be many megabytes and would materially
alter timing on a V30, so the full trace is binary and buffered. A host-side
decoder converts it to readable text:

```sh
python3 tools/decode_trace.py WTRACE.BIN > WTRACE.TXT
```

Session logging remains enabled even when full tracing is disabled. Full trace
mode will be available explicitly (for example `/TRACE`) for the first physical
reads and whenever diagnostics are needed.
