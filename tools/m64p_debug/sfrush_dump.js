// sfrush_dump.js — Boot SF Rush in mupen64plus, dump RDRAM at timed intervals
// Primary use: capture DMA-loaded code segments after the game boots
// Usage: node sfrush_dump.js [seconds_to_run] [dump_interval_seconds]

const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const M64P = path.join(__dirname, 'Release', 'mupen64plus-ui-console.exe');
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const DUMP_DIR = path.join(__dirname, 'dumps');

if (!fs.existsSync(DUMP_DIR)) fs.mkdirSync(DUMP_DIR);

const totalSeconds = parseInt(process.argv[2]) || 30;
const intervalSeconds = parseInt(process.argv[3]) || 10;

console.log(`[sfrush] Will run SF Rush for ${totalSeconds}s, dumping every ${intervalSeconds}s`);
console.log(`[sfrush] ROM: ${ROM}`);
console.log(`[sfrush] Dumps go to: ${DUMP_DIR}`);

if (!fs.existsSync(ROM)) {
    console.error(`[sfrush] ERROR: ROM not found at ${ROM}`);
    console.error(`[sfrush] Expected baserom.z64 in the sfrush project root`);
    process.exit(1);
}

const emumode = '1'; // Must use cached interpreter for debug
console.log(`[sfrush] Using emumode ${emumode}`);

const proc = spawn(M64P, ['--emumode', emumode, '--noosd', '--debug', ROM], {
    cwd: path.join(__dirname, 'Release'),
    stdio: ['pipe', 'pipe', 'pipe']
});

let allOutput = '';
proc.stdout.on('data', d => { allOutput += d.toString(); });
proc.stderr.on('data', d => { allOutput += d.toString(); });
proc.on('exit', code => {
    console.log(`[sfrush] mupen exited (code ${code})`);
});

function send(cmd) {
    console.log(`[sfrush] >>> ${cmd}`);
    proc.stdin.write(cmd + '\n');
}

// Key SF Rush addresses to monitor
const STATE_VARS = {
    // Boot thread entry (known)
    'bootproc_entry':     '80000450',
    // Thread created at 0x800005D8, entry 0x800CBE88 (DMA-loaded)
    'dma_thread_entry':   '800CBE88',
    // DMA table area (ROM 0x00FFB000 = likely loaded somewhere in RDRAM)
    // Check if PI DMA has loaded code to 0x800C0000+
    'dma_code_start':     '800C0000',
    // osMemSize (total RDRAM)
    'osMemSize':          '80000318',
    // First word at potential DMA code region
    'dma_code_word0':     '800C0000',
    'dma_code_word1':     '800C0004',
    'dma_code_word2':     '800C0008',
    'dma_code_word3':     '800C000C',
    // Check 0x800CBE88 area (thread entry point)
    'thread_entry_w0':    '800CBE88',
    'thread_entry_w1':    '800CBE8C',
    'thread_entry_w2':    '800CBE90',
};

async function sleep(ms) {
    return new Promise(r => setTimeout(r, ms));
}

async function main() {
    await sleep(3000);
    send('run');

    let dumpNum = 0;
    const startTime = Date.now();

    while ((Date.now() - startTime) < totalSeconds * 1000) {
        await sleep(intervalSeconds * 1000);
        dumpNum++;

        const elapsed = ((Date.now() - startTime) / 1000).toFixed(1);
        console.log(`\n[sfrush] === Snapshot #${dumpNum} at ${elapsed}s ===`);

        send('pause');
        await sleep(500);

        // Dump full RDRAM (8MB) to file
        const rdramFile = path.join(DUMP_DIR, `rdram_${dumpNum}.bin`).replace(/\\/g, '/');
        send(`dumpmem 80000000 800000 ${rdramFile}`);
        await sleep(500);

        // Read key state variables
        for (const [name, addr] of Object.entries(STATE_VARS)) {
            send(`mem ${addr} 4`);
        }
        await sleep(300);

        send('run');
    }

    // Final dump
    dumpNum++;
    console.log(`\n[sfrush] === Final snapshot #${dumpNum} ===`);
    send('pause');
    await sleep(500);
    const finalFile = path.join(DUMP_DIR, `rdram_final.bin`).replace(/\\/g, '/');
    send(`dumpmem 80000000 800000 ${finalFile}`);
    await sleep(500);

    for (const [name, addr] of Object.entries(STATE_VARS)) {
        send(`mem ${addr} 4`);
    }
    await sleep(500);

    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}

main().catch(err => {
    console.error('[sfrush] Error:', err);
    proc.kill();
    process.exit(1);
});
