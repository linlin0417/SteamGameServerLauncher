#include "GameProfileManager.h"
#include "GameProfile.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

GameProfileManager::GameProfileManager(const QString &userProfilesDir, QObject *parent)
    : QObject(parent), m_userProfilesDir(userProfilesDir)
{
    reload();
}

void GameProfileManager::reload()
{
    m_profiles.clear();
    loadBuiltinProfiles();
    loadUserProfiles();
    emit profilesChanged();
}

QList<GameProfile> GameProfileManager::allProfiles() const
{
    return m_profiles;
}

GameProfile GameProfileManager::profileById(const QString &id) const
{
    for (const GameProfile &profile : m_profiles) {
        if (profile.id == id) {
            return profile;
        }
    }
    return GameProfile();
}

bool GameProfileManager::saveProfile(const GameProfile &profile)
{
    QDir dir;
    if (!dir.mkpath(m_userProfilesDir)) {
        qWarning() << "Failed to create directory:" << m_userProfilesDir;
        return false;
    }

    QString filePath = QDir(m_userProfilesDir).filePath(profile.id + ".json");
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }

    QJsonDocument doc(profile.toJson());
    file.write(doc.toJson());
    file.close();

    bool found = false;
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == profile.id) {
            m_profiles[i] = profile;
            found = true;
            break;
        }
    }

    if (!found) {
        m_profiles.append(profile);
    }

    emit profilesChanged();
    return true;
}

bool GameProfileManager::removeProfile(const QString &id)
{
    if (GameProfile::builtinProfileIds().contains(id)) {
        qWarning() << "Cannot remove builtin profile:" << id;
        return false;
    }

    QString filePath = QDir(m_userProfilesDir).filePath(id + ".json");
    QFile file(filePath);
    if (file.exists()) {
        if (!file.remove()) {
            qWarning() << "Failed to remove profile file:" << filePath;
            return false;
        }
    }

    bool removed = false;
    for (int i = 0; i < m_profiles.size(); ++i) {
        if (m_profiles[i].id == id) {
            m_profiles.removeAt(i);
            removed = true;
            break;
        }
    }

    if (removed) {
        emit profilesChanged();
    }
    
    return removed;
}

bool GameProfileManager::hasProfile(const QString &id) const
{
    for (const GameProfile &profile : m_profiles) {
        if (profile.id == id) {
            return true;
        }
    }
    return false;
}

QString GameProfileManager::generateUniqueId() const
{
    QString baseId = "custom_" + QString::number(QDateTime::currentMSecsSinceEpoch());
    QString id = baseId;
    int counter = 1;

    while (hasProfile(id)) {
        id = baseId + '_' + QString::number(counter++);
    }

    return id;
}

void GameProfileManager::loadBuiltinProfiles()
{
    QDir dir(":/profiles/");
    if (!dir.exists()) {
        return;
    }

    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : files) {
        QFile file(dir.filePath(fileName));
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                GameProfile profile = GameProfile::fromJson(doc.object());
                if (!profile.id.isEmpty()) {
                    m_profiles.append(profile);
                }
            }
            file.close();
        } else {
            qWarning() << "Failed to open builtin profile:" << fileName;
        }
    }
}

void GameProfileManager::loadUserProfiles()
{
    QDir dir(m_userProfilesDir);
    if (!dir.exists()) {
        return;
    }

    QStringList files = dir.entryList(QStringList() << "*.json", QDir::Files);
    for (const QString &fileName : files) {
        QFile file(dir.filePath(fileName));
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                GameProfile profile = GameProfile::fromJson(doc.object());
                if (!profile.id.isEmpty()) {
                    bool replaced = false;
                    for (int i = 0; i < m_profiles.size(); ++i) {
                        if (m_profiles[i].id == profile.id) {
                            m_profiles[i] = profile;
                            replaced = true;
                            break;
                        }
                    }
                    if (!replaced) {
                        m_profiles.append(profile);
                    }
                }
            }
            file.close();
        } else {
            qWarning() << "Failed to open user profile:" << fileName;
        }
    }
}
