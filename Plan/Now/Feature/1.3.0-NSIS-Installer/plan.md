# 1.3.0 NSIS 安裝包製作

## 目標
為 SteamGameServerLauncher 建立完整的 NSIS .exe 安裝檔，取代原有的 CPack WiX/NSIS 混合方案。

## 變更摘要

### 新增檔案
- `src/UI/SGSL_icon.ico` - 多尺寸應用程式圖示 (16/32/48/64/128/256 px)
- `installer/sgsl_installer.nsi` - 獨立 NSIS 安裝腳本
- `installer/licenses/EULA_zh.md` - 繁體中文版 EULA
- `installer/licenses/EULA_en.md` - 英文版 EULA
- `installer/licenses/QT_License_zh.md` - 繁體中文版 Qt 授權條款
- `installer/licenses/QT_License_en.md` - 英文版 Qt 授權條款
- `installer/licenses/Data_Collection_zh.md` - 繁體中文版資料收集協定
- `installer/licenses/Data_Collection_en.md` - 英文版資料收集協定

### 修改檔案
- `CMakeLists.txt` - CPack 簡化為僅 ZIP、更新版權所有人、加入圖示安裝
- `.github/workflows/build-and-release.yml` - 加入 NSIS 安裝、Markdown 轉 txt、makensis 編譯步驟
- `Readme.md` - 下載說明從 .msi 改為 .exe、更新版權所有人

### 功能特性
1. 安裝啟動時彈出 6 種語言選擇（繁體中文/英文/日文/西班牙語/法文/德文）
2. 3 個獨立授權頁面：EULA、Qt 授權、資料收集協定
3. 授權文字根據語言切換：繁體中文顯示繁中版，其餘語言顯示英文版
4. 安裝路徑偵測：選擇 Program Files 等權限管制區域時彈出 UAC 警告
5. 安裝完成後建立桌面與開始選單捷徑（含圖示）
6. 寫入「新增/移除程式」登錄檔供系統管理
7. CI/CD 每次編譯時自動從 .md 來源產生授權 .txt 檔

### 版權所有人
Yukimura Saya & The Dream Studio

## 狀態
進行中
