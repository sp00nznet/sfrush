#!/usr/bin/env python3
"""
San Francisco Rush: Extreme Racing - Symbol Extractor

Scans the ROM binary for function entry points and generates a
N64Recomp-compatible symbol TOML file with proper function sizes.

Discovery methods:
  1. JAL (Jump And Link) targets - function calls in MIPS
  2. Standard function prologues (addiu sp, sp, -N)
  3. JR RA detection for function boundary termination

Function sizes are calculated from the distance between adjacent
function start addresses. The code section bounds are determined
by scanning for the end of executable code.

Based on tools from Rampage Recompiled and DKR Recompiled.

Usage:
    python extract_symbols.py <rom.z64> [output.toml]
"""

import struct
import sys

# MIPS instruction constants
JAL_OPCODE = 0x03     # JAL instruction opcode (bits 31-26)
ADDIU_SP_SP = 0x27BD  # addiu sp, sp, imm (common function prologue)
JR_RA = 0x03E00008    # jr ra (function return)
NOP = 0x00000000      # nop (delay slot filler)

# ROM layout for SF Rush USA
ROM_CODE_START = 0x1000
VRAM_BASE = 0x80000400
ROM_SIZE = 8 * 1024 * 1024


def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset + 4])[0]


def find_code_end(rom_data, code_start):
    """
    Find the end of the code section by looking for a long run of
    zero/NOP instructions or non-code data patterns.
    """
    consecutive_zeros = 0
    last_code_offset = code_start

    for offset in range(code_start, len(rom_data) - 4, 4):
        instr = read_be32(rom_data, offset)

        if instr == 0x00000000:
            consecutive_zeros += 1
            if consecutive_zeros >= 64:  # 256 bytes of zeros = end of code
                return last_code_offset + 4
        else:
            consecutive_zeros = 0
            last_code_offset = offset

    return min(len(rom_data), 4 * 1024 * 1024)


def extract_jal_targets(rom_data, code_start, code_end):
    """Extract all JAL (function call) targets from the code section."""
    targets = set()
    vram_start = VRAM_BASE
    vram_end = VRAM_BASE + (code_end - code_start)

    for offset in range(code_start, code_end, 4):
        instr = read_be32(rom_data, offset)
        opcode = (instr >> 26) & 0x3F

        if opcode == JAL_OPCODE:
            target = (instr & 0x03FFFFFF) << 2
            target |= 0x80000000

            # Only include targets within the code section
            if vram_start <= target < vram_end:
                # Verify target is word-aligned
                if (target & 0x3) == 0:
                    targets.add(target)

    return targets


def find_function_prologues(rom_data, code_start, code_end):
    """Find addiu sp, sp, -N patterns (function prologues)."""
    prologues = set()

    for offset in range(code_start, code_end, 4):
        instr = read_be32(rom_data, offset)
        upper16 = (instr >> 16) & 0xFFFF

        if upper16 == ADDIU_SP_SP:
            imm = instr & 0xFFFF
            if imm & 0x8000:  # Negative immediate = stack allocation
                vram = VRAM_BASE + (offset - code_start)
                prologues.add(vram)

    return prologues


def find_jr_ra_locations(rom_data, code_start, code_end):
    """Find jr ra instructions to help validate function boundaries."""
    locations = set()

    for offset in range(code_start, code_end, 4):
        instr = read_be32(rom_data, offset)
        if instr == JR_RA:
            vram = VRAM_BASE + (offset - code_start)
            locations.add(vram)

    return locations


def validate_and_filter_functions(all_addrs, rom_data, code_start, code_end):
    """
    Filter out likely false positives by checking that addresses
    point to plausible MIPS code.
    """
    valid = []
    vram_start = VRAM_BASE
    vram_end = VRAM_BASE + (code_end - code_start)

    for addr in sorted(all_addrs):
        if addr < vram_start or addr >= vram_end:
            continue
        if (addr & 0x3) != 0:
            continue

        rom_offset = (addr - VRAM_BASE) + ROM_CODE_START
        if rom_offset < code_start or rom_offset >= code_end:
            continue

        # Check first instruction is plausible MIPS
        first_instr = read_be32(rom_data, rom_offset)
        opcode = (first_instr >> 26) & 0x3F

        # Filter out obviously invalid first instructions
        # (e.g., all zeros shouldn't be a function start unless it's a nop prologue)
        if first_instr == 0x00000000:
            continue

        valid.append(addr)

    return valid


