"""Execute the bcc-compiled core across all 2048 addresses, without hardware."""
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as name:
    work = Path(name)
    subprocess.run(['bcc', '-0', '-Md', '-ansi', '-O', '-I'+str(ROOT/'include'),
        '-c', str(ROOT/'tests/test_2716_dos_core.c'), '-o', str(work/'test.o')], check=True)
    subprocess.run(['bcc', '-0', '-Md', '-i', '-o', str(work/'CORE.COM'),
        str(work/'test.o'), str(ROOT/'build/dos/willem.o')], check=True)
    for cpu in ('8086', '80186'):
        subprocess.run(['dosbox-x', '-silent', '-fastlaunch',
            '-set', 'dosbox quit warning=false', '-set', 'sdl output=surface',
            '-set', f'cpu cputype={cpu}', '-set', 'cpu cycles=max',
            '-set', 'midi mididevice=none', '-c', f'mount c {work}', '-c', 'c:',
            '-c', 'CORE > RESULT.TXT', '-c', 'exit'],
            env={**os.environ,'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'},
            stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT, check=True, timeout=60)
        result = (work/'RESULT.TXT').read_text()
        assert 'errors=0 pulses=2048' in result, result
        print(cpu, result.strip(), 'PASS')
