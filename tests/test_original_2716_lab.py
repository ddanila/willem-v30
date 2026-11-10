"""Port-level checks of the bounded original-sequence diagnostic."""
import os
from pathlib import Path
from test_dos_port_model import run_model, ROOT
# Capture oracle: status reads return successive bits of the selected byte.
# This deliberately does not qualify physical 4014/4021 latch timing.
os.environ['WL_SAMPLE_ORACLE']='1'
os.environ['WL_ADDRESS_BITS']='0'
for alias in (0,512):
    d=run_model('original-sequence-'+str(alias),[
        'ORG2716 W 201 A5 B > WRITE.TXT',
        'ORG2716 R READ.BIN B 0 > READ.TXT',
        'ORG2716 R HIGH.BIN 18 1 > HIGH.TXT',
        'ORG2716 W 201 00 B > REFUSE.TXT',
        'ORG2716 W garbage A5 B > INVALID.TXT'],alias,[ROOT/'build/dos/ORG2716.COM'])
    b=bytearray(b'\xff'*2048);b[0x201 & ~alias]=0xa5
    expected=bytes(b[i & ~alias] for i in range(2048))
    assert (d/'READ.BIN').read_bytes()==expected
    assert (d/'HIGH.BIN').read_bytes()==expected
    assert 'REFUSED' in (d/'REFUSE.TXT').read_text()
    assert not (d/'INVALID.TXT').read_text()
    trace=(d/'ports.log').read_text()
    assert trace.count('PULSE ')==1 and 'errors=0 rails=0' in trace,trace
    print('Original sequences: 11/24-bit reads, normal/high VPP, one pulse, refusal PASS',alias,flush=True)
