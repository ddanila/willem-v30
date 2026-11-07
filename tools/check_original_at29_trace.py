#!/usr/bin/env python3
"""Check one 0.97ja PCB3 AT29C512 read trace from original_willem_io.c.

The input is a private capture of the unmodified application using synthetic
status replies. This checks host transactions, not socket voltage or timing.
"""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path


def analyze(path):
    opener = gzip.open if path.suffix == '.gz' else open
    data = control = shift = bits = 0
    group = []
    addresses = []
    controls = Counter()
    clocks = Counter()
    events = Counter()
    digest = hashlib.sha256()
    with opener(path, 'rb') as stream:
        for line in stream:
            digest.update(line)
            op, port, value = line.split()
            port, value = int(port, 16), int(value, 16)
            events[f'{op.decode()} {port:04X}'] += 1
            if op == b'W' and port == 0x37a:
                control = value
            if op == b'W' and port == 0x378:
                if not control & 2 and not data & 1 and value & 1:
                    shift = ((shift << 1) | ((value >> 1) & 1)) & 0xffffff
                    bits += 1
                data = value
            if op == b'R' and port == 0x379:
                # Hardware presence samples BUSY with CONTROL 07 and D0 toggled.
                if control == 7:
                    continue
                controls[f'{control:02X}'] += 1
                group.append(shift & 0xffff)
                if len(group) == 8:
                    assert len(set(group)) == 1, 'address changed during sample'
                    addresses.append(group[0])
                    clocks[bits] += 1
                    group.clear()
                    bits = 0
    assert not group
    assert addresses == list(range(65536)), 'not one complete sequential read'
    assert controls == {'05': 65536 * 8}, 'unexpected read control levels'
    assert clocks[16] == 65535, 'unexpected per-byte address shift count'
    return dict(trace_sha256=digest.hexdigest(), bytes=len(addresses),
                address_range='0000-FFFF', sample_control_counts=dict(controls),
                address_clocks_between_groups=dict(clocks), events=dict(events))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace', type=Path)
    print(json.dumps(analyze(parser.parse_args().trace), indent=2))
