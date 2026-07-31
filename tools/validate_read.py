#!/usr/bin/env python3
"""Validate two physical Willem reads against one authoritative 8 KiB image."""

import argparse
import hashlib
import pathlib
import sys

ROM_SIZE = 8192


def crc16_ccitt(data):
    crc = 0xFFFF
    for value in data:
        crc ^= value << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def load_image(path):
    data = path.read_bytes()
    if len(data) != ROM_SIZE:
        raise ValueError(f"{path}: expected {ROM_SIZE} bytes, found {len(data)}")
    return data


def describe(label, path, data):
    print(
        f"{label}: {path} bytes={len(data)} "
        f"crc16={crc16_ccitt(data):04X} sha256={hashlib.sha256(data).hexdigest()}"
    )


def differences(left, right, limit=8):
    found = [(offset, a, b) for offset, (a, b) in enumerate(zip(left, right)) if a != b]
    for offset, a, b in found[:limit]:
        print(f"  mismatch {offset:04X}h: left={a:02X} right={b:02X}")
    if len(found) > limit:
        print(f"  ... {len(found) - limit} more mismatches")
    return len(found)


def main():
    parser = argparse.ArgumentParser(
        description="accept two independent physical 2764 reads only if both equal a known image"
    )
    parser.add_argument("expected", type=pathlib.Path, help="authoritative 8192-byte image")
    parser.add_argument("read1", type=pathlib.Path, help="first physical read")
    parser.add_argument("read2", type=pathlib.Path, help="second physical read after reseating/power cycle")
    args = parser.parse_args()

    try:
        expected = load_image(args.expected)
        read1 = load_image(args.read1)
        read2 = load_image(args.read2)
    except (OSError, ValueError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1

    describe("EXPECTED", args.expected, expected)
    describe("READ 1  ", args.read1, read1)
    describe("READ 2  ", args.read2, read2)

    failed = False
    if len(set(read1)) == 1:
        print(f"FAIL: read 1 is a stuck uniform {read1[0]:02X} image")
        failed = True
    if len(set(read2)) == 1:
        print(f"FAIL: read 2 is a stuck uniform {read2[0]:02X} image")
        failed = True

    count = differences(read1, read2)
    if count:
        print(f"FAIL: independent reads differ at {count} byte(s)")
        failed = True
    else:
        print("PASS: independent reads are byte-identical")

    count = differences(expected, read1)
    if count:
        print(f"FAIL: physical read differs from expected image at {count} byte(s)")
        failed = True
    else:
        print("PASS: physical read matches the expected image")

    if failed:
        print("READ GATE: FAILED; AT28C64 hardware writing must remain disabled")
        return 2
    print("READ GATE: PASSED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
