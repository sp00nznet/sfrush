#!/usr/bin/env python3
"""Find JAL targets that are outside the known code sections."""

import struct
import sys
import os

def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset+4])[0]

def main():
    z64_files = [f for f in os.listdir('.') if f.endswith('.z64')]
    if not z64_files:
        print("No .z64 file found")
        sys.exit(1)

    with open(z64_files[0], 'rb') as f:
        data = f.read()

    # Boot code section bounds
    code_start = 0x1000
    code_end = 0x1CA00  # Approximate
    vram_start = 0x80000400
    vram_end = vram_start + (code_end - code_start)

    # Find all JAL targets from within the boot code
    internal_targets = set()
    external_targets = set()

    for off in range(code_start, code_end, 4):
        instr = read_be32(data, off)
        if (instr >> 26) == 0x03:  # JAL
            target = ((instr & 0x03FFFFFF) << 2) | 0x80000000
            if vram_start <= target < vram_end:
                internal_targets.add(target)
            else:
                external_targets.add(target)

    print(f"Internal JAL targets (within boot code): {len(internal_targets)}")
    print(f"External JAL targets (outside boot code): {len(external_targets)}")

    if external_targets:
        print(f"\nExternal targets that need stubs:")
        for addr in sorted(external_targets):
            # Find which function calls this
            callers = []
            for off in range(code_start, code_end, 4):
                instr = read_be32(data, off)
                if (instr >> 26) == 0x03:
                    target = ((instr & 0x03FFFFFF) << 2) | 0x80000000
                    if target == addr:
                        caller_vram = vram_start + (off - code_start)
                        callers.append(caller_vram)
            print(f"  0x{addr:08X}  (called from: {', '.join(f'0x{c:08X}' for c in callers[:3])})")

    # Print ranges of external targets to understand where the game code lives
    if external_targets:
        sorted_ext = sorted(external_targets)
        print(f"\nExternal target range: 0x{sorted_ext[0]:08X} - 0x{sorted_ext[-1]:08X}")

        # Group into 64KB buckets
        buckets = {}
        for addr in sorted_ext:
            bucket = addr & 0xFFFF0000
            if bucket not in buckets:
                buckets[bucket] = []
            buckets[bucket].append(addr)

        print("\nBy region:")
        for bucket in sorted(buckets.keys()):
            addrs = buckets[bucket]
            print(f"  0x{bucket:08X}: {len(addrs)} targets ({', '.join(f'0x{a:08X}' for a in addrs[:5])}{'...' if len(addrs) > 5 else ''})")

if __name__ == "__main__":
    main()
