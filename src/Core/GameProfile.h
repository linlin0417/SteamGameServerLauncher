#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

/// 遊戲伺服器設定檔資料模型。
/// 定義一款遊戲伺服器的所有可參數化屬性。
struct GameProfile {
    QString id;                // 唯一識別碼 "icarus", "palworld", "custom_001"
    QString displayName;       // 顯示名稱

    // --- 安裝來源 ---
    enum SourceType { SteamCMD, CustomScript, ManualOnly };
    SourceType sourceType = ManualOnly;
    QString steamAppId;        // SteamCMD 來源: App ID
    QString installScript;     // CustomScript 來源: 安裝用腳本路徑
    QString updateScript;      // CustomScript 來源: 更新用腳本路徑

    // --- 執行檔 ---
    QString exeRelativePath;   // 相對於安裝目錄的執行檔路徑
    QString launchArgsTemplate;// 啟動參數模板, 使用 {variableName} 語法
    bool isJavaApp = false;    // 是否為 Java 應用（需 java -jar 啟動）

    // --- 設定檔適配 ---
    enum ConfigFormat { None, INI, Properties, JSON_Config };
    ConfigFormat configFormat = None;
    QString configFilePath;    // 遊戲設定檔相對路徑
    QString configSection;     // INI 格式：目標區段名
    QJsonObject configMappings;// key -> 模板變數對應表
    QStringList configDefaultContent; // 設定檔預設內容（逐行），檔案不存在時以此建立

    // --- 存檔管理 ---
    QString savesRelativePath; // 存檔目錄相對路徑
    QStringList saveFilePatterns; // 存檔匹配模式

    // --- 預設值 ---
    int defaultPort = 0;
    int defaultQueryPort = 0;
    int defaultMaxPlayers = 0;
    QJsonObject extraDefaults; // 遊戲專屬額外預設值

    // --- 序列化 ---
    QJsonObject toJson() const;
    static GameProfile fromJson(const QJsonObject &obj);

    // --- 模板工具 ---
    /// 將模板字串中的 {variableName} 替換為 vars 中對應的值
    static QString expandTemplate(const QString &tpl, const QJsonObject &vars);
    
    /// 根據 launchArgsTemplate 和使用者設定產生啟動參數列表
    QStringList buildLaunchArgs(const QJsonObject &settings) const;
    
    /// 從 launchArgsTemplate + configMappings 中提取所有變數名
    QStringList extractVariableNames() const;

    // --- 查詢 ---
    /// 是否為內建設定檔（不可刪除）
    bool isBuiltin() const;
    
    /// 是否有效（最少需要 id 和 displayName）
    bool isValid() const;

    // 內建設定檔 ID 清單
    static QStringList builtinProfileIds();
};
