#!/usr/bin/env python3
"""Analyze SF Rush boot code to find DMA load addresses."""

import struct
import sys
import os

def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset+4])[0]

def disasm_simple(instr, pc):
    """Basic MIPS disassembly for key instructions."""
    op = (instr >> 26) & 0x3F
    rs = (instr >> 21) & 0x1F
    rt = (instr >> 16) & 0x1F
    rd = (instr >> 11) & 0x1F
    imm = instr & 0xFFFF
    simm = imm if imm < 0x8000 else imm - 0x10000

    regs = ['zero','at','v0','v1','a0','a1','a2','a3',
            't0','t1','t2','t3','t4','t5','t6','t7',
            's0','s1','s2','s3','s4','s5','s6','s7',
            't8','t9','k0','k1','gp','sp','fp','ra']

    if op == 0x0F:  # LUI
        return f"lui {regs[rt]}, 0x{imm:04X}"
    elif op == 0x09:  # ADDIU
        return f"addiu {regs[rt]}, {regs[rs]}, {simm}"
    elif op == 0x0D:  # ORI
        return f"ori {regs[rt]}, {regs[rs]}, 0x{imm:04X}"
    elif op == 0x03:  # JAL
        target = ((instr & 0x03FFFFFF) << 2) | (pc & 0xF0000000)
        return f"jal 0x{target:08X}"
    elif op == 0x00:
        funct = instr & 0x3F
        if funct == 0x08:
            return f"jr {regs[rs]}"
        elif funct == 0x21:
            return f"addu {regs[rd]}, {regs[rs]}, {regs[rt]}"
    elif op == 0x2B:  # SW
        return f"sw {regs[rt]}, {simm}({regs[rs]})"
    elif op == 0x23:  # LW
        return f"lw {regs[rt]}, {simm}({regs[rs]})"
    elif op == 0x05:  # BNE
        target = pc + 4 + (simm << 2)
        return f"bne {regs[rs]}, {regs[rt]}, 0x{target:08X}"
    elif op == 0x04:  # BEQ
        target = pc + 4 + (simm << 2)
        return f"beq {regs[rs]}, {regs[rt]}, 0x{target:08X}"

    return f"0x{instr:08X}"

def main():
    z64_files = [f for f in os.listdir('.') if f.endswith('.z64')]
    if not z64_files:
        print("No .z64 file found")
        sys.exit(1)

    with open(z64_files[0], 'rb') as f:
        data = f.read()

    # Disassemble boot code (first 512 instructions from entry)
    print("=== Boot code disassembly (0x80000400 / ROM 0x1000) ===\n")
    rom_start = 0x1000
    vram_start = 0x80000400

    for i in range(256):
        rom_off = rom_start + (i * 4)
        vram_addr = vram_start + (i * 4)
        instr = read_be32(data, rom_off)
        disasm = disasm_simple(instr, vram_addr)
        print(f"  {vram_addr:08X}: {instr:08X}  {disasm}")

    # Look for osPiStartDma patterns or DMA-related function calls
    # Also look for LUI pairs that construct large ROM addresses
    print("\n=== Searching for LUI/ORI pairs constructing addresses ===")
    for off in range(0x1000, 0x1CA00, 4):
        instr = read_be32(data, off)
        op = (instr >> 26) & 0x3F
        if op == 0x0F:  # LUI
            rt = (instr >> 16) & 0x1F
            hi = instr & 0xFFFF
            # Look for following ORI/ADDIU to complete the address
            if off + 4 < len(data):
                next_instr = read_be32(data, off + 4)
                next_op = (next_instr >> 26) & 0x3F
                if next_op in (0x09, 0x0D):  # ADDIU or ORI
                    next_rs = (next_instr >> 21) & 0x1F
                    next_rt_2 = (next_instr >> 16) & 0x1F
                    lo = next_instr & 0xFFFF
                    if next_rs == rt:
                        if next_op == 0x09:  # ADDIU (sign-extend)
                            full = ((hi << 16) + (lo if lo < 0x8000 else lo - 0x10000)) & 0xFFFFFFFF
                        else:  # ORI
                            full = (hi << 16) | lo
                        vram = vram_start + (off - rom_start)
                        if 0x00010000 <= full <= 0x00800000:
                            print(f"  {vram:08X}: ROM addr 0x{full:08X}")
                        elif 0x80000000 <= full <= 0x80800000:
                            print(f"  {vram:08X}: VRAM addr 0x{full:08X}")

if __name__ == "__main__":
    main()
