#!/usr/bin/env python3
"""Decode append-only WTRACE.BIN files produced by WILLEM.COM."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

HEADER = struct.Struct("<4s7H H")
RECORD = struct.Struct("<IBBH")
FOOTER = struct.Struct("<4sII")


def decode(path: Path) -> int:
    data = path.read_bytes()
    offset = 0
    run = 0
    while offset < len(data):
        if len(data) - offset < HEADER.size:
            raise ValueError(f"truncated header at byte {offset}")
        magic, year, month, day, hour, minute, second, hundredth, base = HEADER.unpack_from(data, offset)
        if magic != b"WLT1":
            raise ValueError(f"bad run header at byte {offset}: {magic!r}")
        offset += HEADER.size
        run += 1
        print(
            f"# run {run}: {year:04}-{month:02}-{day:02} "
            f"{hour:02}:{minute:02}:{second:02}.{hundredth:02} LPT={base:03X}h"
        )
        records = 0
        while True:
            if len(data) - offset < 4:
                raise ValueError(f"run {run} has no footer")
            if data[offset : offset + 4] == b"WLE1":
                if len(data) - offset < FOOTER.size:
                    raise ValueError(f"truncated footer at byte {offset}")
                _, expected, _ = FOOTER.unpack_from(data, offset)
                offset += FOOTER.size
                if records != expected:
                    raise ValueError(
                        f"run {run}: decoded {records} records, footer says {expected}"
                    )
                print(f"# end run {run}: {records} records")
                break
            if len(data) - offset < RECORD.size:
                raise ValueError(f"truncated record at byte {offset}")
            sequence, operation, register, value = RECORD.unpack_from(data, offset)
            offset += RECORD.size
            if sequence != records:
                raise ValueError(
                    f"run {run}: sequence {sequence}, expected {records} at byte {offset - RECORD.size}"
                )
            op = chr(operation)
            if op == "D":
                print(f"{sequence:08d} DELAY {value} us")
            else:
                print(f"{sequence:08d} {op} LPT+{register} {value & 0xFF:02X}")
            records += 1
    return run


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("trace", type=Path)
    args = parser.parse_args()
    try:
        runs = decode(args.trace)
    except (OSError, ValueError) as error:
        print(f"decode_trace: {error}", file=sys.stderr)
        return 1
    if runs == 0:
        print("decode_trace: empty trace", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
