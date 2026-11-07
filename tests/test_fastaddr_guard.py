"""DOS CLI rejects accelerated tracing/writing/wrong devices before power-on."""
import tempfile, subprocess, os
from pathlib import Path
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='willem-fast512-guard-') as d:
 p=Path(d);(p/'WILLEM.COM').write_bytes((root/'build/dos/WILLEM.COM').read_bytes())
 args=['dosbox-x','-silent','-fastlaunch','-set','dosbox quit warning=false','-set','sdl output=surface','-set','cpu cputype=8086','-set','cpu cycles=max','-set','midi mididevice=none','-c',f'mount c {d}','-c','c:']
 for c in ['WILLEM R27512 BAD.BIN 378 /FASTADDR /TRACE','WILLEM R2764 BAD2.BIN 378 /FASTADDR','WILLEM W28C64 BAD3.BIN 378 /FASTADDR /WRITE']:
  args+=['-c',c]
 args+=['-c','exit']
 subprocess.run(args,env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy'),stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True,timeout=30)
 s=(p/'WILLEM.LOG').read_text();assert s.count('/FASTADDR requires R27512 without /TRACE')==3;assert not list(p.glob('*.BIN'));assert 'Power transition' not in s
 print('FASTADDR trace/wrong-device/write rejection passed before power-on')