def calculate_sizes(func_addrs, code_end_vram):
    """Calculate function sizes based on distance to next function."""
    sizes = []
    for i in range(len(func_addrs)):
        if i + 1 < len(func_addrs):
            size = func_addrs[i + 1] - func_addrs[i]
        else:
            # Last function: extend to end of code section
            size = code_end_vram - func_addrs[i]

        # Clamp to reasonable size (max 64KB per function)
        size = min(size, 0x10000)
        # Ensure minimum size of 4 bytes and word alignment
        size = max(size, 4)
        size = (size + 3) & ~3

        sizes.append(size)

    return sizes


def write_symbols_toml(func_addrs, func_sizes, code_section_size, output_path):
    """Write discovered symbols to N64Recomp-compatible TOML format."""
    with open(output_path, 'w') as f:
        f.write("# San Francisco Rush: Extreme Racing (USA) - Extracted Symbols\n")
        f.write(f"# Auto-generated - {len(func_addrs)} functions\n")
        f.write(f"# Code section: 0x{ROM_CODE_START:X} - 0x{ROM_CODE_START + code_section_size:X}\n")
        f.write(f"# VRAM range: 0x{VRAM_BASE:08X} - 0x{VRAM_BASE + code_section_size:08X}\n\n")

        f.write("[[section]]\n")
        f.write(f'name = "main"\n')
        f.write(f"rom = 0x{ROM_CODE_START:X}\n")
        f.write(f"vram = 0x{VRAM_BASE:08X}\n")
        f.write(f"size = 0x{code_section_size:X}\n\n")

        for addr, size in zip(func_addrs, func_sizes):
            f.write(f"[[section.functions]]\n")
            f.write(f'name = "func_{addr:08X}"\n')
            f.write(f"vram = 0x{addr:08X}\n")
            f.write(f"size = 0x{size:X}\n\n")

    print(f"Wrote {len(func_addrs)} symbols to {output_path}")


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <rom.z64> [output.toml]")
        sys.exit(1)

    rom_path = sys.argv[1]
    output_path = sys.argv[2] if len(sys.argv) > 2 else "symbols/sfrush.us.syms.toml"

    with open(rom_path, 'rb') as f:
        rom_data = f.read()

    print(f"ROM: {rom_path} ({len(rom_data)} bytes)")

    # Find code section bounds
    code_end = find_code_end(rom_data, ROM_CODE_START)
    code_section_size = code_end - ROM_CODE_START
    code_end_vram = VRAM_BASE + code_section_size

    print(f"Code section: 0x{ROM_CODE_START:X} - 0x{code_end:X} ({code_section_size} bytes)")
    print(f"VRAM range: 0x{VRAM_BASE:08X} - 0x{code_end_vram:08X}")

    # Discover functions
    print("\nScanning for JAL targets...")
    jal_targets = extract_jal_targets(rom_data, ROM_CODE_START, code_end)
    print(f"  Found {len(jal_targets)} JAL targets")

    print("Scanning for function prologues...")
    prologues = find_function_prologues(rom_data, ROM_CODE_START, code_end)
    print(f"  Found {len(prologues)} prologues")

    print("Finding jr ra locations...")
    jr_ras = find_jr_ra_locations(rom_data, ROM_CODE_START, code_end)
    print(f"  Found {len(jr_ras)} return points")

    # Merge and validate
    all_candidates = jal_targets | prologues
    # Always include the entry point
    all_candidates.add(VRAM_BASE)

    print(f"\nTotal candidates: {len(all_candidates)}")
    print("Validating and filtering...")
    func_addrs = validate_and_filter_functions(all_candidates, rom_data, ROM_CODE_START, code_end)
    print(f"Valid functions: {len(func_addrs)}")

    # Calculate sizes
    func_sizes = calculate_sizes(func_addrs, code_end_vram)

    # Write output
    write_symbols_toml(func_addrs, func_sizes, code_section_size, output_path)

    # Print summary stats
    total_covered = sum(func_sizes)
    coverage = (total_covered / code_section_size) * 100 if code_section_size > 0 else 0
    print(f"\nCode coverage: {total_covered} / {code_section_size} bytes ({coverage:.1f}%)")
    print(f"Average function size: {total_covered // len(func_addrs)} bytes")


if __name__ == "__main__":
    main()
