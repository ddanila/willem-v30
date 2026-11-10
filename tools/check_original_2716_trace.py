#!/usr/bin/env python3
"""Decode original DOS 2716 captures; does not establish electrical timing.

Trace lines: W/R hexadecimal-port hexadecimal-value monotonic-host-seconds.
Synthetic chip replies are a data oracle, not a 4014/4021 electrical model.
"""
import argparse
from collections import Counter
import gzip
import hashlib
import json
from pathlib import Path

def analyze(path, expected):
    raw = gzip.decompress(path.read_bytes()) if path.suffix=='.gz' else path.read_bytes()
    control=data=shift=0
    bits=[];reads=[];pulses=[];start=None
    events=Counter()
    for line in raw.decode().splitlines():
        op,port,value,stamp=line.split()
        port=int(port,16);value=int(value,16);stamp=float(stamp)
        events[f'{op} {port:04X}']+=1
        if op=='W' and port==0x378:
            if not control&2 and not data&1 and value&1:
                shift=((shift<<1)|((value>>1)&1))&0xffffff
            data=value
        elif op=='W' and port==0x37a:
            if control&8 and not value&8 and not value&1 and value&4:
                assert start is None
                start=(stamp,shift&2047,data)
            if not control&8 and value&8 and start is not None:
                pulses.append(dict(address=start[1],value=start[2],
                    emulated_host_ms=round((stamp-start[0])*1000,3)))
                start=None
            control=value
        elif op=='R' and port==0x379 and control!=7:
            bits.append((shift&2047,control,int(not value&64)))
            if len(bits)==8:
                assert len({(a,c) for a,c,_ in bits})==1
                reads.append((bits[0][0],bits[0][1],sum(b[2]<<(7-i) for i,b in enumerate(bits))))
                bits=[]
    assert not bits and start is None
    wanted=[(i,v) for i,v in enumerate(expected) if v!=255]
    assert [(p['address'],p['value']) for p in pulses]==wanted
    assert len(reads)==len(wanted)+2048
    assert [(a,v) for a,_,v in reads[:len(wanted)]]==wanted
    assert [(a,v) for a,_,v in reads[-2048:]]==list(enumerate(expected))
    assert {c for _,c,_ in reads}=={0x0d}
    assert (control ^ 0x0b)&5==0
    return dict(trace_sha256=hashlib.sha256(raw).hexdigest(),pulses=pulses,
        read_bytes=len(reads),read_control='0D',final_rails_off=True,
        verified_image_sha256=hashlib.sha256(expected).hexdigest(),events=dict(events),
        limitations='Synthetic replies; emulator host timing is not a physical CE measurement.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('trace',type=Path);p.add_argument('image',type=Path)
    a=p.parse_args();print(json.dumps(analyze(a.trace,a.image.read_bytes()),indent=2))
