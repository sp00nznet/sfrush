// sfrush_trace_table.js — Set write breakpoint on 0x80161700 to find what populates it
// Usage: node sfrush_trace_table.js [seconds_to_run]

const { spawn } = require('child_process');
const path = require('path');
const M64P = 'D:/diddy/mupen64plus/Release/mupen64plus-ui-console.exe';
const ROM = path.join(__dirname, '..', '..', 'baserom.z64');
const totalSeconds = parseInt(process.argv[2]) || 15;

const proc = spawn(M64P, ['--emumode', '1', '--noosd', '--debug', ROM], {
    cwd: 'D:/diddy/mupen64plus/Release',
    stdio: ['pipe', 'pipe', 'pipe']
});

let allOutput = '';
proc.stdout.on('data', d => { allOutput += d.toString(); });
proc.stderr.on('data', d => { allOutput += d.toString(); });

proc.on('exit', code => {
    console.log(`\nmupen exited (code ${code})`);
    const lines = allOutput.split('\n');
    for (const line of lines) {
        if (line.includes('Hit') || line.includes('break') || line.includes('BP') ||
            line.includes('PC at') || line.includes('0x80161')) {
            console.log('  ' + line.trim());
        }
    }
    // Also show all (dbg) responses
    console.log('\n--- All debugger output ---');
    for (const line of lines) {
        if (line.includes('(dbg)') || line.includes('Hit') || line.includes('PC at')) {
            console.log('  ' + line.trim());
        }
    }
});

function send(cmd) {
    console.log(`>>> ${cmd}`);
    proc.stdin.write(cmd + '\n');
}
async function sleep(ms) { return new Promise(r => setTimeout(r, ms)); }

async function main() {
    await sleep(3000);

    // Set a write breakpoint on 0x80161700
    // mupen64plus debug format: bp add <address> [flags]
    // flags: r=read, w=write, x=execute
    // Try: "bp add 80161700 w"
    send('bp add 80161700 4');  // 4 = write flag in mupen debug
    await sleep(200);

    // Also try alternate syntax
    send('wp add 80161700 4 w'); // watchpoint
    await sleep(200);

    console.log('Starting execution with breakpoint on 0x80161700 write...');
    send('run');

    // Wait and check for breakpoint hits
    for (let i = 0; i < totalSeconds; i++) {
        await sleep(1000);
        // Check if we hit a breakpoint
        if (allOutput.includes('Hit') || allOutput.includes('break') || allOutput.includes('PC at 0x')) {
            console.log('Breakpoint hit detected!');
            await sleep(500);
            send('pc');
            await sleep(200);
            // Read the value that was just written
            send('mem 80161700 4');
            send('mem 80161704 4');
            await sleep(200);

            // Get registers to see the writer
            // In mupen debug, we can check PC
            send('run');
            await sleep(2000);
            // Check if another hit
            if (allOutput.split('PC at').length > 2) {
                send('pause');
                await sleep(200);
                send('pc');
                await sleep(200);
            }
            break;
        }
    }

    // If no breakpoint hit, just check memory state
    send('pause');
    await sleep(500);
    send('mem 80161700 4');
    send('mem 80161704 4');
    send('mem 80161708 4');
    send('mem 8016170C 4');
    await sleep(500);

    send('quit');
    await sleep(3000);
    if (!proc.killed) proc.kill();
}

main().catch(err => { console.error(err); proc.kill(); process.exit(1); });
