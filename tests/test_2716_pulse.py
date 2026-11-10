"""Execute actual pulse/probe assembly with a clocked PIT2 mode-0 model."""
from pathlib import Path
import subprocess
import tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
source = (ROOT/'src/dosio.asm').read_text().split('; 2716 positive pulse,', 1)[1]
source = '; 2716 positive pulse,' + source
with tempfile.TemporaryDirectory() as name:
    tmp = Path(name)
    for probe in (False, True):
        entry = '_dos_timer2_50ms' if probe else '_dos_2716_pulse'
        (tmp/'pulse.asm').write_text('BITS 16\njmp '+entry+'\n'+source)
        subprocess.run(['nasm','-f','bin',str(tmp/'pulse.asm'),'-o',str(tmp/'pulse.bin')],check=True)
        for initial in (0, 1, 2, 3):
            for flags in (2, 0x202):
                cpu = Uc(UC_ARCH_X86, UC_MODE_16)
                cpu.mem_map(0,65536)
                cpu.mem_write(0x1000,(tmp/'pulse.bin').read_bytes())
                cpu.mem_write(0x8000,b'\x00\x20\x78\x03')
                cpu.reg_write(UC_X86_REG_SP,0x8000)
                cpu.reg_write(UC_X86_REG_EFLAGS,flags)
                regs=[UC_X86_REG_BP,UC_X86_REG_BX,UC_X86_REG_CX,UC_X86_REG_DX,UC_X86_REG_SI,UC_X86_REG_DI]
                for r in regs: cpu.reg_write(r,0x3456)
                state=dict(ticks=0, control=initial, load=[], start=None, count=0, edges=[])
                def out(uc,port,size,value,_):
                    assert size==1
                    assert not uc.reg_read(UC_X86_REG_EFLAGS)&0x200
                    state['ticks']+=17
                    if port==0x37a:
                        state['edges'].append((state['ticks'],value))
                    elif port==0x43:
                        assert value==0xb0
                        state['load']=[];state['start']=None
                    elif port==0x42:
                        state['load'].append(value)
                        assert len(state['load'])<=2
                        if len(state['load'])==2:
                            state['count']=state['load'][0]+256*value
                    else:
                        assert port==0x61
                        if value&1 and not state['control']&1 and state['count']:
                            state['start']=state['ticks']
                        state['control']=value
                def inp(uc,port,size,_):
                    assert port==0x61 and size==1
                    state['ticks']+=17
                    done=state['start'] is not None and state['ticks']-state['start']>=state['count']
                    return (state['control'] & ~32) | (32 if done else 0)
                cpu.hook_add(UC_HOOK_INSN,out,None,1,0,UC_X86_INS_OUT)
                cpu.hook_add(UC_HOOK_INSN,inp,None,1,0,UC_X86_INS_IN)
                cpu.emu_start(0x1000,0x2000,count=50000)
                assert state['count']==59659
                assert state['control']==initial
                assert cpu.reg_read(UC_X86_REG_SP)==0x8002
                assert cpu.reg_read(UC_X86_REG_EFLAGS)&0x202==flags
                for r in regs: assert cpu.reg_read(r)==0x3456
                if probe:
                    assert state['edges']==[]
                else:
                    assert [v for _,v in state['edges']]==[6,14]
                    elapsed=state['edges'][1][0]-state['edges'][0][0]
                    assert 59659<=elapsed<=59727,elapsed
    print('2716 PIT2 mode-0: 50ms positive pulse, no-LPT probe, gate/speaker/flags/registers PASS')
