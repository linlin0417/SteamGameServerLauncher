#include "ServerInstance.h"
#include "SteamCmdManager.h"
#include "DiscordManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

ServerInstance::ServerInstance(SteamCmdManager *steamCmd, QObject *parent)
    : QObject(parent)
    , m_steamCmd(steamCmd)
{
    m_serverMgr = new ServerManager(this);
    m_discordMgr = new DiscordManager(this);

    // m_serverMgr 信號連接
    connect(m_serverMgr, &ServerManager::stateChanged, this, [this](ServerManager::ServerState state) {
        emit stateChanged(state);
        
        QString stateStr;
        switch (state) {
            case ServerManager::ServerState::Starting: stateStr = "啟動中..."; break;
            case ServerManager::ServerState::Running: stateStr = "已啟動"; break;
            case ServerManager::ServerState::Stopping: stateStr = "停止中..."; break;
            case ServerManager::ServerState::Stopped: stateStr = "已停止"; break;
        }
        
        if (!stateStr.isEmpty()) {
            m_discordMgr->sendEmbedMessage(m_profile.displayName, QString("伺服器狀態變更: %1").arg(stateStr), DiscordManager::ColorInfo);
        }
    });

    connect(m_serverMgr, &ServerManager::logMessage, this, &ServerInstance::logMessage);

    connect(m_serverMgr, &ServerManager::serverCrashed, this, [this](int exitCode) {
        emit serverCrashed(exitCode);
        m_discordMgr->sendEmbedMessage(m_profile.displayName, QString("伺服器異常崩潰，結束代碼: %1").arg(exitCode), DiscordManager::ColorError);
    });

    // m_steamCmd 信號連接
    if (m_steamCmd) {
        connect(m_steamCmd, &SteamCmdManager::operationFinished, this, [this](bool success, const QString &msg) {
            emit operationFinished(success, msg);
            if (success) {
                autoDetectServerExe();
            }
            int color = success ? DiscordManager::ColorSuccess : DiscordManager::ColorError;
            m_discordMgr->sendEmbedMessage(m_profile.displayName, QString("安裝/更新操作完成: %1").arg(msg), color);
        });

        connect(m_steamCmd, &SteamCmdManager::updateCheckFinished, this, &ServerInstance::updateCheckFinished);
        connect(m_steamCmd, &SteamCmdManager::logMessage, this, &ServerInstance::logMessage);
    }
}

ServerInstance::~ServerInstance()
{
}

GameProfile ServerInstance::profile() const
{
    return m_profile;
}

void ServerInstance::setProfile(const GameProfile &p)
{
    m_profile = p;
}

QJsonObject ServerInstance::settings() const
{
    return m_settings;
}

void ServerInstance::setSettings(const QJsonObject &s)
{
    m_settings = s;
}

void ServerInstance::loadSettings(const QString &filePath)
{
    m_settingsFilePath = filePath;
    m_settings = ServerManager::loadSettings(filePath);
}

void ServerInstance::saveSettings(const QString &filePath) const
{
    m_settingsFilePath = filePath;
    ServerManager::saveSettings(filePath, m_settings);
}

void ServerInstance::saveSettings() const
{
    if (!m_settingsFilePath.isEmpty()) {
        saveSettings(m_settingsFilePath);
    }
}

QJsonObject ServerInstance::mergedSettings() const
{
    QJsonObject merged = m_profile.extraDefaults;
    
    merged["defaultPort"] = m_profile.defaultPort;
    merged["defaultQueryPort"] = m_profile.defaultQueryPort;
    merged["defaultMaxPlayers"] = m_profile.defaultMaxPlayers;

    for (auto it = m_settings.constBegin(); it != m_settings.constEnd(); ++it) {
        merged.insert(it.key(), it.value());
    }

    return merged;
}

QString ServerInstance::installDir() const
{
    if (!m_installDir.isEmpty()) {
        return m_installDir;
    }
    
    QString dataRootDir = QCoreApplication::applicationDirPath() + "/GameData";
    return dataRootDir + "/servers/" + m_profile.id;
}

void ServerInstance::setInstallDir(const QString &dir)
{
    m_installDir = dir;
}

QString ServerInstance::serverExePath() const
{
    if (m_settings.contains("serverExePath")) {
        return m_settings["serverExePath"].toString();
    }
    
    return installDir() + "/" + m_profile.exeRelativePath;
}

bool ServerInstance::autoDetectServerExe()
{
    QString expectedPath = installDir() + "/" + m_profile.exeRelativePath;
    if (QFileInfo::exists(expectedPath)) {
        m_settings["serverExePath"] = expectedPath;
        return true;
    }
    return false;
}

void ServerInstance::installOrUpdate()
{
    if (!m_steamCmd) {
        emit logMessage("SteamCMD 管理器未設定，無法執行安裝或更新");
        return;
    }

    switch (m_profile.sourceType) {
        case GameProfile::SteamCMD:
            m_steamCmd->installOrUpdateServer(m_profile.steamAppId, installDir());
            break;
        case GameProfile::CustomScript:
            m_steamCmd->runCustomScript(m_profile.installScript, installDir());
            break;
        case GameProfile::ManualOnly:
            emit logMessage("此遊戲僅支援手動安裝，請依照官方指示進行安裝。");
            emit operationFinished(false, "此遊戲僅支援手動安裝。");
            break;
    }
}

void ServerInstance::checkUpdate()
{
    if (!m_steamCmd) {
        emit logMessage("SteamCMD 管理器未設定，無法檢查更新");
        return;
    }

    if (m_profile.sourceType == GameProfile::SteamCMD) {
        m_steamCmd->checkServerUpdate(m_profile.steamAppId, installDir());
    } else {
        emit logMessage("此遊戲不支援透過 SteamCMD 檢查更新。");
    }
}

void ServerInstance::startServer()
{
    if (isRunning()) {
        emit logMessage("伺服器已在執行中。");
        return;
    }

    applyGameConfig();

    QStringList args = m_profile.buildLaunchArgs(mergedSettings());
    m_serverMgr->setServerExecutable(serverExePath());
    m_serverMgr->startServer(args);
    
    m_discordMgr->sendEmbedMessage(m_profile.displayName, "正在啟動伺服器...", DiscordManager::ColorInfo);
}

void ServerInstance::stopServer()
{
    m_serverMgr->stopServer();
}

bool ServerInstance::isRunning() const
{
    return m_serverMgr->isRunning();
}

ServerManager::ServerState ServerInstance::serverState() const
{
    return m_serverMgr->state();
}

DiscordManager *ServerInstance::discordManager() const
{
    return m_discordMgr;
}

void ServerInstance::applyGameConfig()
{
    if (m_profile.configFormat == GameProfile::None) {
        return;
    }

    QString formatStr;
    switch (m_profile.configFormat) {
        case GameProfile::INI: formatStr = "ini"; break;
        case GameProfile::Properties: formatStr = "properties"; break;
        case GameProfile::JSON_Config: formatStr = "json"; break;
        default: formatStr = "none"; break;
    }

    QString configPath = installDir() + "/" + m_profile.configFilePath;
    QJsonObject finalMappings;
    QJsonObject currentMerged = mergedSettings();

    for (auto it = m_profile.configMappings.constBegin(); it != m_profile.configMappings.constEnd(); ++it) {
        QString valueTpl = it.value().toString();
        QString expandedValue = GameProfile::expandTemplate(valueTpl, currentMerged);
        finalMappings[it.key()] = expandedValue;
    }

    ServerManager::applyGameConfig(formatStr, configPath, m_profile.configSection, finalMappings);
}
