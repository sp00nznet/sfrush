const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');
const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const CWD = 'D:/diddy/mupen64plus/Release';
const DUMP_FILE = path.join(CWD, 'rdram_dump.bin');

const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: CWD, stdio: ['pipe', 'pipe', 'pipe']
});
let out = '';
proc.stdout.on('data', d => { out += d.toString(); });
proc.stderr.on('data', d => { out += d.toString(); });
proc.on('exit', () => {
    console.log('mupen exited');
    // Check if dump file was created
    if (fs.existsSync(DUMP_FILE)) {
        const data = fs.readFileSync(DUMP_FILE);
        console.log(`RDRAM dump: ${data.length} bytes`);
        // Copy to sfrush tools dir
        const dest = path.join(__dirname, 'mupen_rdram_full.bin');
        fs.copyFileSync(DUMP_FILE, dest);
        console.log(`Copied to ${dest}`);
    } else {
        console.log('Dump file not found at ' + DUMP_FILE);
        // Check what files are in CWD
        const files = fs.readdirSync(CWD).filter(f => f.includes('rdram') || f.includes('dump'));
        console.log('Files matching dump/rdram:', files);
    }
});
function send(cmd) { console.log('>>> ' + cmd); proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000); send('run'); await sleep(5000);
    send('pause'); await sleep(500);
    // Use dumpmem with relative path (CWD is mupen dir)
    send('dumpmem 80000000 800000 rdram_dump.bin');
    await sleep(2000);
    send('quit'); await sleep(3000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
