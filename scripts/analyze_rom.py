#!/usr/bin/env python3
"""Analyze San Francisco Rush ROM structure to find code/data sections."""

import struct
import sys
import os

def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset+4])[0]

def main():
    # Find the z64 file
    z64_files = [f for f in os.listdir('.') if f.endswith('.z64')]
    if not z64_files:
        print("No .z64 file found")
        sys.exit(1)

    rom_path = z64_files[0]
    with open(rom_path, 'rb') as f:
        data = f.read()

    print(f"ROM: {rom_path} ({len(data)} bytes)")

    # Check around 0x1C9C8
    print("\n=== Around 0x1C9C8 (detected code end) ===")
    for off in range(0x1C9B0, 0x1CA20, 4):
        val = read_be32(data, off)
        print(f"  0x{off:06X}: 0x{val:08X}")

    # Scan for non-zero 4KB blocks across entire ROM
    print("\n=== ROM region density (non-zero 4KB blocks) ===")
    regions = []
    for block_start in range(0, len(data), 0x1000):
        block = data[block_start:block_start+0x1000]
        nonzero = sum(1 for b in block if b != 0)
        if nonzero > 100:
            regions.append((block_start, nonzero))

    # Print summary of regions
    if regions:
        start = regions[0][0]
        prev_end = regions[0][0] + 0x1000
        for block_start, density in regions[1:]:
            if block_start > prev_end + 0x2000:  # Gap > 8KB
                print(f"  Region: 0x{start:06X} - 0x{prev_end:06X} ({prev_end-start} bytes)")
                start = block_start
            prev_end = block_start + 0x1000
        print(f"  Region: 0x{start:06X} - 0x{prev_end:06X} ({prev_end-start} bytes)")

    # Look for boot code structure
    # N64 ROM: 0x0-0x40 header, 0x40-0x1000 IPL3 bootcode, 0x1000+ game code
    # Check for the boot procedure
    entry_rom = 0x1000  # Maps to 0x80000400
    print(f"\n=== Entry point analysis ===")
    print(f"Entry ROM offset: 0x{entry_rom:X}")
    for off in range(entry_rom, entry_rom + 0x40, 4):
        val = read_be32(data, off)
        print(f"  0x{off:06X}: 0x{val:08X}")

    # Check for JAL instructions density across ROM
    print("\n=== JAL instruction density per 64KB block ===")
    for block_start in range(0x1000, len(data), 0x10000):
        jal_count = 0
        for off in range(block_start, min(block_start + 0x10000, len(data)), 4):
            instr = read_be32(data, off)
            if (instr >> 26) == 0x03:  # JAL opcode
                jal_count += 1
        if jal_count > 10:
            print(f"  0x{block_start:06X}: {jal_count} JALs")

    # Look for the DMA table or boot loader that loads code segments
    # Midway games often have a loading table
    print("\n=== Potential DMA/load table entries (pointer pairs) ===")
    for off in range(0x1000, min(0x20000, len(data)), 4):
        val1 = read_be32(data, off)
        val2 = read_be32(data, off + 4) if off + 4 < len(data) else 0
        # Look for ROM offset / VRAM address pairs
        if (0x80000000 <= val1 <= 0x80800000 and
            0x00001000 <= val2 <= 0x00800000):
            print(f"  0x{off:06X}: VRAM=0x{val1:08X} ROM=0x{val2:08X}")
        elif (0x00001000 <= val1 <= 0x00800000 and
              0x80000000 <= val2 <= 0x80800000):
            print(f"  0x{off:06X}: ROM=0x{val1:08X} VRAM=0x{val2:08X}")

if __name__ == "__main__":
    main()
