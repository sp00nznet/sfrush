// identify_libultra.js — Identify libultra functions in the boot segment
// by matching known instruction patterns and cross-referencing with
// the list of functions reimplemented by librecomp/ultramodern.
//
// Usage: node identify_libultra.js [rom_path]

const fs = require('fs');
const path = require('path');

const romPath = process.argv[2] || path.join(__dirname, '..', 'baserom.z64');
const symsPath = path.join(__dirname, '..', 'symbols', 'sfrush.us.syms.toml');

const rom = fs.readFileSync(romPath);
const syms = fs.readFileSync(symsPath, 'utf8');

function readU32(off) { return rom.readUInt32BE(off); }
function hex(v, w=8) { return '0x' + (v>>>0).toString(16).toUpperCase().padStart(w, '0'); }

// Parse boot section functions from symbols TOML
const funcRegex = /\[\[section\.functions\]\]\s*\nname = "func_([0-9A-Fa-f]+)"\s*\nvram = (0x[0-9A-Fa-f]+)\s*\nsize = (0x[0-9A-Fa-f]+)/g;
const bootFuncs = [];
let m;
while ((m = funcRegex.exec(syms)) !== null) {
    const vram = parseInt(m[2]);
    const size = parseInt(m[3]);
    if (vram >= 0x80000400 && vram < 0x80020000) { // boot section only
        bootFuncs.push({ name: `func_${m[1]}`, vram, size });
    }
}
console.log(`Boot section: ${bootFuncs.length} functions`);

// Known libultra function signatures (instruction patterns at entry)
// These are well-known patterns from N64 OS analysis
const signatures = {
    // osCreateThread: saves RA, stores args to thread struct
    // Pattern: LUI at, 0x8003 or similar followed by specific struct operations

    // osCreateMesgQueue: very small, stores args to queue struct
    // Pattern: SW a1, 0x8(a0); SW zero, 0x0(a0); ...

    // osRecvMesg: reads from message queue
    // Pattern: LW from queue struct + complex branching

    // osViSetMode: writes to VI globals
    // Pattern: LUI loading VI register addresses (0xA440)

    // PI DMA functions: write to 0xA4600000-0xA460000C
    // Pattern: LUI r, 0xA460

    // Hardware register access patterns
    'VI_ACCESS': { pattern: [0x3C01A440], mask: [0xFFFF0000], desc: 'VI register access (0xA440xxxx)' },
    'PI_ACCESS': { pattern: [0x3C01A460], mask: [0xFFFF0000], desc: 'PI register access (0xA460xxxx)' },
    'SI_ACCESS': { pattern: [0x3C01A480], mask: [0xFFFF0000], desc: 'SI register access (0xA480xxxx)' },
    'AI_ACCESS': { pattern: [0x3C01A450], mask: [0xFFFF0000], desc: 'AI register access (0xA450xxxx)' },
    'SP_ACCESS': { pattern: [0x3C01A404], mask: [0xFFFF0000], desc: 'SP register access (0xA404xxxx)' },
    'MI_ACCESS': { pattern: [0x3C01A430], mask: [0xFFFF0000], desc: 'MI register access (0xA430xxxx)' },
};

// Analyze each boot function
const results = [];

