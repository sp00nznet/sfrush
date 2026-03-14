// decompress_game_code.js — Extract and decompress SF Rush main game code from ROM
//
// The game uses LZSS compression. The boot code at func_800005D8 calls
// func_80003B18(rom_src=0x7A7930, vram_dst=0x8005BB10) to decompress
// the main game code from ROM into RDRAM.
//
// Algorithm (from full disassembly of func_80003B18):
//   - 4096-byte sliding window (circular buffer, index masked with 0x0FFF)
//   - window[0] = 0x00, window_pos starts at 1
//   - Control byte processed LSB first, shifted right after each bit
//   - bit=1: literal byte copy (1 byte from input → output + window)
//   - bit=0: backreference (2 bytes):
//     - b0: high nibble = bits 11:8 of offset, low nibble = length - 2
//     - b1: bits 7:0 of offset
//     - offset = ((b0 & 0xF0) << 4) | b1
//     - length = (b0 & 0x0F) + 2
//     - if b0==0 && b1==0: end of stream
//     - copies 'length' bytes from window[offset..] to output + window
//
// Usage: node decompress_game_code.js [rom_path] [output_path]

const fs = require('fs');
const path = require('path');

const romPath = process.argv[2] || path.join(__dirname, '..', 'baserom.z64');
const outputPath = process.argv[3] || path.join(__dirname, '..', 'game_code_decompressed.bin');

if (!fs.existsSync(romPath)) {
    console.error(`ROM not found: ${romPath}`);
    process.exit(1);
}

const rom = fs.readFileSync(romPath);
console.log(`ROM: ${romPath} (${rom.length} bytes)`);

// Constants from the disassembly
const COMPRESSED_ROM_OFFSET = 0x7A7930;
const DECOMPRESSED_VRAM_START = 0x8005BB10;
const DECOMPRESSED_VRAM_END   = 0x800D8060;
const EXPECTED_DECOMP_SIZE    = DECOMPRESSED_VRAM_END - DECOMPRESSED_VRAM_START; // 0x7C550 = 509,264 bytes

console.log(`Compressed data at ROM: 0x${COMPRESSED_ROM_OFFSET.toString(16)}`);
console.log(`Expected decompressed size: ${EXPECTED_DECOMP_SIZE} bytes (0x${EXPECTED_DECOMP_SIZE.toString(16)})`);
console.log(`VRAM: 0x${DECOMPRESSED_VRAM_START.toString(16)} - 0x${DECOMPRESSED_VRAM_END.toString(16)}`);

// LZSS decompression — exact match to func_80003B18
function decompressLZSS(compressedData) {
    const WINDOW_SIZE = 4096; // 0x1000
    const WINDOW_MASK = 0x0FFF;

    const window = Buffer.alloc(WINDOW_SIZE, 0);
    let windowPos = 1; // window[0] = 0, pos starts at 1

    const output = [];
    let inPos = 0;

    outer:
    while (inPos < compressedData.length) {
        // Read control byte
        let controlByte = compressedData[inPos++];

        for (let bit = 0; bit < 8; bit++) {
            if (controlByte & 1) {
                // Bit 1: literal byte
                if (inPos >= compressedData.length) break outer;
                const byte = compressedData[inPos++];
                output.push(byte);
                window[windowPos] = byte;
                windowPos = (windowPos + 1) & WINDOW_MASK;
            } else {
                // Bit 0: backreference (2 bytes)
                if (inPos + 1 >= compressedData.length) break outer;
                const b0 = compressedData[inPos++];
                const b1 = compressedData[inPos++];

                // offset = (high nibble of b0 << 8) | b1
                const offset = ((b0 & 0xF0) << 4) | b1;
                // length = low nibble of b0 + 2
                const lengthBase = b0 & 0x0F;
                const length = lengthBase + 2;

                // End-of-stream: both offset and lengthBase are 0
                if (offset === 0 && lengthBase === 0) {
                    break outer;
                }

                // Copy from window
                let srcPos = offset;
                for (let j = 0; j < length; j++) {
                    const byte = window[srcPos & WINDOW_MASK];
                    output.push(byte);
                    window[windowPos] = byte;
                    windowPos = (windowPos + 1) & WINDOW_MASK;
                    srcPos++;
                }
            }

            // Shift control byte right for next bit
            controlByte >>>= 1;
        }
    }

    return Buffer.from(output);
}

