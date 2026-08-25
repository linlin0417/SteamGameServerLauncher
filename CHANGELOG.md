# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/0.3.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [2.0.16] - 2026-08-25

### Fixed
- 重構地圖存檔包裝核心 (`MapPackager`)，使其動態支援讀取多遊戲不同的 `saveFilePatterns`，不再硬編碼 Icarus 專用的 `.json` 與 `.json.backup`。
- 修復 `SaveManagerPanel` 匯出存檔列表在遇到多重副檔名（如 `.json.backup`）時發生重複讀取，並導致檔案名稱截斷錯誤的問題。
- 修復 `SaveManagerPanel` 匯出存檔時遺漏寫入 UI 2.0 新增的 `formatVersion` 與 `saveFilePatterns` 中繼資料，解決這些地圖包被誤判為舊版格式的問題。

## [2.0.15] - 2026-08-24

### Fixed
- 修復控制面板中的「安裝 SteamCMD」按鈕點擊後完全沒有反應的問題。由於該按鈕在先前的 UI 重構中遺漏了事件綁定，現已將其正確連接至 SteamCMD 的下載初始化流程。

## [2.0.14] - 2026-08-24

### Added
- 在「伺服器參數」面板的「除錯功能」區塊中，新增「開啟獨立 Console 視窗」按鈕，方便使用者將日誌獨立為獨立視窗，並於切換分頁操作（如更新伺服器）時持續監看完整日誌。

## [2.0.13] - 2026-08-22

### Fixed
- 新增安裝路徑的非英文字元檢查以防止 SteamCMD 啟動崩潰。
- 新增在解壓縮後自動執行 `+quit` 指令以完整初始化 SteamCMD。
- 修復 SteamCMD 自我更新 (Exit Code 7) 會中斷後續伺服器安裝指令的問題，現在會自動重試原本的指令。

## [2.0.12] - 2026-07-30

### Fixed
- 修復點擊「恢復建議設定」時，`port`、`queryPort` 與 `maxPlayers` 會錯誤地被清零（並自動變為 1）的問題。現已確保會正確讀取對應遊戲的基礎預設值（例如：17777、27015）。

## [2.0.11] - 2026-07-30

### Added
- 於伺服器參數面板新增「恢復建議設定」按鈕。
- 支援一鍵將大部份參數還原為伺服器的官方推薦建議設定 (並依使用者要求特別將 `AdminPassword` 推薦值調整為 `12345678`)。
- 確保恢復設定時會略過 `SessionName` 欄位，保護玩家的伺服器名稱不被重置。

## [2.0.10] - 2026-07-30

### Fixed
- 修復伺服器安裝目錄路徑錯誤的 Bug。原本系統錯誤地將伺服器檔案指向 `GameData/instances/`，現已修正為正確的 `GameData/servers/`，並與設定檔所在的 `instances/` 資料夾做出正確的職責分離。

## [2.0.9] - 2026-07-30

### Added
- 在「伺服器參數」面板新增「除錯功能」區塊。
- 新增「開啟 INI 設定檔」按鈕，可直接用系統預設文字編輯器打開真實伺服器設定檔 (例如 `ServerSettings.ini`)。
- 新增「開啟設定檔目錄」按鈕，可直接開啟設定檔所在的資料夾，方便玩家手動除錯與檢查。
- 上述按鈕具備防呆機制，當檔案或目錄尚未建立時，會於日誌提示錯誤而不會崩潰。

## [2.0.8] - 2026-07-30

### Changed
- 完整重構伺服器設定檔 (INI) 雙向同步讀取邏輯。UI 參數面板現在會優先讀取並同步真實伺服器設定檔中的數值。
- 更新 Icarus 設定檔範本，補齊高達 21 項全新伺服器進階設定欄位，並自動對應至 UI 供玩家修改。
- 伺服器設定面板支援以 `extraDefaults` 顯示潛在的預設值 (Placeholder)。
- 點擊「儲存設定」時，現在會立即將設定檔寫回至硬碟 (INI)。

### Fixed
- 增強路徑與安裝狀態防呆機制：在伺服器尚未安裝時切換至參數面板，將顯示提示並平滑降級使用內部預設值，避免程式出錯。

## [2.0.7] - 2026-07-30

### Fixed
- 修復 Icarus `ServerSettings.ini` 首次建立時內容不完整的問題：新增 `configDefaultContent` 機制，當設定檔不存在時先以預設範本建立完整檔案，再執行 key 替換。解決首次啟動時僅寫入 3 個 key 而遺失其他必要預設值的問題。

## [2.0.6] - 2026-07-30

### Fixed
- 移除 Icarus 啟動參數中多餘的 `-UserDir`，該參數會讓 Unreal Engine 改變設定檔的讀取路徑，導致啟動器寫入的 `ServerSettings.ini` 與伺服器實際讀取的檔案不在同一個位置。移除後伺服器將回歸預設路徑 `Icarus/Saved/Config/WindowsServer/`，與啟動器寫入位置一致，密碼與人數上限設定得以正確生效。
- 新增 `applyGameConfig` 偵錯日誌，啟動時會輸出設定檔完整寫入路徑與鍵值內容，便於日後排查。

## [2.0.2] - 2026-07-30

