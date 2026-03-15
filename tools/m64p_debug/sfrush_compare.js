// sfrush_compare.js — Read key memory areas from SF Rush in mupen for comparison
// Usage: node sfrush_compare.js [seconds_to_run]

const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');

const totalSeconds = parseInt(process.argv[2]) || 8;

const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: 'D:/diddy/mupen64plus/Release',
    stdio: ['pipe', 'pipe', 'pipe']
});

let allOutput = '';
proc.stdout.on('data', d => { allOutput += d.toString(); });
proc.stderr.on('data', d => { allOutput += d.toString(); });
proc.on('exit', code => {
    console.log(`\nmupen exited (code ${code})`);
    // Parse mem responses
    const lines = allOutput.split('\n');
    const values = [];
    for (const line of lines) {
        const m = line.match(/^\s*(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
        if (m) values.push(m[1]);
    }
    console.log(`\nCollected ${values.length} memory values:`);

    const reads = [
        // Boot area
        '80000300', '80000304', '80000308', '8000030C',
        '80000310', '80000314', '80000318', '8000031C',
        // Game init flag area
        '80021664', '80021668', '8002166C', '80021670',
        '80021590', '80021594', '80021598', '8002159C',
        // Message queue 1 (0x80020EB8)
        '80020EB8', '80020EBC', '80020EC0', '80020EC4',
        '80020EC8', '80020ECC', '80020ED0', '80020ED4',
        // First few words after game init in BSS
        '800D8060', '800D8064', '800D8068', '800D806C',
        '800D9580', '800D9584', '800D9588', '800D958C',
        // Check what func_800BC2B0 reads from (0x800D9580 area)
        '800D95A0', '800D95A4', '800D95A8', '800D95AC',
    ];

    for (let i = 0; i < reads.length && i < values.length; i++) {
        console.log(`  0x${reads[i]}: 0x${values[i]}`);
    }
});

function send(cmd) { proc.stdin.write(cmd + '\n'); }
async function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function main() {
    await sleep(3000);
    send('run');
    await sleep(totalSeconds * 1000);
    send('pause');
    await sleep(500);

    const reads = [
        '80000300', '80000304', '80000308', '8000030C',
        '80000310', '80000314', '80000318', '8000031C',
        '80021664', '80021668', '8002166C', '80021670',
        '80021590', '80021594', '80021598', '8002159C',
        '80020EB8', '80020EBC', '80020EC0', '80020EC4',
        '80020EC8', '80020ECC', '80020ED0', '80020ED4',
        '800D8060', '800D8064', '800D8068', '800D806C',
        '800D9580', '800D9584', '800D9588', '800D958C',
        '800D95A0', '800D95A4', '800D95A8', '800D95AC',
    ];

    for (const addr of reads) {
        send(`mem ${addr} 4`);
        await sleep(30);
    }
    await sleep(500);
    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}

main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
