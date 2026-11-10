"""Actual DOS one-pulse command, fresh read and refusal after nonblank data."""
from test_dos_port_model import run_model
for alias in (0, 0x40, 0x200):
    d = run_model(f'cold-probe-{alias:x}', [
        'RF2EXP P 0 C3 0 > PULSE.TXT',
        'RF2EXP R COLD.BIN 0 4 > READ.TXT',
        'RF2EXP P 1 06 0 > REFUSE.TXT'], alias)
    assert 'rails OFF; no immediate verification' in (d/'PULSE.TXT').read_text()
    assert 'REFUSED: nonblank address 0000; no pulse' in (d/'REFUSE.TXT').read_text()
    trace = (d/'ports.log').read_text()
    assert trace.count('PULSE ')==1 and 'errors=0 rails=0' in trace, trace
    expected=bytearray(b'\xff'*2048);expected[0]=0xc3
    assert (d/'chip.bin').read_bytes()==expected
    assert (d/'COLD.BIN').read_bytes()==bytes(expected[i & ~alias] for i in range(2048))
    print('One pulse / fresh power-cycle read / full blank guard: alias',alias,'PASS',flush=True)
