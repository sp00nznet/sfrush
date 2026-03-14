// read_sfrush_dump.js — Analyze RDRAM dumps from SF Rush
// Identifies code regions, DMA-loaded segments, and game state
// Usage: node read_sfrush_dump.js <rdram_dump.bin> [rom_path]

const fs = require('fs');
const path = require('path');

const dumpFile = process.argv[2];
const romPath = process.argv[3] || path.join(__dirname, '..', '..', 'baserom.z64');

if (!dumpFile) {
    console.error('Usage: node read_sfrush_dump.js <rdram_dump.bin> [rom_path]');
    process.exit(1);
}

const rdram = fs.readFileSync(dumpFile);
console.log(`RDRAM dump: ${dumpFile} (${rdram.length} bytes = ${(rdram.length / 1024 / 1024).toFixed(1)} MB)`);

let rom = null;
if (fs.existsSync(romPath)) {
    rom = fs.readFileSync(romPath);
    console.log(`ROM: ${romPath} (${rom.length} bytes)`);
}

function readU32(buf, offset) {
    if (offset + 4 > buf.length) return 0;
    return buf.readUInt32BE(offset);
}

function hex(val, width = 8) {
    return '0x' + (val >>> 0).toString(16).toUpperCase().padStart(width, '0');
}

// Read from VRAM address (0x80XXXXXX -> offset 0xXXXXXX)
function readVRAM(vaddr) {
    const offset = (vaddr & 0x7FFFFFFF);
    return readU32(rdram, offset);
}

// ============================================================
// Check known SF Rush addresses
// ============================================================
console.log('\n=== Known SF Rush State ===');
console.log(`osMemSize [80000318]: ${hex(readVRAM(0x80000318))} (${readVRAM(0x80000318) / 1024 / 1024} MB)`);
console.log(`Boot entry [80000400]: ${hex(readVRAM(0x80000400))}`);
console.log(`bootproc [80000450]:   ${hex(readVRAM(0x80000450))}`);

console.log('\n=== Thread entry at 0x800CBE88 (DMA-loaded) ===');
for (let i = 0; i < 8; i++) {
    const addr = 0x800CBE88 + i * 4;
    const val = readVRAM(addr);
    console.log(`  ${hex(addr)}: ${hex(val)}${val === 0 ? '  (empty - not loaded yet?)' : ''}`);
}

// ============================================================
// Scan for code regions by looking for MIPS instruction patterns
// Valid MIPS code has recognizable patterns: ADDIU SP, SW RA, etc.
// ============================================================
console.log('\n=== Scanning for code regions (MIPS instruction density) ===');
console.log('Checking 64KB blocks for JAL instruction density...\n');

const BLOCK_SIZE = 0x10000; // 64KB blocks
const codeRegions = [];

for (let blockStart = 0; blockStart < rdram.length; blockStart += BLOCK_SIZE) {
    let jalCount = 0;
    let instrCount = 0;
    let zeroCount = 0;

    for (let off = blockStart; off < blockStart + BLOCK_SIZE && off + 4 <= rdram.length; off += 4) {
        const w = readU32(rdram, off);
        if (w === 0) { zeroCount++; continue; }
        instrCount++;

        const op = (w >>> 26) & 0x3F;
        // JAL = opcode 3
        if (op === 3) jalCount++;
    }

    const totalWords = BLOCK_SIZE / 4;
    const zeroPct = ((zeroCount / totalWords) * 100).toFixed(0);
    const vramBase = 0x80000000 + blockStart;

    if (jalCount > 10) {
        codeRegions.push({ vram: vramBase, jals: jalCount, instrs: instrCount, zeroPct });
        console.log(`  ${hex(vramBase)}: ${jalCount} JALs, ${instrCount} non-zero words, ${zeroPct}% zeros  ** CODE **`);
    } else if (instrCount > 1000 && zeroCount < totalWords * 0.9) {
        console.log(`  ${hex(vramBase)}: ${jalCount} JALs, ${instrCount} non-zero words, ${zeroPct}% zeros  (data?)`);
    }
}

// ============================================================
// Identify contiguous code regions
// ============================================================
console.log('\n=== Contiguous code regions ===');
if (codeRegions.length > 0) {
    let regionStart = codeRegions[0].vram;
    let regionEnd = regionStart + BLOCK_SIZE;
    let totalJals = codeRegions[0].jals;

    for (let i = 1; i < codeRegions.length; i++) {
        if (codeRegions[i].vram === regionEnd) {
            regionEnd = codeRegions[i].vram + BLOCK_SIZE;
            totalJals += codeRegions[i].jals;
        } else {
            const size = regionEnd - regionStart;
            console.log(`  ${hex(regionStart)} - ${hex(regionEnd)}  (${(size / 1024).toFixed(0)} KB, ${totalJals} JALs)`);
            regionStart = codeRegions[i].vram;
            regionEnd = regionStart + BLOCK_SIZE;
            totalJals = codeRegions[i].jals;
        }
    }
    const size = regionEnd - regionStart;
    console.log(`  ${hex(regionStart)} - ${hex(regionEnd)}  (${(size / 1024).toFixed(0)} KB, ${totalJals} JALs)`);
}

// ============================================================
// If we have the ROM, try to match RDRAM content to ROM offsets
// For each code region in RDRAM, search the ROM for matching data
// ============================================================
if (rom) {
    console.log('\n=== ROM offset matching for code regions ===');
    console.log('Searching ROM for 16-byte sequences from each code region...\n');

    for (const region of codeRegions) {
        const rdramOff = (region.vram & 0x7FFFFFFF);
        // Take 16 bytes from the start of this block
        const sample = rdram.slice(rdramOff, rdramOff + 16);

        if (sample.every(b => b === 0)) continue;

        // Search ROM for this 16-byte sequence
        let found = false;
        for (let romOff = 0; romOff <= rom.length - 16; romOff += 4) {
            if (rom.compare(sample, 0, 16, romOff, romOff + 16) === 0) {
                console.log(`  VRAM ${hex(region.vram)} matches ROM ${hex(romOff)}  (delta: ${hex(romOff - rdramOff)})`);
                found = true;
                break;
            }
        }
        if (!found) {
            console.log(`  VRAM ${hex(region.vram)} — no ROM match (decompressed/generated?)`);
        }
    }
}

// ============================================================
// Check the DMA table area — see if it was loaded into RDRAM
// bootproc reads from ROM 0x00FFB000
// ============================================================
console.log('\n=== Looking for DMA table in RDRAM ===');
console.log('Searching for copies of ROM 0x00FFB000 data in RDRAM...\n');

if (rom) {
    const dmaTableSample = rom.slice(0x00FFB000, 0x00FFB000 + 16);
    console.log(`DMA table first 16 bytes in ROM: ${Array.from(dmaTableSample).map(b => b.toString(16).padStart(2, '0')).join(' ')}`);

    for (let rdramOff = 0; rdramOff <= rdram.length - 16; rdramOff += 4) {
        if (rdram.compare(dmaTableSample, 0, 16, rdramOff, rdramOff + 16) === 0) {
            console.log(`  Found DMA table copy at RDRAM offset ${hex(rdramOff)} (VRAM ${hex(0x80000000 + rdramOff)})`);
        }
    }
}

console.log('\n=== Done ===');
console.log('Use parse_dma_table.js to analyze the DMA table format in the ROM directly.');
