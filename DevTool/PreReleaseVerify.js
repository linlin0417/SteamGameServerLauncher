const { execSync } = require('child_process');
const fs = require('fs');
const readline = require('readline');
const path = require('path');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

// Helper function for colored console output
const colors = {
    reset: "\x1b[0m",
    red: "\x1b[31m",
    green: "\x1b[32m",
    yellow: "\x1b[33m",
    cyan: "\x1b[36m"
};

console.log(`${colors.cyan}==== 開始發布前本地驗證程序 ====${colors.reset}\n`);

// 1. 執行 CMake 基礎建置驗證
console.log(`${colors.yellow}1. 正在執行本地 CMake 基礎編譯測試...${colors.reset}`);
try {
    // 確保有 build 目錄
    execSync('cmake -S . -B build -DCMAKE_BUILD_TYPE=Release', { stdio: 'inherit', cwd: path.join(__dirname, '..') });
    // 執行建置
    execSync('cmake --build build --config Release --parallel', { stdio: 'inherit', cwd: path.join(__dirname, '..') });
    console.log(`${colors.green}✓ 本地編譯測試通過。${colors.reset}\n`);
} catch (error) {
    console.error(`${colors.red}✗ 編譯失敗，請先修復程式碼錯誤後再嘗試發布。${colors.reset}`);
    process.exit(1);
}

// 2. 獲取當前 CMakeLists.txt 版本號
console.log(`${colors.yellow}2. 檢查版本號更新狀態...${colors.reset}`);
let currentVersion = '';
try {
    const cmakeContent = fs.readFileSync(path.join(__dirname, '../CMakeLists.txt'), 'utf8');
    const match = cmakeContent.match(/project\([^)]+VERSION\s+([0-9.]+)/);
    if (match && match[1]) {
        currentVersion = match[1];
        console.log(`目前 CMakeLists.txt 中的版本號為: ${colors.cyan}v${currentVersion}${colors.reset}`);
    } else {
        throw new Error("無法從 CMakeLists.txt 解析出版本號。");
    }
} catch (error) {
    console.error(`${colors.red}✗ 讀取版本號失敗: ${error.message}${colors.reset}`);
    process.exit(1);
}

// 3. 比對 Git Tag 檢查是否重複
let versionUpdated = true;
try {
    const tagOutput = execSync(`git tag -l "v${currentVersion}"`, { encoding: 'utf8', cwd: path.join(__dirname, '..') }).trim();
    if (tagOutput === `v${currentVersion}`) {
        versionUpdated = false;
        console.warn(`\n${colors.red}!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!${colors.reset}`);
        console.warn(`${colors.red}!! 警告：版本號 v${currentVersion} 已經存在於 Git 標籤中！ !!${colors.reset}`);
        console.warn(`${colors.red}!! 您似乎忘記在 CMakeLists.txt 更新版本號了。      !!${colors.reset}`);
        console.warn(`${colors.red}!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!${colors.reset}\n`);
    } else {
        console.log(`${colors.green}✓ 版本號 v${currentVersion} 是新的，尚未發布。${colors.reset}\n`);
    }
} catch (error) {
    console.error(`${colors.red}✗ 執行 git 指令時發生錯誤。${colors.reset}`);
}

// 4. 顯示目前 Git 狀態
console.log(`${colors.yellow}3. 檢查 Git 狀態...${colors.reset}`);
try {
    const statusOutput = execSync('git status -s', { encoding: 'utf8', cwd: path.join(__dirname, '..') });
    if (statusOutput) {
        console.log(statusOutput);
    } else {
        console.log("目前工作目錄乾淨。");
    }
} catch (error) {
    // 忽略 Git Status 錯誤
}

// 5. 提示使用者進行最終確認
console.log(`\n${colors.cyan}====================================${colors.reset}`);
if (!versionUpdated) {
    console.log(`${colors.red}請注意：目前版本號未更新，強行發布將會被 GitHub Actions 阻擋！${colors.reset}`);
}
rl.question('是否確認將目前的變動加入 Git (執行 git add .) 並準備發布？ (Y/N): ', (answer) => {
    if (answer.trim().toUpperCase() === 'Y') {
        try {
            console.log(`\n${colors.yellow}正在執行 git add . ...${colors.reset}`);
            execSync('git add .', { stdio: 'inherit', cwd: path.join(__dirname, '..') });
            console.log(`${colors.green}✓ 已成功執行 git add .${colors.reset}`);
            console.log(`\n${colors.cyan}提示：請手動執行 git commit (需包含 AndBuild) 並 git push 完成發布流程。${colors.reset}`);
            
            // Generate the recommended git commit command based on user's rule
            console.log(`\n${colors.yellow}建議的 Commit 指令 (請自行複製執行)：${colors.reset}`);
            console.log(`git commit -m "Bump version to ${currentVersion}" -m "AndBuild"`);
            
        } catch (error) {
            console.error(`${colors.red}✗ 執行 git add 時發生錯誤。${colors.reset}`);
        }
    } else {
        console.log(`\n${colors.yellow}已取消發布準備。${colors.reset}`);
    }
    rl.close();
});
