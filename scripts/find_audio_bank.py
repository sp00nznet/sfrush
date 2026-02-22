#!/usr/bin/env python3
"""
San Francisco Rush: Extreme Racing - Audio Bank Finder

Scans the ROM for SN64 audio bank signatures.
Midway's SN64 sound engine stores audio data in a specific format.
Cross-references with Rampage's known audio bank location (0x906290)
to identify similar patterns in SF Rush.

Usage:
    python find_audio_bank.py <rom.z64>
"""

import struct
import sys


def read_be32(data, offset):
    return struct.unpack('>I', data[offset:offset+4])[0]


def read_be16(data, offset):
    return struct.unpack('>H', data[offset:offset+2])[0]


def scan_for_audio_signatures(rom_data):
    """
    Look for patterns common to SN64 audio banks:
    - Sample rate tables (common values: 22050, 44100, 11025)
    - ADPCM codebook headers
    - Audio bank header structures
    """
    results = []

    # Scan for potential audio bank headers
    # SN64 banks often start with a count followed by offset table
    for offset in range(0, len(rom_data) - 16, 4):
        val = read_be32(rom_data, offset)

        # Look for reasonable bank counts (1-256 sounds)
        if 1 <= val <= 256:
            # Check if followed by ascending offsets (offset table)
            likely_table = True
            prev_off = 0
            for i in range(1, min(val + 1, 17)):
                entry = read_be32(rom_data, offset + i * 4)
                if entry < prev_off or entry > len(rom_data):
                    likely_table = False
                    break
                prev_off = entry

            if likely_table and val >= 4:
                results.append((offset, val, "potential_bank_header"))

    # Scan for ADPCM predictor tables
    # These typically have specific patterns of 16-bit values
    for offset in range(0, len(rom_data) - 32, 2):
        # ADPCM books often have 8 pairs of 16-bit predictors
        vals = [read_be16(rom_data, offset + i*2) for i in range(16)]
        # Check for typical predictor range
        in_range = all(-32768 <= (v if v < 32768 else v - 65536) <= 32767 for v in vals)
        non_zero = sum(1 for v in vals if v != 0)

        if in_range and 4 <= non_zero <= 14:
            # Could be an ADPCM codebook
            pass  # Too many false positives to report all

    return results


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <rom.z64>")
        sys.exit(1)

    rom_path = sys.argv[1]
    with open(rom_path, 'rb') as f:
        rom_data = f.read()

    print(f"ROM: {rom_path} ({len(rom_data)} bytes)")
    print(f"Scanning for SN64 audio bank signatures...\n")

    results = scan_for_audio_signatures(rom_data)

    print(f"Found {len(results)} potential audio bank headers:\n")
    for offset, count, sig_type in results[:50]:  # Show top 50
        print(f"  0x{offset:08X}: count={count:3d}  ({sig_type})")

    if len(results) > 50:
        print(f"  ... and {len(results) - 50} more")

    print(f"\nNote: Compare with Rampage's audio bank at ROM offset 0x906290")
    print(f"Cross-reference function calls to narrow down the actual audio bank.")


if __name__ == "__main__":
    main()
