const { spawn } = require('child_process');
const path = require('path');
const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: 'D:/diddy/mupen64plus/Release', stdio: ['pipe', 'pipe', 'pipe']
});
let out = '';
proc.stdout.on('data', d => { out += d.toString(); });
proc.stderr.on('data', d => { out += d.toString(); });
proc.on('exit', code => {
    const vals = [];
    for (const line of out.split('\n')) {
        const m = line.match(/^\s*(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
        if (m) vals.push(m[1]);
    }
    for (let i = 0; i < vals.length; i += 2) {
        console.log(`snapshot ${i/2+1}: table=0x${vals[i] || '??'} cursor=0x${vals[i+1] || '??'}`);
    }
});
function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000);
    send('run');
    for (let i = 0; i < 8; i++) {
        await sleep(500);
        send('pause');
        await sleep(300);
        send('mem 80161700 4');
        send('mem 800D9584 4');
        await sleep(200);
        send('run');
    }
    await sleep(500);
    send('quit');
    await sleep(2000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
