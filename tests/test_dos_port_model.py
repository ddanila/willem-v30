"""Run the actual DOS front end through DOSBox LPT and a virtual Willem socket."""
import os, shutil, subprocess, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
WORK=ROOT/'build/port-model'
WORK.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-shared','-fPIC','-O2','-Wall','-Wextra','-Werror',str(ROOT/'tests/willem_ppdev.c'),'-o',str(ROOT/'build/willem_ppdev.so')],check=True)
shutil.copy(shutil.which('dosbox-x'),WORK/'dosbox-x')
def run_model(name, commands, mask=0, extra_files=()):
 d=WORK/name;d.mkdir(exist_ok=True)
 (d/'port').touch();(d/'chip.bin').write_bytes(b'\xff'*(4096 if os.environ.get('WL_2732') else 2048))
 shutil.copy(ROOT/'build/dos/WILLEM.COM',d)
 if (ROOT/'build/dos/RF2EXP.COM').exists(): shutil.copy(ROOT/'build/dos/RF2EXP.COM',d)
 shutil.copy(ROOT/'programming/serial-boot-20261110/R1715MW2.BIN',d)
 for src in extra_files: shutil.copy(src,d)
 (d/'RF2.OK').write_text('WILLEM-2716-5V-25V-GATE-1\n')
 env={**os.environ,'LD_PRELOAD':str(ROOT/'build/willem_ppdev.so'),
      'WL_ENDPOINT':str(d/'port'),'WL_IMAGE':str(d/'chip.bin'),
      'WL_LOG':str(d/'ports.log'),'WL_ALIAS':str(mask),
      'SDL_VIDEODRIVER':'dummy','SDL_AUDIODRIVER':'dummy'}
 cmd=[str(WORK/'dosbox-x'),'-silent','-fastlaunch']
 for option in ('dosbox quit warning=false','cpu cputype=8086','cpu cycles=max',
   f'parallel parallel1=reallpt realport:{d/"port"}','parallel parallel2=disabled','parallel parallel3=disabled'):
  cmd+=['-set',option]
 cmd+=['-c',f'mount c {d}','-c','c:']
 for command in commands: cmd+=['-c',command]
 cmd+=['-c','exit']
 subprocess.run(cmd,env=env,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL,check=True,timeout=240)
 return d

def main():
 reports=[]
 for mask in (0x40,0x200,0):
  d=run_model(f'alias-{mask:x}', ['WILLEM WRF2 R1715MW2.BIN 378 /WRITE > REPORT.TXT'], mask)
  report=(d/'REPORT.TXT').read_text();trace=(d/'ports.log').read_text();chip=(d/'chip.bin').read_bytes()
  assert 'errors=0 rails=0' in trace,trace[-1000:]
  if mask:
   assert f'probe mismatch at {mask:04X}h'.lower() in report.lower(),report
   assert chip==b'\xc3'+b'\xff'*2047
   assert trace.count('PULSE ')==1
  else:
   assert 'WRITE PASSED' in report,report
   assert chip==(d/'R1715MW2.BIN').read_bytes()
   assert trace.count('PULSE ')==1739
  reports.append({'alias_mask':mask,'pulses':trace.count('PULSE '),'passed':True})
  print(reports[-1],flush=True)
 (WORK/'summary.json').write_text(json.dumps(reports,indent=2)+'\n')

if __name__ == "__main__": main()
