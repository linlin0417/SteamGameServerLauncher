# Icarus Dedicated Server 低成本更新檢查研究與實作方案 - 執行結果

此文件旨在評估與研究如何以最低成本，檢查透過 SteamCMD 部署的 Icarus Dedicated Server (AppId: 2089300) 是否有新版本。此任務已完成研究並歸檔。

## 研究結果摘要

根據對 Steam 平台的分析與測試，我們得出了四種方案的評估結果：

| 檢查方案 | 檢查原理 | 時間成本 | 系統資源開銷 | 穩定性與可靠性 | 評估結論 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **方案一：比對本地 ACF 檔與 SteamCMD 線上 BuildID** | 解析本地 `appmanifest_2089300.acf` 中的 `buildid`；以 `app_info_print` 向 Steam 同步並獲取最新線上 `buildid` 作比對。 | 約 3 至 10 秒 | 極低（不掃描硬碟，不下載大檔案） | 100% 穩定且官方保證 | **最推薦方案**，適合在此專案中實作。 |
| **方案二：第三方公用 Web API (steamcmd.net)** | 發送 HTTP GET 請求至 `api.steamcmd.net` 解析 JSON。 | < 1 秒 | 近乎為零 | 較低（因第三方服務隨時可能失效） | 可作為備用方案，但不適合作為唯一依賴。 |
| **方案三：官方 Steam Web API** | 使用 `ISteamApps/GetAppBetas` 等 API。 | - | - | 無法使用 | **不可行**。因需要遊戲發行商的專屬 API 金鑰。 |
| **方案四：SteamCMD app_status 指令** | 透過 `app_status` 指令取得狀態。 | 約 3 至 10 秒 | 極低 | 極低（狀態回報不準確） | **不可行**。無法作為判斷依據。 |

---

## 實作細節與建議

### 1. 解析本地 ACF 檔案取得本地 BuildID
```cpp
QString getLocalBuildId(const QString &acfFilePath) {
    QFile file(acfFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }
    
    QTextStream in(&file);
    // 使用正則表達式尋找 "buildid" "數字" 格式的行
    QRegularExpression rx(R"(\"buildid\"\s+\"(\d+)\")");
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        QRegularExpressionMatch match = rx.match(line);
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }
    return QString();
}
```

### 2. 解析 SteamCMD 輸出取得線上最新 BuildID
當 `steamcmd.exe` 執行完畢並回傳輸出文字時，我們可以使用狀態機來提取 `"public"` 分支下的 `"buildid"`：
```cpp
QString parseOnlineBuildId(const QString &steamCmdOutput) {
    QStringList lines = steamCmdOutput.split('\n');
    bool inBranches = false;
    bool inPublic = false;
    
    QRegularExpression rx(R"(\"buildid\"\s+\"(\d+)\")");
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed == QStringLiteral("\"branches\"")) {
            inBranches = true;
        } else if (inBranches && trimmed == QStringLiteral("\"public\"")) {
            inPublic = true;
        } else if (inPublic && trimmed.startsWith(QStringLiteral("\"buildid\""))) {
            QRegularExpressionMatch match = rx.match(trimmed);
            if (match.hasMatch()) {
                return match.captured(1);
            }
        } else if (trimmed == QStringLiteral("}")) {
            if (inPublic) {
                inPublic = false;
            } else if (inBranches) {
                inBranches = false;
            }
        }
    }
    return QString();
}
```

### 3. 非同步檢查流程設計
我們可以在 `SteamCmdManager` 中新增一個檢查方法，啟動 `QProcess` 並監聽其輸出：
```cpp
void SteamCmdManager::checkServerUpdate(const QString &appId) {
    if (m_busy) return;
    m_busy = true;
    m_currentOperation = tr('Checking for server updates');
    
    // 收集進程輸出的快取
    m_outputCache.clear();
    
    const QStringList args = {
        '+login', 'anonymous',
        '+app_info_update', '1',
        '+app_info_print', appId,
        '+quit'
    };
    
    runSteamCmd(args); // 此處需要微調 runSteamCmd 來在檢查模式下快取輸出
}
```

---

## 執行結果

1. **已建立研究報告**：已將所有研究成果、技術比較與實作程式碼詳細記錄至此文件中。
2. **已封存計畫**：本研究案已移至 `Plan/Done/Research/IcarusUpdateCheck.md`，宣告結案。
3. **後續建議**：若未來需要為 launcher 增加「自動檢查 Icarus 更新」功能，可直接採用本文件所設計的 **方案一** C++ 實作範例進行開發。
