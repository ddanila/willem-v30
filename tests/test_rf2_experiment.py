from test_dos_port_model import run_model
for alias in (0,512):
 d=run_model(f'experiment-{alias:x}',[
  'RF2EXP W 1 FE 14 > WRITE1.TXT',
  'RF2EXP W 202 FD 64 > WRITE2.TXT',
  'RF2EXP W 1 F0 0 > REFUSE.TXT',
  'RF2EXP R CHECK.BIN 0 0 > READ.TXT',
  'RF2EXP R OFF.BIN 14 3 > OFF.TXT',
  'RF2EXP R ASM.BIN 0 4 > ASM.TXT',
  'RF2EXP W garbage FE 0 > BAD.TXT',
  'RF2EXP A 200 1 0 > HOLD.TXT'],alias)
 for f in ('WRITE1.TXT','WRITE2.TXT'):
  assert 'One pulse' in (d/f).read_text(),(d/f).read_text()
 assert 'REFUSED' in (d/'REFUSE.TXT').read_text()
 assert 'result=0' in (d/'READ.TXT').read_text()
 trace=(d/'ports.log').read_text()
 assert 'errors=0 rails=0' in trace and trace.count('PULSE ')==2,trace
 chip=(d/'chip.bin').read_bytes();read=(d/'CHECK.BIN').read_bytes()
 expected=bytearray(b'\xff'*2048);expected[1]=254;expected[0x202 & ~alias]=253
 assert chip==expected
 assert read==bytes(expected[i & ~alias] for i in range(2048))
 assert (d/'OFF.BIN').read_bytes()==b'\xff'*2048
 assert (d/'ASM.BIN').read_bytes()==read
 assert 'Before' not in (d/'BAD.TXT').read_text()
 assert 'A9 expected 1' in (d/'HOLD.TXT').read_text()
 assert 'SAFE OFF' in (d/'HOLD.TXT').read_text()
 print('RF2EXP actual DOS port model passed, alias=',alias,flush=True)
