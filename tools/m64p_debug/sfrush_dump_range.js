// Dump a range of RDRAM from mupen and save to file
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');
const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: 'D:/diddy/mupen64plus/Release', stdio: ['pipe', 'pipe', 'pipe']
});
let out = '';
proc.stdout.on('data', d => { out += d.toString(); });
proc.stderr.on('data', d => { out += d.toString(); });
proc.on('exit', () => {
    const vals = [];
    for (const line of out.split('\n')) {
        const m = line.match(/^\s*(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
        if (m) vals.push(m[1]);
    }
    // Output as a simple hex dump
    const startAddr = 0x80162000;
    let result = '';
    for (let i = 0; i < vals.length; i++) {
        result += `0x${(startAddr + i*4).toString(16).padStart(8,'0')}: 0x${vals[i]}\n`;
    }
    fs.writeFileSync(path.join(__dirname, 'mupen_162000.txt'), result);
    console.log(`Wrote ${vals.length} values to mupen_162000.txt`);
    // Show non-zero values
    for (let i = 0; i < vals.length; i++) {
        if (vals[i] !== '00000000') {
            console.log(`  0x${(startAddr + i*4).toString(16)}: 0x${vals[i]}`);
        }
    }
});
function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000); send('run'); await sleep(5000); send('pause'); await sleep(300);
    // Dump 0x80162000-0x80163000 (4KB, 1024 words)
    for (let a = 0x80162000; a < 0x80163000; a += 4) {
        send(`mem ${a.toString(16)} 4`);
        await sleep(5);
    }
    await sleep(2000); send('quit'); await sleep(2000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
