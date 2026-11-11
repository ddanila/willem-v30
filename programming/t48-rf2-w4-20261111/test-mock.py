#!/usr/bin/env python3
"""Host-only tests; arguments: mock executable, 2048-byte W4 image."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
exe, image = map(lambda x: str(Path(x).resolve()), sys.argv[1:])
with tempfile.TemporaryDirectory() as tmp:
    for name, env, limit, expected, pulses in [
        ('full', {}, 2048, 0, None),
        ('one', {}, 1, 3, 1),
        ('incompatible', {'MOCK_INCOMPATIBLE': '1'}, 2048, 1, 0),
        ('verify-fail', {'MOCK_VERIFY_FAIL': '1'}, 2048, 1, 1),
        ('overvolt', {'MOCK_OVERVOLT': '1'}, 2048, 1, 0),
    ]:
        output = Path(tmp) / (name + '.bin')
        result = subprocess.run([exe, 'write', image, str(output), str(limit)],
                                env=os.environ | env, capture_output=True, timeout=10)
        assert result.returncode == expected, (name, result.stderr[-1000:])
        assert b'cleanup=OK' in result.stderr
        if pulses is not None:
            assert f'MOCK pulses={pulses} '.encode() in result.stderr
        if name == 'full':
            assert output.read_bytes() == Path(image).read_bytes()
        print(name + ': PASS')
