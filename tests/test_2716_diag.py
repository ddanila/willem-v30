#!/usr/bin/env python3
"""Exercise the real DOS front end with no hardware and inspect rail traces."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
for cpu in ('8086', '80186'):
    for case in range(9):
        count = case if case < 5 else case - 5
        tail = b"" if case < 5 else b"NO\r\n"
        with tempfile.TemporaryDirectory(prefix='d2716-') as name:
            work = Path(name)
            shutil.copy(ROOT / 'build/dos/WILLEM.COM', work)
            (work / 'INPUT.TXT').write_bytes(b'YES\r\n' * count + tail)
            args = ['dosbox-x', '-silent', '-fastlaunch',
                    '-set', 'dosbox quit warning=false',
                    '-set', 'sdl output=surface', '-set', f'cpu cputype={cpu}',
                    '-set', 'cpu cycles=max', '-set', 'midi mididevice=none',
                    '-c', f'mount c {work}', '-c', 'c:',
                    '-c', 'WILLEM D2716 378 /VPP /TRACE < INPUT.TXT', '-c', 'exit']
            subprocess.run(args, env={**os.environ, 'SDL_VIDEODRIVER': 'dummy',
                                     'SDL_AUDIODRIVER': 'dummy'},
                           stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT,
                           check=True, timeout=30)
            data = (work / 'WTRACE.BIN').read_bytes()
            assert data[:4] == b'WLT1' and data[-12:-8] == b'WLE1'
            controls = []
            for off in range(20, len(data)-12, 8):
                seq, op, reg, value = struct.unpack_from('<IBBH', data, off)
                if op == ord('O') and reg == 2:
                    controls.append(value ^ 0x0b)
            rails = []
            for ctl in controls:
                state = ctl & 5
                assert state != 1, 'VPP energized without VCC'
                if not rails or rails[-1] != state:
                    rails.append(state)
            expected = [0] if count == 0 else ([0,4,0] if count == 1 else [0,4,5,4,0])
            assert rails == expected, (cpu, count, rails)
            log = (work / 'WILLEM.LOG').read_text()
            assert 'Safe shutdown complete' in log
            assert ('D2716 aborted' in log) == (count < 4)
            print(cpu, 'confirmations', count, 'rail sequence', rails, 'PASS')
    with tempfile.TemporaryDirectory(prefix='d2716-gate-') as name:
        work = Path(name)
        shutil.copy(ROOT / 'build/dos/WILLEM.COM', work)
        args[-6:] = ['-c', 'c:', '-c', 'WILLEM D2716 378 /TRACE', '-c', 'exit']
        # Replace the prior temporary mount; rejected commands must not open a trace.
        args[args.index('-c') + 1] = f'mount c {work}'
        subprocess.run(args, env={**os.environ, 'SDL_VIDEODRIVER': 'dummy',
                                 'SDL_AUDIODRIVER': 'dummy'},
                       stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT,
                       check=True, timeout=30)
        assert not (work / 'WTRACE.BIN').exists()
        assert 'requires explicit /VPP' in (work / 'WILLEM.LOG').read_text()
        print(cpu, 'missing /VPP rejected before I/O PASS')
