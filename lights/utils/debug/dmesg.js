#!/usr/bin/env node

const fs = require('node:fs');
const child_process = require('node:child_process');

function fatal(msg) {
    console.log('Fatal error:', msg);
    process.exit(1);
}

function main() {
    const mapFileName = process.argv[2];
    if (!mapFileName) {
        console.log(`usage: node ${process.argv[1]} build/mytarget/source/myprog.map`);
        return;
    }
    console.log(`Map file: ${mapFileName}`);
    const mapFile = fs.readFileSync(mapFileName, 'utf8');
    let addr = 0;
    const logSize = 1024 * 4 + 4;
    for (const ln of mapFile.split(/\r?\n/)) {
        const m = /^\s*0x00000([0-9a-f]+)\s+(\S+)/.exec(ln);
        if (m && m[2] === 'codalLogStore') {
            addr = Number.parseInt(m[1], 16);
            break;
        }
    }
    if (!addr) {
        fatal('Cannot find codalLogStore symbol in map file');
    }

    const dirs = [
        `${process.env.HOME}/Library/Arduino15`,
        `${process.env.USERPROFILE}/AppData/Local/Arduino15`,
        `${process.env.HOME}/.arduino15`,
    ];

    let pkgDir = '';

    for (const d of dirs) {
        pkgDir = `${d}/packages/arduino/`;
        if (fs.existsSync(pkgDir)) {
            break;
        }
        pkgDir = '';
    }

    if (!pkgDir) {
        fatal('cannot find Arduino packages directory');
    }

    const openocdPath = `${pkgDir}tools/openocd/0.9.0-arduino/`;
    if (!fs.existsSync(openocdPath)) {
        fatal('openocd not installed in Arduino');
    }

    let openocdBin = `${openocdPath}bin/openocd`;

    if (process.platform === 'win32') {
        openocdBin += '.exe';
    }

    const zeroCfg = `${pkgDir}hardware/samd/1.6.8/variants/arduino_zero/openocd_scripts/arduino_zero.cfg`;
    const cmd = `init; set M(0) 0; mem2array M 8 ${addr} ${logSize}; parray M; exit`;

    console.log('Starting openocd');
    child_process.execFile(
        openocdBin,
        ['-d2', '-s', `${openocdPath}/share/openocd/scripts/`, '-f', zeroCfg, '-c', cmd],
        {
            maxBuffer: 1 * 1024 * 1024,
        },
        (err, stdout, stderr) => {
            if (err) {
                fatal(`error: ${err.message}`);
            }
            const buf = new Buffer(logSize);
            for (const l of stdout.split(/\r?\n/)) {
                const m = /^M\((\d+)\)\s*=\s*(\d+)/.exec(l);
                if (m) {
                    buf[Number.parseInt(m[1])] = Number.parseInt(m[2]);
                }
            }
            const len = buf.readUInt32LE(0);
            if (len === 0 || len > buf.length) {
                console.log(stderr);
                console.log('No logs.');
            } else {
                console.log('*\n* Logs\n*\n');
                console.log(buf.slice(4, 4 + len).toString('binary'));
            }
        }
    );
}

main();
