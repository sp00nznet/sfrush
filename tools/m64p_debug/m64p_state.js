// m64p_state.js — Boot an N64 ROM via mupen64plus debugger, read state at timed intervals
// Usage: node m64p_state.js <config.json> [total_seconds] [interval_seconds]
// Outputs clean state dumps to stdout
// Adapted from DKR project for general use

const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

// --- Load config ---
const configPath = process.argv[2];
if (!configPath) {
    console.error('Usage: node m64p_state.js <config.json> [total_seconds] [interval_seconds]');
    console.error('  config.json — JSON file with rom, emumode, state_reads, and optional labels');
    process.exit(1);
}

const config = JSON.parse(fs.readFileSync(configPath, 'utf8'));
const ROM = config.rom;
const EMUMODE = String(config.emumode != null ? config.emumode : 1);
const STATE_READS = config.state_reads.map(r => [r.name, r.addr, r.size]);
const LABELS = config.labels || {};

const totalSec = parseInt(process.argv[3]) || 20;
const intervalSec = parseInt(process.argv[4]) || 10;

const M64P = path.join(__dirname, 'Release', 'mupen64plus-ui-console.exe');

const proc = spawn(M64P, ['--emumode', EMUMODE, '--noosd', '--debug', ROM], {
    cwd: path.join(__dirname, 'Release'),
    stdio: ['pipe', 'pipe', 'pipe']
});

let allStdout = '';
proc.stdout.on('data', d => { allStdout += d.toString(); });
proc.stderr.on('data', d => {}); // suppress stderr noise

proc.on('exit', code => {
    const lines = allStdout.split('\n');
    const snapshots = [];
    let currentSnap = null;

    for (const line of lines) {
        const pcMatch = line.match(/PC at 0x([0-9a-fA-F]+)/);
        if (pcMatch) {
            if (currentSnap && currentSnap.values.length > 0) {
                snapshots.push(currentSnap);
            }
            currentSnap = { pc: pcMatch[1], values: [] };
            continue;
        }

        if (currentSnap) {
            const memMatch = line.match(/^(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
            if (memMatch) {
                currentSnap.values.push(memMatch[1]);
            }
        }
    }
    if (currentSnap && currentSnap.values.length > 0) {
        snapshots.push(currentSnap);
    }

    const numReads = STATE_READS.length;
    let snapIdx = 0;
    for (const snap of snapshots) {
        if (snap.values.length < numReads) continue;
        snapIdx++;
        const elapsed = snapIdx * intervalSec;
        console.log(`\n=== Snapshot #${snapIdx} (${elapsed}s) PC=0x${snap.pc} ===`);
        for (let r = 0; r < numReads && r < snap.values.length; r++) {
            const [name, addr] = STATE_READS[r];
            const val = snap.values[r];
            let extra = '';
            if (LABELS[name] && LABELS[name][val]) {
                extra = ` (${LABELS[name][val]})`;
            }
            console.log(`  ${name.padEnd(24)} [${addr}] = ${val}${extra}`);
        }
    }

    if (snapIdx === 0) {
        console.log('\nNo complete snapshots captured. Raw (dbg) output:');
        for (const line of lines) {
            if (line.includes('(dbg)') || line.includes('PC at')) console.log(line);
        }
    }
});

function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function main() {
    await sleep(3000);
    send('run');

    const startTime = Date.now();
    let snapNum = 0;

    while ((Date.now() - startTime) < totalSec * 1000) {
        await sleep(intervalSec * 1000);
        snapNum++;
        send('pause');
        await sleep(500);
        send('pc');
        await sleep(100);
        for (const [name, addr, len] of STATE_READS) {
            send(`mem ${addr} ${len}`);
            await sleep(50);
        }
        await sleep(300);
        send('run');
    }

    send('pause');
    await sleep(500);
    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}

main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
