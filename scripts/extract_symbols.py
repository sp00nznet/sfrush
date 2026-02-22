#!/usr/bin/env python3
"""
San Francisco Rush: Extreme Racing - Symbol Extractor

Scans the ROM binary for function entry points by looking for:
  1. JAL (Jump And Link) targets - function calls in MIPS
  2. Standard function prologues (addiu sp, sp, -N)
  3. Known libultra function signatures

Based on tools from Rampage Recompiled and DKR Recompiled.

Usage:
    python extract_symbols.py <rom.z64> [output.toml]
"""

import struct
import sys
import os

# MIPS instruction constants
JAL_OPCODE = 0x03  # JAL instruction opcode (bits 31-26)
ADDIU_SP_SP = 0x27BD  # addiu sp, sp, imm (common function prologue)

# ROM layout for SF Rush USA
ROM_CODE_START = 0x1000
VRAM_BASE = 0x80000400
ROM_SIZE = 8 * 1024 * 1024


def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset+4])[0]


def extract_jal_targets(rom_data, code_start, code_end):
    """Extract all JAL (function call) targets from the code section."""
    targets = set()

    for offset in range(code_start, code_end, 4):
        instr = read_be32(rom_data, offset)
        opcode = (instr >> 26) & 0x3F

        if opcode == JAL_OPCODE:
            # JAL target is 26-bit word address, shifted left 2
            target = (instr & 0x03FFFFFF) << 2
            # Combine with upper bits of PC
            target |= 0x80000000
            targets.add(target)

    return sorted(targets)


def find_function_prologues(rom_data, code_start, code_end):
    """Find addiu sp, sp, -N patterns (function prologues)."""
    prologues = set()

    for offset in range(code_start, code_end, 4):
        instr = read_be32(rom_data, offset)
        upper16 = (instr >> 16) & 0xFFFF

        if upper16 == ADDIU_SP_SP:
            imm = instr & 0xFFFF
            # Negative immediate = stack frame allocation
            if imm & 0x8000:  # Sign bit set
                vram = VRAM_BASE + (offset - code_start)
                prologues.add(vram)

    return sorted(prologues)


def write_symbols_toml(targets, prologues, output_path):
    """Write discovered symbols to TOML format for N64Recomp."""
    all_funcs = sorted(set(targets) | set(prologues))

    with open(output_path, 'w') as f:
        f.write("# San Francisco Rush: Extreme Racing (USA) - Extracted Symbols\n")
        f.write(f"# Auto-generated - {len(all_funcs)} functions discovered\n")
        f.write(f"# JAL targets: {len(targets)}, Prologues: {len(prologues)}\n\n")

        f.write("[[section]]\n")
        f.write(f'name = "main"\n')
        f.write(f"rom = 0x{ROM_CODE_START:X}\n")
        f.write(f"vram = 0x{VRAM_BASE:08X}\n\n")

        for i, addr in enumerate(all_funcs):
            source = "jal" if addr in targets else "prologue"
            f.write(f"[[section.functions]]\n")
            f.write(f'name = "func_{addr:08X}"\n')
            f.write(f"vram = 0x{addr:08X}\n")
            f.write(f"# source: {source}\n\n")

    print(f"Wrote {len(all_funcs)} symbols to {output_path}")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <rom.z64> [output.toml]")
        sys.exit(1)

    rom_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "symbols/sfrush.us.syms.toml"

    with open(rom_path, 'rb') as f:
        rom_data = f.read()

    print(f"ROM: {rom_path} ({len(rom_data)} bytes)")

    # Determine code section bounds
    # For now, scan the first 4MB (code is typically in the first half)
    code_end = min(len(rom_data), 4 * 1024 * 1024)

    print(f"Scanning code section: 0x{ROM_CODE_START:X} - 0x{code_end:X}")

    targets = extract_jal_targets(rom_data, ROM_CODE_START, code_end)
    print(f"Found {len(targets)} JAL targets")

    prologues = find_function_prologues(rom_data, ROM_CODE_START, code_end)
    print(f"Found {len(prologues)} function prologues")

    write_symbols_toml(targets, prologues, output_path)


if __name__ == "__main__":
    main()
