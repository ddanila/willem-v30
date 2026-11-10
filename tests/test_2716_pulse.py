"""Run the actual fixed-pulse assembly with a wrapping PIT and check polarity."""
from pathlib import Path
import subprocess
import tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'src/dosio.asm').read_text().split('_dos_2716_pulse:', 1)[1]
with tempfile.TemporaryDirectory() as name:
    tmp = Path(name)
    (tmp/'pulse.asm').write_text('BITS 16\n' + source)
    subprocess.run(['nasm', '-f', 'bin', str(tmp/'pulse.asm'), '-o', str(tmp/'pulse.bin')], check=True)
    for initial in (0, 1000, 65535):
        for flags in (2, 0x202):
            cpu = Uc(UC_ARCH_X86, UC_MODE_16)
            cpu.mem_map(0, 65536)
            cpu.mem_write(0x1000, (tmp/'pulse.bin').read_bytes())
            cpu.mem_write(0x8000, b'\x00\x20\x78\x03')
            cpu.reg_write(UC_X86_REG_SP, 0x8000)
            cpu.reg_write(UC_X86_REG_EFLAGS, flags)
            regs = [UC_X86_REG_BP, UC_X86_REG_BX, UC_X86_REG_CX, UC_X86_REG_DX]
            for r in regs: cpu.reg_write(r, 0x3456)
            state = dict(ticks=0, latched=0, half=0, edges=[])
            def out(uc, port, size, value, _):
                assert size == 1
                if port == 0x37a:
                    state['edges'].append((state['ticks'], value))
                    assert not uc.reg_read(UC_X86_REG_EFLAGS) & 0x200
                else:
                    assert port == 0x43 and value == 0
                    state['latched'] = (initial - state['ticks']) & 65535
                    state['half'] = 0
                    state['ticks'] += 17
            def inp(uc, port, size, _):
                assert port == 0x40 and size == 1
                value = (state['latched'] >> (8 * state['half'])) & 255
                state['half'] ^= 1
                return value
            cpu.hook_add(UC_HOOK_INSN, out, None, 1, 0, UC_X86_INS_OUT)
            cpu.hook_add(UC_HOOK_INSN, inp, None, 1, 0, UC_X86_INS_IN)
            cpu.emu_start(0x1000, 0x2000, count=100000)
            assert [v for _, v in state['edges']] == [0x06, 0x0e]
            elapsed = state['edges'][1][0] - state['edges'][0][0]
            assert 59659 <= elapsed <= 59693, elapsed
            assert cpu.reg_read(UC_X86_REG_EFLAGS) & 0x202 == flags
            for r in regs: assert cpu.reg_read(r) == 0x3456
            assert cpu.reg_read(UC_X86_REG_SP) == 0x8002
    print('2716 actual assembly: positive 50ms pulse, PIT wrap, IRQ flags and registers PASS')
