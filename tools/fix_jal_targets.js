// fix_jal_targets.js — Iteratively fix "No function found for jal target" errors
// by splitting boot segment functions to expose sub-function entry points
//
// Usage: node fix_jal_targets.js

const { execSync } = require('child_process');
const fs = require('fs');
const path = require('path');

const N64RECOMP = path.join(__dirname, '..', 'build_n64recomp', 'Release', 'N64Recomp.exe');
const TOML = path.join(__dirname, '..', 'sfrush.recomp.toml');
const SYMS = path.join(__dirname, '..', 'symbols', 'sfrush.us.syms.toml');

const MAX_ITERATIONS = 50;

for (let iter = 0; iter < MAX_ITERATIONS; iter++) {
    console.log(`\n=== Iteration ${iter + 1} ===`);

    let output;
    try {
        output = execSync(`"${N64RECOMP}" "${TOML}" 2>&1`, { encoding: 'utf8', timeout: 60000 });
    } catch (e) {
        output = e.stdout || '';
        if (e.status === 139 || e.signal === 'SIGSEGV') {
            console.log('N64Recomp segfaulted — this might be a different issue');
            break;
        }
    }

    // Find "No function found for jal target: 0xXXXXXXXX"
    const match = output.match(/No function found for jal target: (0x[0-9a-fA-F]+)/);
    if (!match) {
        // Check for function count
        const countMatch = output.match(/Function count: (\d+)/);
        if (countMatch) {
            console.log(`Success! Function count: ${countMatch[1]}`);
        } else {
            console.log('No JAL target errors found.');
        }
        console.log('Output:', output.trim());
        break;
    }

    const targetAddr = parseInt(match[1]);
    console.log(`Missing JAL target: 0x${targetAddr.toString(16)}`);

    // Read symbols file
    let syms = fs.readFileSync(SYMS, 'utf8');

    // Find the function that contains this address
    // Parse all [[section.functions]] entries
    const funcRegex = /\[\[section\.functions\]\]\s*\nname = "func_([0-9A-Fa-f]+)"\s*\nvram = (0x[0-9A-Fa-f]+)\s*\nsize = (0x[0-9A-Fa-f]+)/g;
    let found = false;

    const entries = [];
    let m;
    while ((m = funcRegex.exec(syms)) !== null) {
        entries.push({
            name: m[1],
            vram: parseInt(m[2]),
            size: parseInt(m[3]),
            fullMatch: m[0],
            index: m.index,
        });
    }

    // Only check boot section functions (0x80000400-0x8001BDC8)
    for (const entry of entries) {
        if (entry.vram > 0x8001BDC8) continue; // skip game section
        const endAddr = entry.vram + entry.size;
        if (targetAddr > entry.vram && targetAddr < endAddr) {
            // Split this function
            const newSize1 = targetAddr - entry.vram;
            const newSize2 = endAddr - targetAddr;
            const targetHex = targetAddr.toString(16).toUpperCase();

            console.log(`  Splitting func_${entry.name} (0x${entry.vram.toString(16)}, size 0x${entry.size.toString(16)})`);
            console.log(`  → func_${entry.name} size 0x${newSize1.toString(16)} + func_${targetHex} size 0x${newSize2.toString(16)}`);

            const oldText = entry.fullMatch;
            const newText = `[[section.functions]]\nname = "func_${entry.name}"\nvram = 0x${entry.vram.toString(16).toUpperCase()}\nsize = 0x${newSize1.toString(16).toUpperCase()}\n\n[[section.functions]]\nname = "func_${targetHex}"\nvram = 0x${targetAddr.toString(16).toUpperCase()}\nsize = 0x${newSize2.toString(16).toUpperCase()}`;

            syms = syms.replace(oldText, newText);
            fs.writeFileSync(SYMS, syms);
            found = true;
            break;
        }
    }

    if (!found) {
        console.log(`  Could not find containing function for 0x${targetAddr.toString(16)} in boot section`);
        console.log(`  This might be in a section gap or need manual handling`);

        // Check if it's in the game section range
        if (targetAddr >= 0x8005BB10 && targetAddr < 0x800D8060) {
            console.log(`  Target is in game section — may need to add it to game symbols`);
        }
        break;
    }
}
