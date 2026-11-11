"""Actual bcc DOS writer through the port model; no physical I/O."""
import os
from test_dos_port_model import run_model, ROOT
os.environ['WL_2732']='1'
work=ROOT/'build/2732a-fixtures';work.mkdir(exist_ok=True)
gate=work/'2732A.OK';gate.write_text('D2732A-5V-21V-50MS\n')
rom=ROOT/'programming/serial-boot-20261110/KAYPRO2.BIN'
files=[ROOT/'build/dos/PG2732A.COM',rom,gate]
expected=rom.read_bytes()
for alias in (0x40,0x200,0x800,0):
 d=run_model('2732a-'+str(alias), ['PG2732A KAYPRO2.BIN /WRITE21 > REPORT.TXT',
    'PG2732A KAYPRO2.BIN /WRITE21 > AGAIN.TXT'],alias,files)
 log=(d/'ports.log').read_text();report=(d/'REPORT.TXT').read_text()
 assert 'errors=0 rails=0' in log,log[-1000:]
 if alias:
  assert 'result=3 pulses=1' in report,report
  assert log.count('PULSE ')==1
 else:
  assert 'WRITE PASSED' in report,report
  assert (d/'chip.bin').read_bytes()==expected
  assert log.count('PULSE ')==sum(v!=255 for v in expected)
 assert 'result=2 pulses=0' in (d/'AGAIN.TXT').read_text()
 print('2732A actual DOS full image / alias / nonblank refusal PASS',alias,flush=True)
os.environ['WL_NO_PROGRAM']='1'
d=run_model('2732a-dead',['PG2732A KAYPRO2.BIN /WRITE21 > REPORT.TXT'],0,files)
assert 'result=3 pulses=1' in (d/'REPORT.TXT').read_text()
assert 'errors=0 rails=0' in (d/'ports.log').read_text()
del os.environ['WL_NO_PROGRAM']
gate.write_text('WRONG\n')
short=work/'SHORT.BIN';short.write_bytes(b'\xff'*4095)
d=run_model('2732a-gates',['PG2732A KAYPRO2.BIN /WRITE21 > BADGATE.TXT',
 'DEL 2732A.OK','PG2732A KAYPRO2.BIN /WRITE21 > NOGATE.TXT'],0,files)
assert 'PULSE ' not in (d/'ports.log').read_text()
gate.write_text('D2732A-5V-21V-50MS\n')
d=run_model('2732a-size',['PG2732A SHORT.BIN /WRITE21 > SHORT.TXT'],0,files+[short])
assert 'PULSE ' not in (d/'ports.log').read_text()
print('2732A failed programming, missing/wrong gate, wrong image size PASS',flush=True)
