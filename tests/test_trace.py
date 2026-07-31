#!/usr/bin/env python3
"""Regression tests for the append-only binary trace decoder."""

from __future__ import annotations

import contextlib
import importlib.util
import io
import struct
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "decode_trace", ROOT / "tools" / "decode_trace.py"
)
assert SPEC is not None and SPEC.loader is not None
TRACE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TRACE)


def header(second: int) -> bytes:
    return struct.pack("<4s7H H", b"WLT1", 2026, 7, 31, 23, 59, second, 42, 0x378)


def record(sequence: int, operation: str, register: int, value: int) -> bytes:
    return struct.pack("<IBBH", sequence, ord(operation), register, value)


def footer(count: int) -> bytes:
    return struct.pack("<4sII", b"WLE1", count, 0)


def expect_value_error(path: Path, text: str) -> None:
    try:
        with contextlib.redirect_stdout(io.StringIO()):
            TRACE.decode(path)
    except ValueError as error:
        assert text in str(error), error
    else:
        raise AssertionError("corrupt trace unexpectedly decoded")


def main() -> None:
    first = header(1) + record(0, "O", 0, 0xA5) + footer(1)
    second = header(2) + record(0, "D", 0, 100) + record(1, "I", 1, 0x40)

    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "WTRACE.BIN"
        path.write_bytes(first + second + footer(2))
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            assert TRACE.decode(path) == 2
        text = output.getvalue()
        assert "# run 1:" in text and "# end run 2: 2 records" in text
        assert "00000000 O LPT+0 A5" in text
        assert "00000000 DELAY 100 us" in text
        assert "00000001 I LPT+1 40" in text

        path.write_bytes(first + second + b"xyz12")
        expect_value_error(path, "truncated record")
        output = io.StringIO()
        errors = io.StringIO()
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(errors):
            assert TRACE.decode(path, recover=True) == 2
        assert "DELAY 100 us" in output.getvalue()
        assert "partial record" in errors.getvalue()

        path.write_bytes(first + second)
        errors = io.StringIO()
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(errors):
            assert TRACE.decode(path, recover=True) == 2
        assert "no footer" in errors.getvalue()

        path.write_bytes(first + second + b"WLE1xx")
        errors = io.StringIO()
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(errors):
            assert TRACE.decode(path, recover=True) == 2
        assert "truncated footer" in errors.getvalue()

        bad = header(3) + record(1, "O", 0, 0) + footer(1)
        path.write_bytes(bad)
        expect_value_error(path, "sequence 1, expected 0")

    print("append-only trace decoder tests passed")


if __name__ == "__main__":
    main()
