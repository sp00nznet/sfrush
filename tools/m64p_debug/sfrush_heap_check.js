// sfrush_heap_check.js — Read heap-related memory from mupen for comparison
const { spawn } = require('child_process');
const path = require('path');
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
    const values = [];
    for (const line of allOutput.split('\n')) {
        const m = line.match(/^\s*(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
        if (m) values.push(m[1]);
    }
    const reads = [
        // Heap area (func_80007C50 reads from a struct passed in a2)
        // The struct is likely at the address passed in the call context
        // Let me read the heap management area
        '800D9580', '800D9584', '800D9588', '800D958C',
        '800D9590', '800D9594', '800D9598', '800D959C',
        '800D95A0', '800D95A4', '800D95A8', '800D95AC',
        '800D95B0', '800D95B4', '800D95B8', '800D95BC',
        // Also check what's at 0x801252E0 (source for the big copy)
        '801252E0', '801252E4', '801252E8', '801252EC',
        '801252F0', '801252F4', '801252F8', '801252FC',
        // And 0x80125050 (used in func_800BC2B0 as s0 = heap pointer)
        '80125050', '80125054', '80125058', '8012505C',
        '80125060', '80125064', '80125068', '8012506C',
    ];
    console.log(`Collected ${values.length} values:`);
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
        '800D9580', '800D9584', '800D9588', '800D958C',
        '800D9590', '800D9594', '800D9598', '800D959C',
        '800D95A0', '800D95A4', '800D95A8', '800D95AC',
        '800D95B0', '800D95B4', '800D95B8', '800D95BC',
        '801252E0', '801252E4', '801252E8', '801252EC',
        '801252F0', '801252F4', '801252F8', '801252FC',
        '80125050', '80125054', '80125058', '8012505C',
        '80125060', '80125064', '80125068', '8012506C',
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
