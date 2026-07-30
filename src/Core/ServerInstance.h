#pragma once

#include <QObject>
#include <QJsonObject>
#include "GameProfile.h"
#include "ServerManager.h"

class SteamCmdManager;
class DiscordManager;

/// 封裝一個伺服器實例的完整狀態。
/// 包含遊戲設定檔、使用者設定、進程管理、SteamCMD 操作與 Discord 通知。
class ServerInstance : public QObject
{
    Q_OBJECT

public:
    explicit ServerInstance(SteamCmdManager *steamCmd, QObject *parent = nullptr);
    ~ServerInstance() override;

    // --- 遊戲設定檔 ---
    GameProfile profile() const;
    void setProfile(const GameProfile &p);

    // --- 實例設定 ---
    QJsonObject settings() const;
    void setSettings(const QJsonObject &s);
    void loadSettings(const QString &filePath);
    void saveSettings(const QString &filePath) const;
    void saveSettings() const;
    
    /// 產生合併遊戲預設值與使用者設定後的完整設定
    QJsonObject mergedSettings() const;

    // --- 安裝目錄 ---
    QString installDir() const;
    void setInstallDir(const QString &dir);
    
    /// 取得伺服器執行檔的完整路徑
    QString serverExePath() const;
    
    /// 自動偵測伺服器執行檔
    bool autoDetectServerExe();

    // --- 伺服器操作 ---
    void installOrUpdate();
    void checkUpdate();
    void startServer();
    void stopServer();
    bool isRunning() const;
    ServerManager::ServerState serverState() const;

    // --- Discord ---
    DiscordManager *discordManager() const;

signals:
    void logMessage(const QString &msg);
    void stateChanged(ServerManager::ServerState state);
    void serverCrashed(int exitCode);
    void operationFinished(bool success, const QString &msg);
    void updateCheckFinished(bool hasUpdate, const QString &localVer, const QString &onlineVer, const QString &msg);

public:
    /// 根據 GameProfile 的 configFormat/configMappings 應用遊戲設定
    void applyGameConfig();

private:

    GameProfile m_profile;
    QJsonObject m_settings;
    QString m_installDir;
    mutable QString m_settingsFilePath;

    SteamCmdManager *m_steamCmd = nullptr; // 共用，不擁有
    ServerManager *m_serverMgr = nullptr;  // 擁有
    DiscordManager *m_discordMgr = nullptr; // 擁有
};
