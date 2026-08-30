#!/usr/bin/env python3
"""Create the M2764A high-voltage gate from reviewed physical evidence."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


SIZE = 8192


def identity(path: Path, data: bytes) -> str:
    return f"{path.name}:{hashlib.sha256(data).hexdigest()}"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--blank-read", type=Path, action="append", default=[])
    parser.add_argument("--control-read", type=Path, required=True)
    parser.add_argument("--chip-marking", required=True)
    parser.add_argument("--vcc", type=float, required=True)
    parser.add_argument("--vpp", type=float, required=True)
    parser.add_argument("--ambient", type=float, required=True)
    parser.add_argument(
        "--allow-marginal-vcc",
        action="store_true",
        help=(
            "explicitly authorize a measured 5.70..<5.75 V experimental VCC; "
            "the out-of-spec value is recorded in the gate"
        ),
    )
    parser.add_argument(
        "--writer-blank-scan-only",
        action="store_true",
        help=(
            "skip separate host blank-read evidence and rely on WM2764A's "
            "mandatory full VPP-off scan immediately before programming"
        ),
    )
    args = parser.parse_args()

    if args.output.name.upper() != "M2764A.OK":
        parser.error("the DOS gate token must be named M2764A.OK")
    args.output.unlink(missing_ok=True)
    if args.writer_blank_scan_only:
        if args.blank_read:
            parser.error(
                "--writer-blank-scan-only cannot be combined with --blank-read"
            )
    elif len(args.blank_read) != 2:
        parser.error(
            "exactly two independent --blank-read files are required unless "
            "--writer-blank-scan-only is selected"
        )
    if "M2764A" not in args.chip_marking.upper():
        parser.error("--chip-marking must explicitly identify an M2764A")
    vcc_in_spec = 5.75 <= args.vcc <= 6.25
    vcc_marginal = 5.70 <= args.vcc < 5.75
    if not vcc_in_spec and not (args.allow_marginal_vcc and vcc_marginal):
        parser.error(
            "measured programming VCC must be within 5.75..6.25 V; "
            "5.70..<5.75 V requires --allow-marginal-vcc"
        )
    if not 12.2 <= args.vpp <= 12.8:
        parser.error("measured programming VPP must be within 12.2..12.8 V")
    if not 20.0 <= args.ambient <= 30.0:
        parser.error("programming ambient must be within 20..30 C")

    blanks = [path.read_bytes() for path in args.blank_read]
    control = args.control_read.read_bytes()
    if any(len(data) != SIZE for data in (*blanks, control)):
        parser.error("every evidence image must be exactly 8192 bytes")
    if blanks and (
        blanks[0] != blanks[1] or any(byte != 0xFF for byte in blanks[0])
    ):
        parser.error("blank reads must independently match and contain only FF")
    if all(byte == 0xFF for byte in control) or all(byte == 0x00 for byte in control):
        parser.error("control read must be nonuniform to reject a stuck bus")

    lines = [
        "WILLEM-M2764A-WRITE-GATE-1",
        f"CHIP={args.chip_marking}",
        f"VCC={args.vcc:.3f}",
        f"VCC_STATUS={'IN_SPEC' if vcc_in_spec else 'MARGINAL_EXPERIMENTAL_USER_ACCEPTED'}",
        f"VPP={args.vpp:.3f}",
        f"AMBIENT_C={args.ambient:.1f}",
        "BLANK_CHECK="
        + (
            "WRITER_VPP_OFF_FULL_SCAN_ONLY"
            if args.writer_blank_scan_only
            else "TWO_HOST_READS_PLUS_WRITER_VPP_OFF_FULL_SCAN"
        ),
        f"HOST_BLANK_READS={len(blanks)}",
        *(f"BLANK{index}={identity(path, data)}" for index, (path, data) in enumerate(zip(args.blank_read, blanks, strict=True), 1)),
        f"CONTROL={identity(args.control_read, control)}",
    ]
    args.output.write_bytes(("\r\n".join(lines) + "\r\n").encode("ascii"))
    blank_source = (
        "the writer's mandatory VPP-off blank scan"
        if args.writer_blank_scan_only
        else "two blank reads plus the writer's mandatory VPP-off scan"
    )
    print(f"created {args.output} from {blank_source} and measured rails")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
