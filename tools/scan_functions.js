// scan_functions.js — Scan decompressed game code to find MIPS function boundaries
//
// Looks for function prologues (ADDIU SP, -N / SW RA / JR RA patterns)
// and JAL targets to identify function entry points.
//
// Usage: node scan_functions.js [decompressed_bin] [vram_base]

const fs = require('fs');
const path = require('path');

const binPath = process.argv[2] || path.join(__dirname, '..', 'game_code_decompressed.bin');
const vramBase = parseInt(process.argv[3]) || 0x8005BB10;

const code = fs.readFileSync(binPath);
console.log(`Code: ${code.length} bytes, VRAM base: 0x${vramBase.toString(16)}`);

function readU32(offset) {
    if (offset + 4 > code.length) return 0;
    return code.readUInt32BE(offset);
}

function hex(v, w=8) { return '0x' + (v>>>0).toString(16).padStart(w, '0'); }

// Step 1: Find all JAL targets within the game code range
const jalTargets = new Set();
const vramEnd = vramBase + code.length;

for (let off = 0; off + 3 < code.length; off += 4) {
    const instr = readU32(off);
    const op = (instr >>> 26) & 0x3F;
    if (op === 3) { // JAL
        const target = (((instr & 0x3FFFFFF) << 2) | 0x80000000) >>> 0;
        if (target >= vramBase && target < vramEnd) {
            jalTargets.add(target);
        }
    }
}

console.log(`Found ${jalTargets.size} unique JAL targets within game code range`);

// Step 2: Find function prologues
// Pattern: ADDIU SP, SP, -N (op=9, rs=29, rt=29, imm<0)
const prologues = new Set();
for (let off = 0; off + 3 < code.length; off += 4) {
    const instr = readU32(off);
    // ADDIU SP, SP, -N
    if ((instr & 0xFFFF0000) === 0x27BD0000) {
        const imm = instr & 0xFFFF;
        if (imm >= 0x8000) { // negative immediate (sign-extended)
            const vram = vramBase + off;
            prologues.add(vram);
        }
    }
}

console.log(`Found ${prologues.size} stack frame prologues`);

// Step 3: Combine JAL targets and prologues
// A function is a JAL target that either has a prologue or is aligned
const functions = new Set();

// All JAL targets are potential functions
for (const target of jalTargets) {
    functions.add(target);
}

// Prologues that aren't already JAL targets might be leaf functions or
// functions only called via function pointers
for (const addr of prologues) {
    if (!functions.has(addr)) {
        // Check if it looks like a real function (not in the middle of another)
        // Check if the previous instruction is a NOP, JR RA, or the start of code
        const prevOff = (addr - vramBase) - 4;
        if (prevOff < 0) {
            functions.add(addr);
            continue;
        }
        const prevInstr = readU32(prevOff);
        // NOP, JR RA (0x03E00008), or branch delay NOP after JR
        if (prevInstr === 0 || prevInstr === 0x03E00008) {
            functions.add(addr);
        }
    }
}

// Also add the first address (start of section) if not already present
functions.add(vramBase);

// Sort functions
const sorted = Array.from(functions).sort((a, b) => a - b);

console.log(`Total functions identified: ${sorted.length}`);

// Step 4: Calculate sizes and generate TOML output
const tomlLines = [];
tomlLines.push(`# Game code section - ${sorted.length} functions`);
tomlLines.push(`# Auto-generated from decompressed game code`);
tomlLines.push(`# LZSS decompressed from ROM 0x7A7930`);
tomlLines.push(`# Placed at ROM 0x5C710 in patched sfrush_recomp.z64`);
tomlLines.push(`# BSS: 0x800D8060-0x8016A9B0 (zeroed, not in ROM)`);
tomlLines.push('');
tomlLines.push('[[section]]');
tomlLines.push('name = "game"');
tomlLines.push('rom = 0x5C710');
tomlLines.push(`vram = 0x${vramBase.toString(16).toUpperCase()}`);
tomlLines.push(`size = 0x${code.length.toString(16).toUpperCase()}`);
tomlLines.push('');

for (let i = 0; i < sorted.length; i++) {
    const addr = sorted[i];
    const nextAddr = (i + 1 < sorted.length) ? sorted[i + 1] : vramBase + code.length;
    const size = nextAddr - addr;

    tomlLines.push('[[section.functions]]');
    tomlLines.push(`name = "func_${addr.toString(16).toUpperCase()}"`);
    tomlLines.push(`vram = 0x${addr.toString(16).toUpperCase()}`);
    tomlLines.push(`size = 0x${size.toString(16).toUpperCase()}`);
    tomlLines.push('');
}

const tomlOutput = tomlLines.join('\n');
const tomlPath = path.join(__dirname, '..', 'symbols', 'sfrush.game.syms.toml');
fs.writeFileSync(tomlPath, tomlOutput);
console.log(`\nWrote symbols to: ${tomlPath}`);
console.log(`${sorted.length} functions defined`);

// Stats
const jalOnlyCount = Array.from(jalTargets).filter(t => !prologues.has(t)).length;
const prologueOnlyCount = Array.from(prologues).filter(p => !jalTargets.has(p) && functions.has(p)).length;
const bothCount = Array.from(jalTargets).filter(t => prologues.has(t)).length;
console.log(`\nBreakdown:`);
console.log(`  JAL targets with prologue: ${bothCount}`);
console.log(`  JAL targets without prologue: ${jalOnlyCount} (leaf functions or stubs)`);
console.log(`  Prologues not JAL targets: ${prologueOnlyCount} (func ptr / internal)`);
