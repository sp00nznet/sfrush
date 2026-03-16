// sfrush_heap_deep.js — Read heap and allocation-related memory from mupen
// Focus on the data that the 8th allocation reads its size from
const { spawn } = require('child_process');
const path = require('path');
const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const totalSeconds = parseInt(process.argv[2]) || 10;

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
    for (let i = 0; i < reads.length && i < values.length; i++) {
        console.log(`  0x${reads[i]}: 0x${values[i]}`);
    }
});

// Read the heap struct at 0x800D9580 and surrounding area
// Also read the allocator's data area (what gets allocated from the heap)
// The heap base is 0x801252E0, size 0x3C000, so heap end = 0x801612E0
// The 8th allocation reads its size from game data — need to find WHERE
// Also dump the area around allocation #7's end (0x80140C30) which is
// where the next allocation struct would be read from
const reads = [];
// Heap descriptor
for (let a = 0x800D9580; a < 0x800D95C0; a += 4) reads.push(a.toString(16));
// Heap data area — first 256 bytes
for (let a = 0x801252E0; a < 0x801253E0; a += 4) reads.push(a.toString(16));
// Around allocation cursor (after 7 allocs, cursor at ~0x80140C30)
for (let a = 0x80140C00; a < 0x80140D00; a += 4) reads.push(a.toString(16));
// The game's struct that stores allocation sizes (likely in the 0x8016xxxx range)
for (let a = 0x80161700; a < 0x80161800; a += 4) reads.push(a.toString(16));
// Check what's at the addresses the game reads for the allocation struct
// func_800BC2B0 loads from s0 (0x80125050 area initially)
for (let a = 0x80125038; a < 0x80125070; a += 4) reads.push(a.toString(16));
// Also check 0x80125050+0x58 = 0x801250A8 which might be the alloc struct
for (let a = 0x801250A0; a < 0x801250E0; a += 4) reads.push(a.toString(16));

function send(cmd) { proc.stdin.write(cmd + '\n'); }
async function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function main() {
    await sleep(3000);
    send('run');
    await sleep(totalSeconds * 1000);
    send('pause');
    await sleep(500);
    for (const addr of reads) {
        send(`mem ${addr} 4`);
        await sleep(20);
    }
    await sleep(500);
    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