// Extract compressed data from ROM
const compressedData = rom.slice(COMPRESSED_ROM_OFFSET);
console.log(`Compressed data available: ${compressedData.length} bytes`);

console.log('\nDecompressing...');
const result = decompressLZSS(compressedData);
console.log(`Decompressed: ${result.length} bytes (expected ${EXPECTED_DECOMP_SIZE})`);

if (result.length > 0) {
    // Check if output looks like MIPS code
    let jalCount = 0;
    let swCount = 0;
    let lwCount = 0;
    for (let i = 0; i + 3 < result.length; i += 4) {
        const w = result.readUInt32BE(i);
        const op = (w >>> 26) & 0x3F;
        if (op === 3) jalCount++;   // JAL
        if (op === 0x2B) swCount++; // SW
        if (op === 0x23) lwCount++; // LW
    }
    console.log(`\nInstruction analysis:`);
    console.log(`  JAL: ${jalCount}  SW: ${swCount}  LW: ${lwCount}`);

    // First 16 words
    console.log(`\nFirst 32 bytes (VRAM 0x${DECOMPRESSED_VRAM_START.toString(16)}):`);
    for (let i = 0; i < 32 && i < result.length; i += 4) {
        const w = result.readUInt32BE(i);
        process.stdout.write('  0x' + (w>>>0).toString(16).padStart(8,'0'));
        if (i % 16 === 12) process.stdout.write('\n');
    }
    console.log('');

    // Check thread entry point
    const threadEntryOffset = 0x800BBE88 - DECOMPRESSED_VRAM_START;
    if (threadEntryOffset >= 0 && threadEntryOffset + 16 <= result.length) {
        console.log(`\nThread entry at VRAM 0x800BBE88 (offset 0x${threadEntryOffset.toString(16)}):`);
        for (let i = 0; i < 16; i += 4) {
            const w = result.readUInt32BE(threadEntryOffset + i);
            process.stdout.write('  0x' + (w>>>0).toString(16).padStart(8,'0'));
        }
        console.log('');
        const entryOp = (result.readUInt32BE(threadEntryOffset) >>> 26) & 0x3F;
        console.log(`  First instruction opcode: ${entryOp} (${entryOp === 0x09 ? 'ADDIU' : entryOp === 0x0F ? 'LUI' : entryOp === 0 ? 'SPECIAL' : '?'})`);
    }

    // Check func_800BD440 (first call into loaded code)
    const bd440Offset = 0x800BD440 - DECOMPRESSED_VRAM_START;
    if (bd440Offset >= 0 && bd440Offset + 16 <= result.length) {
        console.log(`\nfunc_800BD440 (offset 0x${bd440Offset.toString(16)}):`);
        for (let i = 0; i < 16; i += 4) {
            const w = result.readUInt32BE(bd440Offset + i);
            process.stdout.write('  0x' + (w>>>0).toString(16).padStart(8,'0'));
        }
        console.log('');
    }

    // Write output
    fs.writeFileSync(outputPath, result);
    console.log(`\nWrote decompressed game code to: ${outputPath}`);
    console.log(`Size: ${result.length} bytes (${(result.length/1024).toFixed(1)} KB)`);

    if (jalCount > 500) {
        console.log('\n*** Decompression looks SUCCESSFUL — high JAL count indicates valid MIPS code ***');
    } else {
        console.log('\n*** WARNING: Low JAL count — decompression may be incorrect ***');
    }
}
