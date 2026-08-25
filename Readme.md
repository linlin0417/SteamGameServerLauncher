本專案測試中 - by Saya  
目前僅限支援 icarus  
> 注意: 目前發現些微的記憶體使用異常與權限問題
> 注意: 本專案目前因為是demo與更新頻率較快所以不適合長期使用  請等待v2.0.0版本

目前進度[TODO.md](TODO.md)  
更新日誌[CHANGELOG.md](changelog.md)

## 如何使用
前往[releases](https://github.com/linlin0417/SteamGameServerLauncher/releases/latest)
請下載 `.exe` 安裝檔  
> 請注意: 請盡量安裝在非權限管制的資料夾中以避免需要UAC

### 匯入原版地圖 (手動打包)
如果您想要直接匯入原版地圖，可以使用我們提供的範例空檔案：
[下載 EmptyTemplate.SGSLMap (位於 Templates 目錄下)](Templates/EmptyTemplate.SGSLMap)

**匯入步驟：**
1. 將下載的 `EmptyTemplate.SGSLMap` 檔案副檔名改為 `.zip`。
2. 解壓縮該檔案。
3. 將您的原版地圖存檔（包含 `.json` 與 `.json.backup`）放入解壓縮後的 `saves/` 目錄中。
4. (非必要) 您可以編輯 `metadata.json` 修改地圖資訊。
5. 將 `metadata.json` 與 `saves/` 目錄重新打包成 `.zip` 壓縮檔。
6. 將該壓縮檔副檔名改為 `.SGSLMap` 或 `.IcarusMap`。
7. 在啟動器中匯入此檔案即可。
---
## 許可證
本專案原始碼採用 Apache License 2.0 授權。

- 完整授權條款請參閱專案根目錄的 `LICENSE`。
- Copyright (c) 2026 Yukimura Saya & The Dream Studio

## 往後的大致規劃
將會把ui重構成原本預想的樣子  
現在的ui是暫時測試的demo 預計會在v2.0.0進行重構  
但大致規劃可以透過 [ToDo](TODO.md) 與 [CHANGELOG](changelog.md) 查看

### Qt 相關授權說明
本專案使用 Qt。Qt 與其模組、外掛及相關函式庫不受本專案 Apache-2.0
條款覆蓋，仍各自適用其原始授權條款（例如 LGPL、GPL 或商業授權）。

當你重新散布本專案的執行檔或安裝包時，請自行確認並遵循所使用 Qt
版本與模組的授權義務，包含但不限於：

- 保留原始授權與版權聲明
- 提供相應的授權文件與第三方聲明
- 若適用 LGPL，遵守動態連結、可替換函式庫與必要的重連結要求
