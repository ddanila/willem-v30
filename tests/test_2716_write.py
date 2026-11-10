"""DOS front-end write guards; DOSBox's unconnected port reads as zero."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
for cpu in ('8086', '80186'):
    for options, gate, size, expected in (
        ('', True, 2048, 'require explicit /WRITE'),
        ('/WRITE', False, 2048, 'RF2.OK required'),
        ('/WRITE /TRACE', True, 2048, '/TRACE is forbidden'),
        ('/WRITE', True, 2049, 'ERROR:'),
        ('/WRITE', True, 2048, 'nonblank=2048'),
    ):
        with tempfile.TemporaryDirectory() as directory:
            work = Path(directory)
            shutil.copy(ROOT/'build/dos/WILLEM.COM', work)
            (work/'IMAGE.BIN').write_bytes(b'\x42'*size)
            if gate: (work/'RF2.OK').write_bytes(b'WILLEM-2716-5V-25V-GATE-1\r\n')
            subprocess.run(['dosbox-x', '-silent', '-fastlaunch',
                '-set', 'dosbox quit warning=false', '-set', 'sdl output=surface',
                '-set', f'cpu cputype={cpu}', '-set', 'cpu cycles=max',
                '-set', 'midi mididevice=none', '-c', f'mount c {work}', '-c', 'c:',
                '-c', f'WILLEM WRF2 IMAGE.BIN 378 {options}', '-c', 'exit'],
                env={**os.environ, 'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'},
                stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, timeout=45)
            log = (work/'WILLEM.LOG').read_text()
            assert expected in log, log
            assert 'programming rails enabled' not in log
            assert 'WRITE PASSED' not in log
            if expected == 'nonblank=2048':
                assert 'Safe shutdown complete' in log and 'WRITE FAILED' in log
            else:
                assert 'Initial raw DATA' not in log
            print(cpu, expected, 'PASS')
