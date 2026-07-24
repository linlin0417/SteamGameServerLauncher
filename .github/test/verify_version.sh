#!/bin/bash
set -e

# 讀取 CMakeLists.txt 並提取版本號
VERSION=$(sed -n 's/.*project(.*VERSION[[:space:]]*\([0-9.]*\).*/\1/p' CMakeLists.txt | head -n 1)

if [ -z "$VERSION" ]; then
    echo "[FAIL] 錯誤：無法從 CMakeLists.txt 提取版本號。"
    exit 1
fi

echo "==== 版本驗證開始 ===="
echo "從 CMakeLists.txt 獲取到的版本號為: $VERSION"

# 檢查標籤是否存在
if git rev-parse "v$VERSION" >/dev/null 2>&1; then
    echo "[FAIL] 發布中斷：版本號 v$VERSION 已經存在於 Git 標籤中！"
    echo "請確認您是否忘記更新版本號，本次發布流程已被強制中斷。"
    exit 1
else
    echo "[OK] 版本號檢查通過：v$VERSION 是一個尚未發布的新版本。"
fi

echo "======================"