### Fixed
- 將 MainWindow 中的 QHBoxLayout 替換為 QSplitter，新增左右動態調整大小功能，解決側邊欄擠壓問題。
- 將 SidebarWidget 的固定寬度改為最小寬度限制，配合 QSplitter 讓使用者自由縮放。
- 修正側邊欄左上角版本標籤文字寫死為舊版本號的問題，改為動態讀取系統設定的 APP_VERSION。

## [2.0.1] - 2026-07-30

### Fixed
- 修復 Icarus 伺服器啟動時，未強制指定 `-UserDir` 導致伺服器讀取錯誤路徑下的設定檔，進而使密碼設定失效的問題。
- 修復 Icarus 的 `MaxPlayers` (加入人數上限) 設定無效的問題：將其從啟動參數移至 `ServerSettings.ini` 中，以符合 Icarus 伺服器的讀取規範。
- 修復在伺服器設定面板中修改設定後，若未點擊儲存按鈕直接點擊啟動，會導致變更未生效的問題。現在點擊啟動前會自動儲存目前的設定。
- 在 `ServerInstance` 注入 `installDir` 變數至環境設定中，確保支援自定義啟動參數引用該路徑，並轉換為 Windows 原生路徑分隔符號以避免解析錯誤。

## [2.0.0] - 2026-07-30

### Added
- 新增 `GameProfile` 資料模型與 `GameProfileManager`，實作完整的遊戲設定檔生命週期管理。
- 新增 `ServerInstance` 抽象層，完整封裝單一伺服器實例的狀態（設定、進程、Discord 通知）。
- 內建 Icarus、Palworld、Rust、Minecraft Paper 四款預設遊戲設定檔，支援自動化安裝與啟動設定。
- 新增 `SettingsMigrator`，支援 v1.x 版本設定的無痛自動遷移至 v2.0.0 格式。
- 新增支援自訂腳本（.bat/.ps1）的安裝與更新模式。
- 新增 `.SGSLMap` 存檔打包格式，包含版本資訊並完全向下相容舊版的 `.IcarusMap` 格式。

### Changed
- 將原先專屬於 Icarus 的工具架構，全面重構為通用的多遊戲伺服器管理啟動器。
- 將 `SteamCmdManager` 參數化，不再硬編碼特定 AppID。
- 將 `ServerManager` 中的設定檔套用邏輯通用化，支援 INI、Properties、JSON 等多種遊戲設定檔格式。
- 全面重寫 UI 介面，從舊版 `QTabWidget` 架構改為側邊導覽列（Sidebar）搭配 `QStackedWidget` 的模組化殼層架構。
- 拆分出獨立的 UI 元件：`SidebarWidget`、`ServerControlPanel`、`ServerSettingsPanel`、`SaveManagerPanel`、`GameProfileDialog` 與 `AboutPanel`。
- `ServerSettingsPanel` 現能根據 `GameProfile` 動態生成對應的設定表單。
## [1.5.2] - 2026-07-27

### Added
- 新增預設下載大小與發布日期的資訊於更新介面。
- 於 GitHub Actions 編譯時自動產生所有檔案的 SHA256，並附加於 Release Notes。
- 應用程式自動下載更新時，會抓取線上發布的 `sha256sums.txt` 進行本地 ZIP 檔案的 SHA256 驗證，保障下載更新包的完整與安全性。

## [1.5.1] - 2026-07-27

### Added
- 修改自我更新介面 (Self Update)，現在檢查更新時會以 Markdown 格式直接於介面內顯示 GitHub Release 的發布日誌，方便使用者閱讀改版內容。

## [1.5.0] - 2026-07-26

### Changed
- 改良 CI/CD 自動發布流程，新增 PowerShell 腳本自動從 CHANGELOG.md 提取當前版本的更新日誌內容，並作為 GitHub Release 的內文，取代原有的預設固定文字。

## [1.4.2] - 2026-07-25

### Fixed
- 修正伺服器設定頁面 (Settings) 於視窗高度不足時無法完整顯示的問題，加入了捲動條 (QScrollArea) 以便使用者瀏覽。

## [1.4.0] - 2026-07-24

### Added
- 新增 Discord Hook 通知功能。透過 HTTP POST 支援 Discord Rich Embeds 格式的事件推播（啟動、關閉、崩潰、更新完成、地圖操作）。

## [1.3.1] - 2026-07-24

### Fixed
- 修復 NSIS 安裝包中的多國語系文字與授權書中文顯示亂碼問題（強制將編譯用的文字檔與腳本加上 UTF-8 BOM）。

## [1.3.0] - 2026-07-23

### Added
- 將打包機制從 WiX (MSI) 完全轉換為獨立的 NSIS 可執行安裝檔 (.exe)。
- 支援安裝時 6 種語言選擇（繁體中文、英文、日文、西班牙語、法文、德文），並動態切換 UI 與授權書語系。
- 新增三個獨立授權頁面（EULA、Qt 授權條款、資料收集協定），支援繁體中文與英文雙語切換。
- 加入 UAC 權限警告機制，當使用者選擇安裝於 Program Files 等受限資料夾時會跳出提示。
- 安裝完成後自動建立桌面與開始選單捷徑。
- CI/CD 整合 PowerShell 腳本，自動將 Markdown 格式的授權檔案轉換為純文字檔供 NSIS 讀取。
- 更新版權所有人聲明為 Yukimura Saya & The Dream Studio，並加入與 Valve 相關的商標免責聲明。

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
