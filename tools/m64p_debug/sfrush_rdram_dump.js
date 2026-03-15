// sfrush_rdram_dump.js — Boot SF Rush in mupen64plus debug mode, dump RDRAM
// Usage: node sfrush_rdram_dump.js [seconds_to_run]

const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const DUMP_DIR = path.join(__dirname, 'dumps');

if (!fs.existsSync(DUMP_DIR)) fs.mkdirSync(DUMP_DIR);
if (!fs.existsSync(ROM)) {
    console.error('ROM not found:', ROM);
    process.exit(1);
}

const totalSeconds = parseInt(process.argv[2]) || 10;
console.log(`[sfrush] Will run for ${totalSeconds}s, then dump RDRAM`);
console.log(`[sfrush] ROM: ${ROM}`);
console.log(`[sfrush] Mupen: ${M64P}`);

const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: 'D:/diddy/mupen64plus/Release',
    stdio: ['pipe', 'pipe', 'pipe']
});

let allOutput = '';
proc.stdout.on('data', d => { allOutput += d.toString(); });
proc.stderr.on('data', d => { allOutput += d.toString(); });
proc.on('exit', code => {
    console.log(`[sfrush] mupen exited (code ${code})`);
    // Parse any mem read outputs
    const lines = allOutput.split('\n');
    for (const line of lines) {
        if (line.includes('(dbg)') && !line.includes('>>>')) {
            console.log('  ' + line.trim());
        }
    }
});

function send(cmd) {
    console.log(`[sfrush] >>> ${cmd}`);
    proc.stdin.write(cmd + '\n');
}

async function sleep(ms) {
    return new Promise(r => setTimeout(r, ms));
}

async function main() {
    await sleep(3000);
    console.log('[sfrush] Starting emulation...');
    send('run');

    // Wait for game to boot and initialize
    await sleep(totalSeconds * 1000);

    console.log('[sfrush] Pausing and dumping...');
    send('pause');
    await sleep(500);

    // Dump full 8MB RDRAM
    const dumpFile = path.join(DUMP_DIR, 'sfrush_rdram.bin').replace(/\\/g, '/');
    send(`dumpmem 80000000 800000 ${dumpFile}`);
    await sleep(1000);

    // Read key addresses
    // Boot state
    send('mem 80000300 4');  // osTvType
    send('mem 80000308 4');  // osRomBase
    send('mem 80000318 4');  // osMemSize
    await sleep(200);

    // Game code area (check if LZSS decompressed)
    send('mem 8005BB10 4');  // First word of game code
    send('mem 8005BB14 4');
    send('mem 800BBE88 4');  // Thread 7 entry
    send('mem 800BD440 4');  // Game init entry
    await sleep(200);

    // BSS area
    send('mem 800D8060 4');  // BSS start
    send('mem 80021664 4');  // Flag set by func_800005D8
    send('mem 80021590 4');  // FPU constant
    await sleep(200);

    // Message queue area
    send('mem 80020EB8 4');  // Queue 1
    send('mem 80020EF0 4');  // Queue 2
    await sleep(500);

    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}

main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
