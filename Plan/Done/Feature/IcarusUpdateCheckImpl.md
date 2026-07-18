# Icarus 伺服器更新檢查功能實作計畫

## 實作目標
基於先前的研究結論，將「比對本地 ACF 檔與 SteamCMD 線上 BuildID」的方案實作進入現有系統，提供低成本、高效率的伺服器更新檢查功能。

## 系統架構調整與實作細節

### 1. `SteamCmdManager` 模組修改
**檔案：`src/Core/SteamCmdManager.h` 及 `src/Core/SteamCmdManager.cpp`**
- **新增公開方法**：
  `void checkServerUpdate(const QString &appId, const QString &acfFilePath);`
- **新增信號 (Signals)**：
  `void updateCheckFinished(bool updateAvailable, const QString &localBuildId, const QString &onlineBuildId, const QString &message);`
- **新增內部狀態**：
  - `bool m_isCheckingUpdate;` 用於標記當前 `runSteamCmd` 是否為單純查詢狀態。
  - `QString m_outputCache;` 用於在檢查模式下暫存 SteamCMD 所有的文字輸出，以供後續正則表達式或狀態機解析。
- **流程邏輯**：
  當調用 `checkServerUpdate` 時，先讀取並解析 `acfFilePath` 取得 `localBuildId`。接著使用 `+login anonymous +app_info_update 1 +app_info_print [appId] +quit` 啟動 SteamCMD。
  在進程結束後 (`onProcessFinished`)，若為檢查模式，則從 `m_outputCache` 解析出 `onlineBuildId`，並將兩者對比，發射 `updateCheckFinished` 信號。

### 2. `MainWindow` UI 介面與交互修改
**檔案：`src/UI/MainWindow.h` 及 `src/UI/MainWindow.cpp`**
- **新增 UI 元素**：
  在控制頁籤 (Control Tab) 中的「更新伺服器」按鈕旁，新增一個「檢查更新」(`m_btnCheckServerUpdate`) 按鈕。
- **按鈕行為綁定**：
  綁定該按鈕至槽函數 `onCheckServerUpdate()`。
  取得目前的 `serverInstallDir` 後，推算出 ACF 檔案路徑（通常位於 `serverInstallDir` 外層的 `steamapps/appmanifest_2089300.acf`，若無則依 SteamCMD 架構尋找）。
- **接收檢查結果**：
  綁定 `SteamCmdManager::updateCheckFinished` 信號。當收到結果時：
  - 若 `updateAvailable` 為 true，則在 Log 中輸出紅字或以彈出視窗 (QMessageBox) 提示使用者有新版本，並顯示本地與線上版號。
  - 若無更新，則提示目前已是最新版本。

## 驗證與測試計畫
1. 編譯並啟動應用程式。
2. 點擊「檢查更新」按鈕，確認是否在數秒內回報版本資訊，且未觸發耗時的硬碟 validate 掃描。
3. 確認 Log 視窗正確印出本地與線上 BuildID。
