'use strict';

const { execSync, spawnSync } = require('child_process');
const fs   = require('fs');
const path = require('path');
const readline = require('readline');

const ROOT = path.resolve(__dirname, '..');

const colors = {
    reset:  '\x1b[0m',
    red:    '\x1b[31m',
    green:  '\x1b[32m',
    yellow: '\x1b[33m',
    cyan:   '\x1b[36m',
    gray:   '\x1b[90m',
};

const log   = (msg) => console.log(msg);
const info  = (msg) => console.log(`${colors.cyan}${msg}${colors.reset}`);
const ok    = (msg) => console.log(`${colors.green}${msg}${colors.reset}`);
const warn  = (msg) => console.log(`${colors.yellow}${msg}${colors.reset}`);
const error = (msg) => console.error(`${colors.red}${msg}${colors.reset}`);

// 允許的後綴類型
const VALID_SUFFIX_TYPES = ['dev', 'hotfix', 'quickfix', 'tmp', 'preview', 'patch'];

function readBaseVersion() {
    const cmakeContent = fs.readFileSync(path.join(ROOT, 'CMakeLists.txt'), 'utf8');
    const match = cmakeContent.match(/project\([^)]+VERSION\s+([0-9.]+)/);
    if (!match) { throw new Error('無法從 CMakeLists.txt 解析基礎版本號。'); }
    return match[1];
}

function askQuestion(rl, question) {
    return new Promise((resolve) => rl.question(question, (ans) => resolve(ans.trim())));
}

async function main() {
    info('==== SGSL 補丁版本快速打包工具 ====\n');

    let baseVersion;
    try {
        baseVersion = readBaseVersion();
        log(`偵測到基礎版本號：${colors.cyan}${baseVersion}${colors.reset}`);
    } catch (err) {
        error(`讀取版本號失敗：${err.message}`);
        process.exit(1);
    }

    // 簡易參數解析
    const args = process.argv.slice(2);
    const getArg = (flag) => {
        const idx = args.indexOf(flag);
        return idx !== -1 && idx + 1 < args.length ? args[idx + 1] : null;
    };

    let suffixType = getArg('--type');
    let seq = getArg('--seq') || '';
    let description = getArg('--desc') || '';
    const autoConfirm = args.includes('--yes');

    if (!suffixType) {
        const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
        // 選擇後綴類型
        log('\n請選擇版本後綴類型：');
        VALID_SUFFIX_TYPES.forEach((t, i) => log(`  ${i + 1}. ${t}`));
        const typeChoice = await askQuestion(rl, '\n請輸入編號 (1-6)：');
        const typeIndex = parseInt(typeChoice, 10) - 1;
        if (typeIndex < 0 || typeIndex >= VALID_SUFFIX_TYPES.length) {
            error('無效的選擇，操作取消。');
            rl.close();
            process.exit(1);
        }
        suffixType = VALID_SUFFIX_TYPES[typeIndex];

        // 輸入流水號
        const seqStr = await askQuestion(rl, `請輸入流水號（例如 1、2、3，按 Enter 跳過）：`);
        seq = seqStr ? seqStr : '';

        // 輸入描述
        description = await askQuestion(rl, `請輸入這次更新的描述（可留空）：`);
        rl.close();
    }

    if (!VALID_SUFFIX_TYPES.includes(suffixType)) {
        error(`無效的後綴類型：${suffixType}`);
        process.exit(1);
    }

    const suffix = `${suffixType}${seq}`;
    const fullVersion = `${baseVersion}-${suffix}`;

    log(`\n${colors.gray}------------------------------------${colors.reset}`);
    log(`基礎版本  ：${baseVersion}`);
    log(`後綴      ：${suffix}`);
    log(`完整版本  ：${colors.cyan}${fullVersion}${colors.reset}`);
    log(`描述      ：${description || '（無）'}`);
    log(`${colors.gray}------------------------------------${colors.reset}\n`);

    // 確認
    if (!autoConfirm) {
        const confirmRl = readline.createInterface({ input: process.stdin, output: process.stdout });
        const confirm = await new Promise((resolve) => {
            confirmRl.question('確認開始編譯並打包？ (Y/N)：', (ans) => {
                confirmRl.close();
                resolve(ans.trim().toUpperCase());
            });
        });

        if (confirm !== 'Y') {
            warn('已取消。');
            process.exit(0);
        }
    }

    // --- 1. CMake Configure ---
    info('\n[1/3] 正在執行 CMake Configure...');
    try {
        execSync(
            `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAPP_VERSION_SUFFIX="${suffix}"`,
            { stdio: 'inherit', cwd: ROOT }
        );
        ok('Configure 完成。');
    } catch (err) {
        error('CMake Configure 失敗，請確認建置環境。');
        process.exit(1);
    }

    // --- 2. CMake Build ---
    info('\n[2/3] 正在編譯...');
    try {
        execSync('cmake --build build --config Release --parallel', { stdio: 'inherit', cwd: ROOT });
        ok('編譯完成。');
    } catch (err) {
        error('編譯失敗，請修復程式碼錯誤後重試。');
        process.exit(1);
    }

    // --- 3. CPack 打包 ZIP ---
    info('\n[3/3] 正在打包更新檔...');
    try {
        execSync('cpack -C Release -G "ZIP"', { stdio: 'inherit', cwd: path.join(ROOT, 'build') });
    } catch (err) {
        error('CPack 打包失敗。');
        process.exit(1);
    }

    // 找到產生的 ZIP 並改名為 .sgsl_update
    const buildDir = path.join(ROOT, 'build');
    const zipFiles = fs.readdirSync(buildDir).filter((f) => f.endsWith('.zip'));
    if (zipFiles.length === 0) {
        error('找不到打包產出的 ZIP 檔案。');
        process.exit(1);
    }

    const zipSrc  = path.join(buildDir, zipFiles[0]);
    const outName = `SteamGameServerLauncher-${fullVersion}`;
    const outDir  = path.join(ROOT, 'DevTool', 'output');
    if (!fs.existsSync(outDir)) { fs.mkdirSync(outDir, { recursive: true }); }

    const sgslUpdatePath = path.join(outDir, `${outName}.sgsl_update`);
    fs.copyFileSync(zipSrc, sgslUpdatePath);

    // 產生 .sgsl_update.info.json（Bootstrap 掃描時讀取）
    const infoJson = {
        version:     fullVersion,
        type:        suffixType,
        description: description || '',
        builtAt:     new Date().toISOString(),
    };
    fs.writeFileSync(
        path.join(outDir, `${outName}.sgsl_update.info.json`),
        JSON.stringify(infoJson, null, 2),
        'utf8'
    );

    log('');
    ok(`打包完成！`);
    log(`  更新包 ：${sgslUpdatePath}`);
    log(`  資訊檔 ：${outName}.sgsl_update.info.json`);
    log(`\n${colors.yellow}提示：將 .sgsl_update 與 .info.json 複製到目標機器的根目錄，Bootstrap.exe 啟動時會自動偵測。${colors.reset}`);
}

main().catch((err) => {
    error(`未預期錯誤：${err.message}`);
    process.exit(1);
});