// Read FPU-related memory values from mupen that affect game init
// Focus on the values that func_800BC2B0 depends on
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
    const addrs = [
        // FPU constant at 0x80021590 (90.0f = 0x42B40000)
        '80021590', '80021594',
        // Values computed by game init that depend on FPU
        '80162AD0', '80162AD4', '80162AD8', '80162ADC',
        '80162AE0', '80162AE4', '80162AE8', '80162AEC',
        // func_80007F40 return area
        '800D9948', '800D994C', '800D9950', '800D9954',
        // Audio frequency related (func_80007F40 called with 0x5622)
        '800D9958', '800D995C', '800D9960', '800D9964',
    ];
    for (let i = 0; i < addrs.length && i < vals.length; i++) {
        console.log(`0x${addrs[i]}: 0x${vals[i]}`);
    }
});
function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000); send('run'); await sleep(5000); send('pause'); await sleep(300);
    const addrs = [
        '80021590', '80021594',
        '80162AD0', '80162AD4', '80162AD8', '80162ADC',
        '80162AE0', '80162AE4', '80162AE8', '80162AEC',
        '800D9948', '800D994C', '800D9950', '800D9954',
        '800D9958', '800D995C', '800D9960', '800D9964',
    ];
    for (const a of addrs) { send(`mem ${a} 4`); await sleep(20); }
    await sleep(500); send('quit'); await sleep(2000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
