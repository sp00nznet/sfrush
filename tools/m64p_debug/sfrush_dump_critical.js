// Dump critical BSS range 0x80162A00-0x80163000 (3KB) from mupen
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
    console.log(`Collected ${vals.length} values`);
    // Generate C code to pre-populate
    const start = 0x80162A00;
    let code = '    // Pre-populated from mupen RDRAM dump\n';
    code += '    // Range: 0x80162A00-0x80163000\n';
    code += '    {\n';
    let nonzero = 0;
    for (let i = 0; i < vals.length; i++) {
        if (vals[i] !== '00000000') {
            const addr = start + i * 4;
            const off = addr - 0x80000000;
            code += `        *(uint32_t*)(rdram + 0x${off.toString(16)}) = 0x${vals[i]};\n`;
            nonzero++;
        }
    }
    code += `        fprintf(stderr, "[SFRush]   Pre-populated ${nonzero} values in 0x80162A00-0x80163000\\n");\n`;
    code += '    }\n';
    fs.writeFileSync(path.join(__dirname, 'bss_init_code.c'), code);
    console.log(`Generated bss_init_code.c with ${nonzero} non-zero values`);
});
function send(cmd) { proc.stdin.write(cmd + '\n'); }
function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }
async function main() {
    await sleep(3000); send('run'); await sleep(5000); send('pause'); await sleep(500);
    const start = 0x80162A00;
    const count = 384; // 0x600 bytes = 384 words
    for (let i = 0; i < count; i++) {
        send(`mem ${(start + i*4).toString(16)} 4`);
        if (i % 50 === 49) await sleep(100);
    }
    await sleep(3000); send('quit'); await sleep(3000);
    if (!proc.killed) proc.kill();
}
main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
