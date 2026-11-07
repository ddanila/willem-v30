"""Compare the actual 8086 address shifter against portable-core port traces.

Requires Unicorn (pip install unicorn) and nasm; no physical I/O is performed.
"""
import ctypes as C
from itertools import product
from pathlib import Path
import random
import subprocess
import tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_16, UC_HOOK_INSN
from unicorn.x86_const import (
    UC_X86_INS_OUT, UC_X86_REG_AX, UC_X86_REG_BP, UC_X86_REG_BX,
    UC_X86_REG_CX, UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI,
    UC_X86_REG_SP,
)

ROOT = Path(__file__).resolve().parents[1]
WRITE = C.CFUNCTYPE(None, C.c_void_p, C.c_int)
READ = C.CFUNCTYPE(C.c_ubyte, C.c_void_p)
PULSE = C.CFUNCTYPE(None, C.c_void_p, C.c_uint)

class IO(C.Structure):
    _fields_ = [('ctx', C.c_void_p), ('data', WRITE), ('control', WRITE),
                ('status', READ), ('delay', WRITE), ('pulse', PULSE)]

class Willem(C.Structure):
    _fields_ = [('io', IO), ('data', C.c_ubyte), ('control', C.c_ubyte)] + [
        (n, C.c_uint) for n in ('address_us', 'oe_us', 'latch_us', 'clock_us', 'power_ms')]

with tempfile.TemporaryDirectory(prefix='willem-fastaddr-') as directory:
    tmp = Path(directory)
    subprocess.run(['nasm', '-f', 'bin', str(ROOT/'src/fastaddr.asm'), '-o', str(tmp/'code.bin')], check=True)
    subprocess.run(['cc', '-std=c89', '-shared', '-fPIC', '-I'+str(ROOT/'include'),
                    str(ROOT/'src/willem.c'), '-o', str(tmp/'core.so')], check=True)
    lib = C.CDLL(str(tmp/'core.so'))
    lib.wl_set_address.argtypes = [C.POINTER(Willem), C.c_ulong, C.c_ulong]
    expected = []
    data_cb = WRITE(lambda _, value: expected.append((0x378, value)))
    control_cb = WRITE(lambda _, value: expected.append((0x37a, value)))
    delay_cb = WRITE(lambda _, value: expected.append(('delay', value)))
    status_cb = READ(lambda _: 0)
    pulse_cb = PULSE(lambda *_: None)
    io = IO(None, data_cb, control_cb, status_cb, delay_cb, pulse_cb)
    rng = random.Random(0x1001)
    addresses = list(range(0, 65536, 257)) + [0, 1, 0x7fff, 0x8000, 0xffff, 0x10000, 0x10001,
                 0x1ffff, 0x800000, 0xffffff] + [rng.randrange(0x1000000) for _ in range(256)]
    tested = 0
    for address in addresses:
        for data, control in product((0, 4, 0xff), (4, 12)):
            expected.clear()
            wl = Willem(io, data, control, 4, 4, 4, 4, 5)
            lib.wl_set_address(C.byref(wl), address, 0x800000)
            # The C caller performs the address delay and final MUX-off write.
            assert expected[-2:] == [('delay', 4), (0x37a, control ^ 0x0b)]
            expected_ports = expected[:-2]
            cpu = Uc(UC_ARCH_X86, UC_MODE_16)
            cpu.mem_map(0, 0x10000)
            cpu.mem_write(0x1000, (tmp/'code.bin').read_bytes())
            regs = [UC_X86_REG_BP, UC_X86_REG_BX, UC_X86_REG_CX,
                    UC_X86_REG_DX, UC_X86_REG_SI, UC_X86_REG_DI]
            for index, reg in enumerate(regs): cpu.reg_write(reg, 0x2200+index)
            cpu.reg_write(UC_X86_REG_SP, 0x8000)
            words = [0x2000, 0x378, address & 0xffff, address >> 16, data, control]
            cpu.mem_write(0x8000, b''.join(w.to_bytes(2, 'little') for w in words))
            actual = []
            def out_hook(uc, port, size, value, _):
                assert size == 1
                actual.append((port, value))
            cpu.hook_add(UC_HOOK_INSN, out_hook, None, 1, 0, UC_X86_INS_OUT)
            cpu.emu_start(0x1000, 0x2000, count=2000)
            assert cpu.reg_read(UC_X86_REG_SP) == 0x8002
            assert actual == expected_ports, (hex(address), data, actual, expected_ports)
            assert cpu.reg_read(UC_X86_REG_AX) == wl.data
            for index, reg in enumerate(regs): assert cpu.reg_read(reg) == 0x2200+index
            # Decode the clock edges independently of the C model.
            old = data
            shifted = 0
            clocks = 0
            for port, value in actual:
                if port == 0x37a:
                    assert value == ((control | 2) ^ 0x0b)  # Preserve CE/WE level.
                else:
                    if not old & 1 and value & 1:
                        shifted = (shifted << 1) | ((value >> 1) & 1)
                        clocks += 1
                    old = value
            assert clocks == 24 and shifted == address
            tested += 1
    print(f'8086 fast-address tests passed: {tested} exact portable-core traces; full 24-bit addresses, registers and rails checked')
