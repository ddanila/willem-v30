"""Exercise the actual DOS fill command with real LPT transactions in DOSBox."""
import os
from test_dos_port_model import ROOT, WORK, run_model
os.environ['WL_SAMPLE_ORACLE']='1'
os.environ['WL_ADDRESS_BITS']='0'
rom=ROOT/'programming/rf2-r22-10k-20261112/full-pattern/A9TEST.BIN'
exe=ROOT/'build/dos/ORG2716.COM'
image=rom.read_bytes()
for alias in (0,0x40,0x200):
 d=run_model('original-fill-'+str(alias),[
  'ORG2716 W 0 5A B > FIRST.TXT',
  'ORG2716 W 200 A5 B > SECOND.TXT',
  'ORG2716 F A9TEST.BIN B 0 > FILL.TXT',
  'ORG2716 F A9TEST.BIN B 0 > AGAIN.TXT',
  'ORG2716 R READ.BIN B 0 > READ.TXT'],alias,[exe,rom])
 trace=(d/'ports.log').read_text(); report=(d/'FILL.TXT').read_text()
 assert 'errors=0 rails=0' in trace
 if not alias:
  assert (d/'READ.BIN').read_bytes()==image
  assert trace.count('PULSE ')==2038
  assert 'pulses=2036 skipped=12 result=0' in report
  assert 'pulses=0 skipped=2048 result=0' in (d/'AGAIN.TXT').read_text()
 else:
  assert 'REFUSED incompatible' in report
  assert 'Fill pulses=0' in report
 print('Fill/preflight/skip/rail checks PASS alias',alias,flush=True)
os.environ['WL_NO_PROGRAM']='1'
d=run_model('original-fill-no-program',['ORG2716 F A9TEST.BIN B 0 > FILL.TXT'],0,[exe,rom])
trace=(d/'ports.log').read_text()
assert trace.count('PULSE ')==1 and 'errors=0 rails=0' in trace
assert 'VERIFY mismatch' in (d/'FILL.TXT').read_text()
del os.environ['WL_NO_PROGRAM']
short=WORK/'SHORT.BIN';short.write_bytes(image[:-1])
d=run_model('original-fill-invalid',[
 'ORG2716 F SHORT.BIN B 0 > SHORT.TXT',
 'ORG2716 F A9TEST.BIN B 1 > HIGH.TXT',
 'del RF2.OK',
 'ORG2716 F A9TEST.BIN B 0 > GATE.TXT'],0,[exe,rom,short])
assert (d/'chip.bin').read_bytes()==b'\xff'*2048
assert (d/'ports.log').read_text().strip() == 'FINAL pulses=0 errors=0 rails=0'
print('No-program abort and invalid size/mode/gate rejection PASS',flush=True)