for (const func of bootFuncs) {
    const romOff = func.vram - 0x80000400 + 0x1000;
    if (romOff + func.size > rom.length) continue;

    const analysis = {
        name: func.name,
        vram: func.vram,
        size: func.size,
        hwAccess: [],
        callsCount: 0,
        isTiny: func.size <= 0x20,
        hasMFC0: false, hasMTC0: false,
        hasCACHE: false, hasTLB: false,
        hasDisableInt: false,
        firstInstr: readU32(romOff),
    };

    for (let off = 0; off < func.size; off += 4) {
        const instr = readU32(romOff + off);
        const op = (instr >>> 26) & 0x3F;

        // JAL count
        if (op === 3) analysis.callsCount++;

        // COP0 (MFC0/MTC0)
        if (op === 0x10) {
            const rs = (instr >>> 21) & 0x1F;
            if (rs === 0) analysis.hasMFC0 = true;
            if (rs === 4) analysis.hasMTC0 = true;
            if (rs === 0x10) { // COP0 CO instructions
                const funct = instr & 0x3F;
                if (funct === 0x02 || funct === 0x06) analysis.hasTLB = true; // TLBWI, TLBWR
                if (funct === 0x08) analysis.hasTLB = true; // TLBP
                if (funct === 0x01) analysis.hasTLB = true; // TLBR
                if (funct === 0x18) analysis.hasDisableInt = true; // ERET
            }
        }

        // CACHE instruction (op = 0x2F)
        if (op === 0x2F) analysis.hasCACHE = true;

        // Hardware register LUI patterns
        const rt = (instr >>> 16) & 0x1F;
        const imm = instr & 0xFFFF;
        if (op === 0x0F) { // LUI
            if (imm === 0xA440) analysis.hwAccess.push('VI');
            if (imm === 0xA460) analysis.hwAccess.push('PI');
            if (imm === 0xA480) analysis.hwAccess.push('SI');
            if (imm === 0xA450) analysis.hwAccess.push('AI');
            if (imm === 0xA404) analysis.hwAccess.push('SP');
            if (imm === 0xA430) analysis.hwAccess.push('MI');
            if (imm === 0xA470) analysis.hwAccess.push('RI'); // RDRAM Interface
        }
    }

    // Deduplicate hwAccess
    analysis.hwAccess = [...new Set(analysis.hwAccess)];

    // Classify
    if (analysis.hasCACHE || analysis.hasTLB || analysis.hasMFC0 || analysis.hasMTC0 ||
        analysis.hwAccess.length > 0 || analysis.hasDisableInt) {
        results.push(analysis);
    }
}

// Group by classification
const cacheFunc = results.filter(r => r.hasCACHE);
const tlbFuncs = results.filter(r => r.hasTLB);
const cop0Funcs = results.filter(r => (r.hasMFC0 || r.hasMTC0) && !r.hasCACHE && !r.hasTLB);
const hwFuncs = results.filter(r => r.hwAccess.length > 0 && !r.hasCACHE && !r.hasTLB && !r.hasMFC0 && !r.hasMTC0);

console.log(`\n=== Functions with hardware/OS characteristics ===`);
console.log(`CACHE instructions: ${cacheFunc.length}`);
console.log(`TLB instructions: ${tlbFuncs.length}`);
console.log(`COP0 (MFC0/MTC0): ${cop0Funcs.length}`);
console.log(`Hardware register access: ${hwFuncs.length}`);

console.log('\n=== All identified OS/hardware functions ===');
const allOsFuncs = new Set();

for (const r of results) {
    const tags = [];
    if (r.hasCACHE) tags.push('CACHE');
    if (r.hasTLB) tags.push('TLB');
    if (r.hasMFC0) tags.push('MFC0');
    if (r.hasMTC0) tags.push('MTC0');
    if (r.hwAccess.length) tags.push('HW:' + r.hwAccess.join(','));
    console.log(`  ${r.name} (${hex(r.vram)}, ${r.size} bytes, ${r.callsCount} calls) — ${tags.join(', ')}`);
    allOsFuncs.add(r.name);
}

// Also identify functions that are ONLY called by OS functions (internal helpers)
// These are likely internal OS functions too
console.log(`\n=== Functions calling OS functions (potential OS wrappers) ===`);
const osFuncVrams = new Set(results.map(r => r.vram));
const osWrappers = [];

for (const func of bootFuncs) {
    if (osFuncVrams.has(func.vram)) continue; // skip already identified
    const romOff = func.vram - 0x80000400 + 0x1000;

    let callsToOs = 0;
    let totalCalls = 0;
    for (let off = 0; off < func.size; off += 4) {
        const instr = readU32(romOff + off);
        if ((instr >>> 26) === 3) { // JAL
            const target = (((instr & 0x3FFFFFF) << 2) | 0x80000000) >>> 0;
            totalCalls++;
            if (osFuncVrams.has(target)) callsToOs++;
        }
    }

    if (callsToOs > 0 && totalCalls <= 5) {
        console.log(`  ${func.name} (${hex(func.vram)}, ${func.size} bytes) — ${callsToOs}/${totalCalls} calls to OS funcs`);
        osWrappers.push(func);
    }
}

// Generate stub list
console.log('\n=== Recommended stubs list ===');
console.log('(add these to sfrush.recomp.toml [patches] stubs = [...])');
console.log('');

const stubList = [...allOsFuncs].sort();
for (const name of stubList) {
    console.log(`    "${name}",`);
}

console.log(`\nTotal: ${stubList.length} functions to stub`);
