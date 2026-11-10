from test_dos_port_model import run_model, WORK
name='fill-alias-200'
d=WORK/name;d.mkdir(exist_ok=True)
pattern=bytes((((i&~512)*73)^((i&~512)>>3)^((i&~512)>>8)^0xa5)&255 for i in range(2048))
(d/'FILL.BIN').write_bytes(pattern)
d=run_model(name,['RF2EXP F FILL.BIN 0 200 > FILL.TXT','RF2EXP R READ.BIN 0 0 > READ.TXT'],512)
report=(d/'FILL.TXT').read_text();trace=(d/'ports.log').read_text()
assert 'result=0' in report,report
assert (d/'READ.BIN').read_bytes()==pattern
assert 'errors=0 rails=0' in trace
assert trace.count('PULSE ')==sum(pattern[i]!=255 for i in range(2048) if not i&512)
print('DOS full 1KiB folded fill passed:',report,flush=True)
