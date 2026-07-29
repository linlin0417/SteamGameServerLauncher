#include "SettingsMigrator.h"
#include "ServerManager.h"

#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

bool SettingsMigrator::needsMigration(const QString &dataRootDir)
{
    QString oldSettingsPath = dataRootDir + "/launcher_settings.json";
    if (!QFile::exists(oldSettingsPath)) {
        return false;
    }

    QJsonObject oldSettings = ServerManager::loadSettings(oldSettingsPath);
    if (oldSettings.isEmpty()) {
        return false;
    }

    if (oldSettings.contains("_migrated_to_v2") && oldSettings.value("_migrated_to_v2").toBool()) {
        return false;
    }

    QDir instancesDir(dataRootDir + "/instances");
    if (instancesDir.exists()) {
        QStringList instanceFiles = instancesDir.entryList(QStringList() << "*.json", QDir::Files);
        if (!instanceFiles.isEmpty()) {
            return false;
        }
    }

    return true;
}

bool SettingsMigrator::migrate(const QString &dataRootDir)
{
    QString oldSettingsPath = dataRootDir + "/launcher_settings.json";
    QJsonObject oldSettings = ServerManager::loadSettings(oldSettingsPath);
    if (oldSettings.isEmpty()) {
        qWarning() << "無法讀取舊版設定檔，遷移失敗。";
        return false;
    }

    QDir dir(dataRootDir);
    if (!dir.mkpath("instances")) {
        qWarning() << "無法建立 instances 目錄。";
        return false;
    }

    QJsonObject newInstance;
    newInstance["profileId"] = "icarus";
    newInstance["serverName"] = oldSettings.value("serverName").toString();
    newInstance["password"] = oldSettings.value("password").toString();
    newInstance["adminPassword"] = oldSettings.value("adminPassword").toString();
    newInstance["maxPlayers"] = oldSettings.value("maxPlayers").toInt(8);
    newInstance["port"] = oldSettings.value("port").toInt(17777);
    newInstance["queryPort"] = oldSettings.value("queryPort").toInt(27015);
    newInstance["steamCmdPath"] = oldSettings.value("steamCmdPath").toString();
    newInstance["installDir"] = oldSettings.value("serverBasePath").toString();
    newInstance["serverExePath"] = oldSettings.value("serverExePath").toString();
    newInstance["additionalArgs"] = oldSettings.value("additionalArgs").toString();
    newInstance["discordWebhookUrl"] = oldSettings.value("discordWebhookUrl").toString();

    QString newInstancePath = dataRootDir + "/instances/icarus.json";
    if (!ServerManager::saveSettings(newInstancePath, newInstance)) {
        qWarning() << "無法儲存新的實例設定檔。";
        return false;
    }

    QJsonObject newGlobalSettings;
    newGlobalSettings["lastSelectedProfile"] = "icarus";
    newGlobalSettings["_migrated_to_v2"] = true;
    
    QString backupPath = dataRootDir + "/launcher_settings.json.v1.bak";
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    
    if (!QFile::rename(oldSettingsPath, backupPath)) {
        qWarning() << "無法備份舊版設定檔。";
        return false;
    }

    if (!ServerManager::saveSettings(oldSettingsPath, newGlobalSettings)) {
        qWarning() << "無法儲存 v2.0.0 全域設定檔。";
        return false;
    }

    qInfo() << "設定檔成功遷移至 v2.0.0 格式。";
    return true;
}
