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
proc.on('exit', () => {
    const vals = [];
    for (const line of out.split('\n')) {
        const m = line.match(/^\s*(?:\(dbg\)\s*)?([0-9a-f]{8})\s*$/);
        if (m) vals.push(m[1]);
    }
    const addrs = ['80162AA0','80162AA4','80162AA8','80162AAC',
                   '80162AB0','80162AB4','80162AB8','80162ABC',
                   '80162AC0','80162AC4','80162AC8','80162ACC',
                   '80140BB0','80140BB4','80140BB8','80140BBC'];
    for (let i = 0; i < addrs.length && i < vals.length; i++) {
        console.log(`0x${addrs[i]}: 0x${vals[i]}`);
    }
});
function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000);
    send('run');
    await sleep(5000);
    send('pause');
    await sleep(300);
    const addrs = ['80162AA0','80162AA4','80162AA8','80162AAC',
                   '80162AB0','80162AB4','80162AB8','80162ABC',
                   '80162AC0','80162AC4','80162AC8','80162ACC',
                   '80140BB0','80140BB4','80140BB8','80140BBC'];
    for (const a of addrs) { send(`mem ${a} 4`); await sleep(30); }
    await sleep(500);
    send('quit');
    await sleep(2000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
