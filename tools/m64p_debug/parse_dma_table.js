// parse_dma_table.js — Parse the DMA table from the SF Rush ROM
// The boot code at 0x80000450 reads from ROM 0x00FFB000 in 16 iterations.
// This script reads the ROM directly to identify DMA segment entries.
//
// N64 DMA tables typically contain entries like:
//   { vram_start, vram_end, rom_start, rom_end }  (each 4 bytes, 16 bytes per entry)
// or sometimes:
//   { rom_start, rom_end, vram_start }  (12 bytes per entry)
//
// We'll try multiple formats and see what makes sense.
//
// Usage: node parse_dma_table.js [rom_path]

const fs = require('fs');
const path = require('path');

const romPath = process.argv[2] || path.join(__dirname, '..', '..', 'baserom.z64');

if (!fs.existsSync(romPath)) {
    console.error(`ROM not found: ${romPath}`);
    process.exit(1);
}

const rom = fs.readFileSync(romPath);
console.log(`ROM: ${romPath} (${rom.length} bytes = ${(rom.length / 1024 / 1024).toFixed(1)} MB)`);

function readU32(offset) {
    if (offset + 4 > rom.length) return 0;
    return rom.readUInt32BE(offset);
}

function readU16(offset) {
    if (offset + 2 > rom.length) return 0;
    return rom.readUInt16BE(offset);
}

// Hex formatter
function hex(val, width = 8) {
    return '0x' + (val >>> 0).toString(16).toUpperCase().padStart(width, '0');
}

// ============================================================
// Scan the DMA table area at ROM 0x00FFB000
// ============================================================
const DMA_TABLE_OFFSET = 0x00FFB000;
const SCAN_SIZE = 0x1000; // scan 4KB around the table

console.log(`\n=== Raw data at ROM ${hex(DMA_TABLE_OFFSET)} ===`);
console.log('(first 256 bytes as 32-bit words)\n');

for (let i = 0; i < 256; i += 4) {
    const off = DMA_TABLE_OFFSET + i;
    const val = readU32(off);
    if (i % 16 === 0) process.stdout.write(`  ${hex(off)}: `);
    process.stdout.write(`${hex(val)} `);
    if (i % 16 === 12) process.stdout.write('\n');
}

// ============================================================
// Try Format A: 16-byte entries { vram_start, vram_end, rom_start, rom_end }
// This is the standard N64 DMA table format (used by Zelda, etc.)
// ============================================================
console.log('\n=== Format A: 16-byte entries {vram_start, vram_end, rom_start, rom_end} ===\n');

let validA = 0;
for (let i = 0; i < 64; i++) {
    const base = DMA_TABLE_OFFSET + i * 16;
    const vramStart = readU32(base + 0);
    const vramEnd   = readU32(base + 4);
    const romStart  = readU32(base + 8);
    const romEnd    = readU32(base + 12);

    // Validity checks
    const vramOk = (vramStart >= 0x80000000 && vramStart < 0x80800000);
    const sizeOk = (vramEnd > vramStart && (vramEnd - vramStart) < 0x800000);
    const romOk  = (romStart < rom.length && romEnd <= rom.length && romEnd > romStart);

    if (vramStart === 0 && vramEnd === 0 && romStart === 0 && romEnd === 0) {
        if (validA > 0) {
            console.log(`  [${i}] --- end of table (all zeros) ---`);
            break;
        }
        continue;
    }

    const tag = (vramOk && sizeOk && romOk) ? '  OK' : ' ???';
    const size = vramOk && sizeOk ? vramEnd - vramStart : 0;
    console.log(`  [${i.toString().padStart(2)}]${tag}  VRAM: ${hex(vramStart)}-${hex(vramEnd)}  ROM: ${hex(romStart)}-${hex(romEnd)}  size: ${size > 0 ? (size / 1024).toFixed(1) + 'KB' : '?'}`);
    if (vramOk && sizeOk && romOk) validA++;
}
console.log(`  (${validA} valid entries)`);

// ============================================================
// Try Format B: 12-byte entries { rom_start, vram_start, size }
// ============================================================
console.log('\n=== Format B: 12-byte entries {rom_start, vram_start, size} ===\n');

let validB = 0;
for (let i = 0; i < 64; i++) {
    const base = DMA_TABLE_OFFSET + i * 12;
    const romStart  = readU32(base + 0);
    const vramStart = readU32(base + 4);
    const size      = readU32(base + 8);

    const vramOk = (vramStart >= 0x80000000 && vramStart < 0x80800000);
    const sizeOk = (size > 0 && size < 0x800000);
    const romOk  = (romStart < rom.length && romStart + size <= rom.length);

    if (romStart === 0 && vramStart === 0 && size === 0) {
        if (validB > 0) {
            console.log(`  [${i}] --- end of table ---`);
            break;
        }
        continue;
    }

    const tag = (vramOk && sizeOk && romOk) ? '  OK' : ' ???';
    console.log(`  [${i.toString().padStart(2)}]${tag}  ROM: ${hex(romStart)}  VRAM: ${hex(vramStart)}  size: ${sizeOk ? (size / 1024).toFixed(1) + 'KB' : hex(size)}`);
    if (vramOk && sizeOk && romOk) validB++;
}
console.log(`  (${validB} valid entries)`);

// ============================================================
// Try Format C: 12-byte entries { vram_start, rom_start, size }
// ============================================================
console.log('\n=== Format C: 12-byte entries {vram_start, rom_start, size} ===\n');

