# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/0.3.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2026-07-20

### Added
- 實作 .IcarusMap 專有格式的地圖檔案匯出與匯入功能（以 ZIP 壓縮包裝，內含 metadata.json 與地圖存檔）。
- 於 MainWindow UI 中新增第四個「地圖管理」分頁，提供地圖備註填寫與預覽圖片選擇。
- 新增 MapPackager 核心模組，利用既有的 miniz 庫進行檔案的壓縮與解壓縮，無額外依賴。

### Changed
- 調整專案版本號至 1.2.0。

### Security
- 限制伺服器運行中禁止進行地圖匯出或匯入操作，防止存檔毀損。
- 匯入地圖時，若目標路徑已存在同名存檔，會跳出提示對話框由使用者決定是否覆寫。

## [1.1.0] - 2026-07-19

### Added
- 實作伺服器低成本更新檢查功能。
- 新增更新完成後自動偵測伺服器執行檔的功能。

### Removed
- 移除 Readme 中關於 AutoUpdater 下載問題的描述。

## [1.0.13] - 2026-07-19

### Added
- 新增更新完成後自動偵測伺服器執行檔的功能（後續於 1.1.0 整合與修正相關說明）。

## [1.0.11] - 2026-07-19

### Fixed
- 修正 CPack 打包時產生多餘的 bin 資料夾，導致更新路徑錯誤的問題。

## [1.0.10] - 2026-07-19

### Added
- 實作 Updater 根據安裝目錄寫入權限，動態決定是否觸發 UAC 權限請求。

## [1.0.8] - 2026-07-19

### Changed
- 更改預設下載路徑至程式根目錄下的 GameData 資料夾。

## [1.0.7] - 2026-07-19

### Fixed
- 修正更新時因權限不足導致無法寫入檔案的錯誤。

## [1.0.6] - 2026-07-19

### Changed
- 移除專案中所有 emoji 與狀態圖示符號（包含日誌中的幾何符號，回歸純文字與標準標籤如 [OK], [FAIL], [RUN], [WARN]），以符合跨系統平台文字渲染的一致性與視覺設計規範。

### Fixed
- 修正 SteamCMD 中文輸出亂碼的問題，解碼方式從 QString::fromLocal8Bit 改為 QString::fromUtf8。

## [1.0.4] - 2026-07-19

### Changed
- 更新 Readme 說明與問題列表。

## [1.0.3] - 2026-07-19

### Fixed
- 修正 README 格式並調整部分 UI。

## [1.0.2] - 2026-07-19

### Changed
- 更新 Readme 說明。

## [1.0.1] - 2026-07-19

### Changed
- 重構更新程式 (Updater) 為純 C++ 與 Win32 API，移除 Qt 依賴以避免自動更新時發生 DLL 檔案鎖定問題。
- 改用 Windows 內建的 tar 指令進行 ZIP 解壓縮，取代原有的 PowerShell。
- 修正 CMake CPack 設定，取消 ZIP 輸出的母目錄結構。

## [1.0.0] - 2026-07-18

### Added
- 初次發布，支援自動編譯與自動更新。

### Fixed
- 修復 GitHub Actions 自動編譯時權限不足的問題。
- 修復 CPack 階段時由於未忽略 _CPack_Packages 導致的打包無限迴圈。
- 更新安裝的 Qt 版本至 6.6.3 / 6.5.4 以避免編譯器相容性錯誤。
