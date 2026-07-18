# 移除專案中所有 Emoji 與相關圖示符號之計畫

## 目的
根據專案開發規範，全專案的所有文件、程式碼註解、提交訊息以及開發對話中，嚴格禁止出現 any emoji 或表情符號。本計畫旨在清理專案中所有已知的 emoji 以及會被系統渲染為 emoji 的功能圖示符號，並以純文字或無符號設計取代，確保介面簡潔且不影響功能。

## 掃描發現之符號與修改方案

### 1. Readme.md
- **Line 14**: `▶ Downloading SteamCMD`
  - 修改方案: 將 `▶` 替換為 `[RUN]`，調整為 `[RUN] Downloading SteamCMD`
- **Line 17**: `✘ Failed to write zip file.`
  - 修改方案: 將 `✘` 替換為 `[FAIL]`，調整為 `[FAIL] Failed to write zip file.`

### 2. src/UI/MainWindow.cpp (日誌輸出與按鈕圖示)
- **Line 86**: `appendLog(tr("✔ %1").arg(msg));`
  - 修改方案: 將 `✔` 替換為 `[OK]`
- **Line 88**: `appendLog(tr("✘ %1").arg(msg));`
  - 修改方案: 將 `✘` 替換為 `[FAIL]`
- **Line 95**: `appendLog(tr("▶ %1").arg(op));`
  - 修改方案: 將 `▶` 替換為 `[RUN]`
- **Line 106**: `appendLog(tr("⚠ Server crashed with exit code %1").arg(code));`
  - 修改方案: 將 `⚠` 替換為 `[WARN]`
- **Line 335**: `m_btnStart = new QPushButton(tr("▶  啟動伺服器"));`
  - 修改方案: 移除 `▶` 與空格，改為 `tr("啟動伺服器")`
- **Line 336**: `m_btnStop = new QPushButton(tr("■  停止伺服器"));`
  - 修改方案: 移除 `■` 與空格，改為 `tr("停止伺服器")`
- **Line 465**: `QPushButton *btnSave = new QPushButton(tr("💾  儲存設定"));`
  - 修改方案: 移除 `💾` 與空格，改為 `tr("儲存設定")`
- **Line 466**: `QPushButton *btnReset = new QPushButton(tr("↺  重置為預設"));`
  - 修改方案: 移除 `↺` 與空格，改為 `tr("重置為預設")`
- **Line 613**: `m_btnDownloadUpdate->setText(tr("⬇  Download Update"));`
  - 修改方案: 移除 `⬇` 與空格，改為 `tr("Download Update")`
- **Line 758**: `appendLog(tr("⚠ Failed to save settings."));`
  - 修改方案: 將 `⚠` 替換為 `[WARN]`

### 3. 關於狀態列的幾何圓點 `●`
- **位置**: `src/UI/MainWindow.cpp` (Line 324, 665, 671, 677, 683)
  - `● 已停止`、`● 啟動中...`、`● 運行中`、`● 停止中...`
- **說明**: `●` 是標準幾何圓形符號，一般不屬於 emoji。但在某些系統字型中，為避免其與 emoji 風格混淆或系統渲染問題，本計畫將維持或一併移除它。若一併移除，狀態標籤將只顯示文字，並維持既有的顏色樣式（例如綠色代表運行中，灰色代表已停止），這樣亦能清楚表達狀態。
- **決策**: 為了徹底清除所有非文字圖示，我們將 `●` 移除，只保留純文字狀態顯示，以確保最高的一致性。
  - `●  已停止` -> `已停止`
  - `●  啟動中...` -> `啟動中...`
  - `●  運行中` -> `運行中`
  - `●  停止中...` -> `停止中...`

## 驗證計畫
1. 程式碼修改完成後，使用 CMake 重新編譯專案，確保編譯無任何錯誤。
2. 執行應用程式，確認 UI 上的按鈕與狀態欄顯示正常，無亂碼，且已完全無 emoji。
3. 執行 test 目錄下的測試（若有），確保無迴歸問題。

## 執行結果與實作細節

本計畫已於 2026-07-18 成功執行：
1. 已經修改 `Readme.md`：
   - 移除日誌範例中的圖示符號。
2. 已經修改 `src/UI/MainWindow.cpp`：
   - 將 `appendLog` 中的 `✔` 替換為 `[OK]`。
   - 將 `appendLog` 中的 `✘` 替換為 `[FAIL]`。
   - 將 `appendLog` 中的 `▶` 替換為 `[RUN]`。
   - 將 `appendLog` 中的 `⚠` 替換為 `[WARN]`。
   - 移除按鈕上的 `▶` (啟動伺服器)、`■` (停止伺服器)、`💾` (儲存設定)、`↺` (重置為預設) 符號，改為純文字。
   - 將更新按鈕上的 `⬇` 移除，改為純文字 `Download Update`。
   - 移除狀態列 `m_statusLabel` 的 `●` 符號，依據計畫僅顯示純文字（如「已停止」、「運行中」），並依既有程式邏輯保留文字顏色。
3. 驗證結果：
   - 本機已配置 Qt 6.11.1 與 MinGW 13.1.0 環境，並成功編譯 `SteamGameServerLauncher` 與 `Updater` 目標，無任何錯誤。
   - 進行了啟動測試，確認應用程式能夠正常載入。
