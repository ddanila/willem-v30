"""Execute the meter helper; verify rail sequencing and absence of CE pulses."""
from pathlib import Path
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN, UC_HOOK_INTR
from unicorn.x86_const import *
root = Path(__file__).resolve().parents[1]
subprocess.run(['nasm', '-f', 'bin', str(root/'tools/vpphold.asm'), '-o', str(root/'build/dos/VPPHOLD.COM')], check=True)
for initial in (0, 65530):
    cpu = Uc(UC_ARCH_X86, UC_MODE_16)
    cpu.mem_map(0, 65536)
    cpu.mem_write(0x100, (root/'build/dos/VPPHOLD.COM').read_bytes())
    cpu.reg_write(UC_X86_REG_SP, 0xfffe)
    state = {'ticks': initial, 'control': 0x0b, 'edges': []}
    def inp(uc, port, size, _):
        assert port == 0x37a and size == 1
        return state['control']
    def out(uc, port, size, value, _):
        assert size == 1 and port in (0x378, 0x37a)
        if port == 0x37a:
            state['control'] = value
            state['edges'].append((state['ticks'], value))
            assert value & 8, 'CE/PGM asserted'
            assert value & 2, 'OE low'
            assert value & 1 or value & 4, 'VPP without VCC'
    def intr(uc, number, _):
        if number == 0x1a:
            state['ticks'] += 1
            uc.reg_write(UC_X86_REG_DX, state['ticks'] & 65535)
            uc.reg_write(UC_X86_REG_CX, state['ticks'] >> 16)
        else:
            assert number == 0x21 and state['control'] == 0x0b
            if uc.reg_read(UC_X86_REG_AH) == 0x4c:
                uc.emu_stop()
    cpu.hook_add(UC_HOOK_INSN, inp, None, 1, 0, UC_X86_INS_IN)
    cpu.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
    cpu.hook_add(UC_HOOK_INTR, intr)
    cpu.emu_start(0x100, 0xffff, count=20000)
    edges = state['edges']
    assert [v for _,v in edges] == [0x0b,0x0b,0x0f,0x0e,0x0f,0x0b]
    assert edges[4][0] - edges[3][0] == 548
    assert edges[3][0] - edges[2][0] >= 2
print('VPP hold: 30-second BIOS timer, wrap, CE inactive, VCC-first/VPP-off-first PASS')
