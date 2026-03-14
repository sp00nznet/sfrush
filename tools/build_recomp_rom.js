// build_recomp_rom.js — Build a patched ROM for N64Recomp
//
// Places the decompressed game code at the correct ROM offset so N64Recomp
// can process it. The patched ROM extends the original to include the
// decompressed code at the linear VRAM-to-ROM mapping.
//
// Memory map:
//   Boot code:  VRAM 0x80000400 → ROM 0x1000 (original, 113 KB)
//   Game code:  VRAM 0x8005BB10 → ROM 0x5C710 (decompressed, 497 KB)
//
// The ROM formula: rom_offset = vram - 0x80000400 + 0x1000
//
// Usage: node build_recomp_rom.js [rom_path] [decompressed_path] [output_path]

const fs = require('fs');
const path = require('path');

const romPath = process.argv[2] || path.join(__dirname, '..', 'baserom.z64');
const decompPath = process.argv[3] || path.join(__dirname, '..', 'game_code_decompressed.bin');
const outputPath = process.argv[4] || path.join(__dirname, '..', 'sfrush_recomp.z64');

if (!fs.existsSync(romPath)) { console.error(`ROM not found: ${romPath}`); process.exit(1); }
if (!fs.existsSync(decompPath)) { console.error(`Decompressed code not found: ${decompPath}`); process.exit(1); }

const rom = fs.readFileSync(romPath);
const decomp = fs.readFileSync(decompPath);

console.log(`Original ROM: ${rom.length} bytes`);
console.log(`Decompressed game code: ${decomp.length} bytes`);

// Calculate ROM offset for game code
const VRAM_BASE = 0x80000400;
const ROM_BASE  = 0x1000;
const GAME_VRAM = 0x8005BB10;
const GAME_ROM_OFFSET = GAME_VRAM - VRAM_BASE + ROM_BASE; // 0x5C710

console.log(`Game code VRAM: 0x${GAME_VRAM.toString(16)}`);
console.log(`Game code ROM offset: 0x${GAME_ROM_OFFSET.toString(16)}`);
console.log(`Game code end ROM offset: 0x${(GAME_ROM_OFFSET + decomp.length).toString(16)}`);

// The patched ROM needs to be at least large enough to hold the decompressed code
const neededSize = GAME_ROM_OFFSET + decomp.length;
const patchedSize = Math.max(rom.length, neededSize);

console.log(`Patched ROM size: ${patchedSize} bytes (${(patchedSize/1024/1024).toFixed(2)} MB)`);

// Create patched ROM
const patched = Buffer.alloc(patchedSize, 0);

// Copy original ROM
rom.copy(patched, 0, 0, rom.length);

// Place decompressed game code at the correct offset
decomp.copy(patched, GAME_ROM_OFFSET, 0, decomp.length);

// Verify the placement
console.log(`\nVerification:`);
console.log(`  Boot code at ROM 0x1000: 0x${patched.readUInt32BE(0x1000).toString(16).padStart(8,'0')}`);
console.log(`  Game code at ROM 0x${GAME_ROM_OFFSET.toString(16)}: 0x${patched.readUInt32BE(GAME_ROM_OFFSET).toString(16).padStart(8,'0')}`);

// Check some known functions
const threadEntryRom = 0x800BBE88 - VRAM_BASE + ROM_BASE;
console.log(`  Thread entry (0x800BBE88) at ROM 0x${threadEntryRom.toString(16)}: 0x${patched.readUInt32BE(threadEntryRom).toString(16).padStart(8,'0')}`);

const bd440Rom = 0x800BD440 - VRAM_BASE + ROM_BASE;
console.log(`  func_800BD440 at ROM 0x${bd440Rom.toString(16)}: 0x${patched.readUInt32BE(bd440Rom).toString(16).padStart(8,'0')}`);

// Check for overlap with boot code
const bootEnd = 0x1CA00;
if (GAME_ROM_OFFSET < bootEnd) {
    console.log(`\n  WARNING: Game code overlaps with boot code! Boot ends at 0x${bootEnd.toString(16)}, game starts at 0x${GAME_ROM_OFFSET.toString(16)}`);
} else {
    console.log(`\n  OK: No overlap. Boot ends at 0x${bootEnd.toString(16)}, game starts at 0x${GAME_ROM_OFFSET.toString(16)} (gap: ${(GAME_ROM_OFFSET - bootEnd)} bytes)`);
}

// Write patched ROM
fs.writeFileSync(outputPath, patched);
console.log(`\nWrote patched ROM: ${outputPath}`);
console.log(`\nNext steps:`);
console.log(`  1. Update sfrush.recomp.toml to add the game code section`);
console.log(`  2. Run N64Recomp on sfrush_recomp.z64`);
console.log(`  3. Fix any recomp errors (stubs, static functions, etc.)`);
