# v1.2.0 - Icarus 地圖檔案匯出與匯入功能

## 實作目標
實作 `.IcarusMap` 專有格式的地圖檔案匯出與匯入功能。此格式本質上是一個 ZIP 壓縮檔，內含遊戲地圖存檔以及額外的中繼資料（名稱、備註、時間戳、預覽圖片等）。匯入時僅將遊戲存檔解壓至伺服器目錄，額外資訊僅供啟動器顯示，不會影響遊戲本身。

## 實作成果

### 新增檔案
- `src/Core/MapPackager.h` - MapMetadata 結構定義與 MapPackager 靜態工具類別宣告
- `src/Core/MapPackager.cpp` - 完整的匯出、匯入、中繼資料讀取、預覽圖片讀取邏輯

### 修改檔案
- `CMakeLists.txt` - 加入 MapPackager 與 miniz 編譯來源、新增 updater include 路徑、版本號更新至 1.2.0
- `src/UI/MainWindow.h` - 新增地圖管理 Tab 相關 UI 元件宣告與方法宣告
- `src/UI/MainWindow.cpp` - 新增第四個「地圖管理」Tab、實作完整匯出/匯入 UI 流程

### .IcarusMap 檔案格式規格
```
*.IcarusMap (ZIP 壓縮檔)
  +-- metadata.json            // 中繼資料
  +-- preview.png              // 預覽圖片（選填）
  +-- saves/
      +-- [ProspectName].json          // 地圖存檔主檔
      +-- [ProspectName].json.backup   // 地圖存檔備份（若存在）
```

### metadata.json 欄位
- packageName: 地圖包顯示名稱
- originalProspectName: 原始存檔名
- notes: 使用者自訂備註
- timestamp: 匯出時間戳（ISO 8601）
- launcherVersion: 匯出時的啟動器版本
- hasPreview: 是否包含預覽圖片

### 核心技術決策
- 使用專案既有的 miniz 壓縮庫，不引入新依賴
- 伺服器運行中禁止匯出/匯入操作，防止存檔損毀
- 匯入時僅解壓 saves/ 目錄下的檔案至伺服器 Prospects 目錄
- 匯入時若同名檔案已存在，彈出對話框由使用者決定是否覆寫

## 編譯驗證結果
- MinGW GCC 13.1.0 + Qt 6.11.1 編譯成功
- 所有 8 個編譯單元正常通過
- windeployqt 部署完成