let validC = 0;
for (let i = 0; i < 64; i++) {
    const base = DMA_TABLE_OFFSET + i * 12;
    const vramStart = readU32(base + 0);
    const romStart  = readU32(base + 4);
    const size      = readU32(base + 8);

    const vramOk = (vramStart >= 0x80000000 && vramStart < 0x80800000);
    const sizeOk = (size > 0 && size < 0x800000);
    const romOk  = (romStart < rom.length && romStart + size <= rom.length);

    if (vramStart === 0 && romStart === 0 && size === 0) {
        if (validC > 0) {
            console.log(`  [${i}] --- end of table ---`);
            break;
        }
        continue;
    }

    const tag = (vramOk && sizeOk && romOk) ? '  OK' : ' ???';
    console.log(`  [${i.toString().padStart(2)}]${tag}  VRAM: ${hex(vramStart)}  ROM: ${hex(romStart)}  size: ${sizeOk ? (size / 1024).toFixed(1) + 'KB' : hex(size)}`);
    if (vramOk && sizeOk && romOk) validC++;
}
console.log(`  (${validC} valid entries)`);

// ============================================================
// Also scan the boot code for osPiStartDma calls that reference the DMA table
// Look for LUI+ADDIU pairs that load 0x00FFB000 or nearby
// ============================================================
console.log('\n=== Scanning boot segment for references to DMA table area ===');
console.log('Looking for LUI/ORI/ADDIU loading values near 0x00FF or 0x800C...\n');

const BOOT_START = 0x1000;
const BOOT_END = 0x1CA00;

for (let off = BOOT_START; off < BOOT_END; off += 4) {
    const instr = readU32(off);
    const op = (instr >>> 26) & 0x3F;
    const rt = (instr >>> 16) & 0x1F;
    const imm = instr & 0xFFFF;

    // LUI rt, imm
    if (op === 0x0F) {
        if (imm === 0x00FF || imm === 0x0100 || imm === 0x800C || imm === 0x800D) {
            const vram = 0x80000400 + (off - 0x1000);
            // Look ahead for ADDIU/ORI on same register
            const next = readU32(off + 4);
            const nextOp = (next >>> 26) & 0x3F;
            const nextRs = (next >>> 21) & 0x1F;
            const nextRt = (next >>> 16) & 0x1F;
            const nextImm = next & 0xFFFF;

            let fullAddr = imm << 16;
            if ((nextOp === 0x09 || nextOp === 0x0D) && nextRs === rt) {
                // ADDIU or ORI
                if (nextOp === 0x09) {
                    // ADDIU: sign-extend
                    fullAddr += (nextImm & 0x8000) ? (nextImm | 0xFFFF0000) : nextImm;
                } else {
                    fullAddr |= nextImm;
                }
            }

            console.log(`  ${hex(vram)}: LUI r${rt}, ${hex(imm, 4)} -> ${hex(fullAddr >>> 0)}  (ROM offset ${hex(off)})`);
        }
    }
}

// ============================================================
// Check what MIPS instructions are at 0x800CBE88 location in ROM
// The boot segment maps 0x80000400 = ROM 0x1000
// So 0x800CBE88 = ROM 0x1000 + (0x800CBE88 - 0x80000400) = ROM 0xCBA88
// But this is DMA-loaded, so it might not be at that ROM offset
// Let's check anyway
// ============================================================
const threadEntryRomGuess = 0x1000 + (0x800CBE88 - 0x80000400);
console.log(`\n=== Checking ROM ${hex(threadEntryRomGuess)} (linear map of 0x800CBE88) ===`);
for (let i = 0; i < 8; i++) {
    const w = readU32(threadEntryRomGuess + i * 4);
    console.log(`  ${hex(threadEntryRomGuess + i * 4)}: ${hex(w)}`);
}

// ============================================================
// Broader scan: look for the 16-iteration loop structure
// bootproc reads from 0x00FFB000 in 16 iterations
// Each iteration likely reads an entry and does a PI DMA
// Let's find what stride/size the loop uses
// ============================================================
console.log('\n=== Scanning for loop counter patterns near DMA table references ===');
// Look for SLTI/BNE with value 16 (0x10) or ADDIU with 16/12/20 near the FFB000 references
for (let off = BOOT_START; off < BOOT_END; off += 4) {
    const instr = readU32(off);
    const op = (instr >>> 26) & 0x3F;
    const imm = instr & 0xFFFF;

    // SLTI rt, rs, 16 or ADDIU rt, rs, 16/12/20
    if (op === 0x0A && imm === 0x0010) { // SLTI with 16
        const vram = 0x80000400 + (off - 0x1000);
        console.log(`  ${hex(vram)}: SLTI r${(instr>>>16)&0x1F}, r${(instr>>>21)&0x1F}, 16  (ROM ${hex(off)})`);
    }
    if (op === 0x09 && (imm === 0x0010 || imm === 0x000C || imm === 0x0014)) { // ADDIU with 16/12/20
        const vram = 0x80000400 + (off - 0x1000);
        const rs = (instr >>> 21) & 0x1F;
        const rt = (instr >>> 16) & 0x1F;
        if (rs === rt) { // self-increment (loop counter)
            console.log(`  ${hex(vram)}: ADDIU r${rt}, r${rs}, ${imm}  (self-increment, ROM ${hex(off)})`);
        }
    }
}

console.log('\n=== Summary ===');
console.log(`Format A valid: ${validA}, Format B valid: ${validB}, Format C valid: ${validC}`);
const best = validA >= validB && validA >= validC ? 'A' : validB >= validC ? 'B' : 'C';
console.log(`Best guess: Format ${best}`);
console.log('\nNext steps:');
console.log('1. Run this and check which format produces valid entries');
console.log('2. Use sfrush_dump.js to dump RDRAM and confirm code is at expected VRAM addresses');
console.log('3. Use read_sfrush_dump.js on the RDRAM dumps to identify loaded code regions');
