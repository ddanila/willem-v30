"""Deterministic sacrificial 2716 test image, preserving the two existing bytes."""
from pathlib import Path
from hashlib import sha256
import json
p = Path(__file__).resolve().parent
image = bytearray(sha256(b'Willem RF2 R22 10k address test v1\0' + a.to_bytes(2, 'little')).digest()[0] for a in range(2048))
image[0] = 0x5a
image[0x200] = 0xa5
coverage = {f'A{bit}': sum(image[a] != image[a ^ (1 << bit)] for a in range(2048) if not a & (1 << bit)) for bit in range(11)}
assert min(coverage.values()) > 990
assert len(set(image)) == 256
assert len({bytes(image[i:i+256]) for i in range(0,2048,256)}) == 8
(p/'A9TEST.BIN').write_bytes(image)
(p/'pattern.json').write_text(json.dumps({'size':len(image),'sha256':sha256(image).hexdigest(),'different_pairs_per_address_bit':coverage,'new_pulses_expected':sum(v!=255 for v in image)-2,'preserved':{'0000':'5A','0200':'A5'},'purpose':'Diagnostic data, not executable firmware'},indent=2)+'\n')
print((p/'pattern.json').read_text())
