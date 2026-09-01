#include "GameProfileManager.h"
#include "GameProfile.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QThreadPool>
#include <QRunnable>

GameProfileManager::GameProfileManager(const QString &userProfilesDir, QObject *parent)
    : QObject(parent), m_userProfilesDir(userProfilesDir)
{
    reload();
}

void GameProfileManager::reload()
{
    m_profileIds.clear();
    m_profileMap.clear();
    loadBuiltinProfiles();
    loadUserProfiles();
    emit profilesChanged();
}

QList<GameProfile> GameProfileManager::allProfiles() const
{
    QList<GameProfile> list;
    for (const QString &id : m_profileIds) {
        list.append(m_profileMap.value(id));
    }
    return list;
}

GameProfile GameProfileManager::profileById(const QString &id) const
{
    return m_profileMap.value(id, GameProfile());
}

bool GameProfileManager::saveProfile(const GameProfile &profile)
{
    QDir dir;
    if (!dir.mkpath(m_userProfilesDir)) {
        qWarning() << "Failed to create directory:" << m_userProfilesDir;
        return false;
    }

    QString filePath = QDir(m_userProfilesDir).filePath(profile.id + ".json");

    if (!m_profileMap.contains(profile.id)) {
        m_profileIds.append(profile.id);
    }
    m_profileMap.insert(profile.id, profile);
    emit profilesChanged();

    class ProfileSaveTask : public QRunnable {
    public:
        ProfileSaveTask(const QString &filePath, const GameProfile &profile) 
            : m_filePath(filePath), m_profile(profile) {}
        
        void run() override {
            QFile file(m_filePath);
            if (!file.open(QIODevice::WriteOnly)) {
                qWarning() << "Failed to open file for writing:" << m_filePath;
                return;
            }
            QJsonDocument doc(m_profile.toJson());
            file.write(doc.toJson());
            file.close();
        }
    private:
        QString m_filePath;
        GameProfile m_profile;
    };

    QThreadPool::globalInstance()->start(new ProfileSaveTask(filePath, profile));

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

    if (m_profileMap.remove(id) > 0) {
        m_profileIds.removeAll(id);
        emit profilesChanged();
        return true;
    }
    
    return false;
}

bool GameProfileManager::hasProfile(const QString &id) const
{
    return m_profileMap.contains(id);
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
                    if (!m_profileMap.contains(profile.id)) {
                        m_profileIds.append(profile.id);
                    }
                    m_profileMap.insert(profile.id, profile);
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
                    if (!m_profileMap.contains(profile.id)) {
                        m_profileIds.append(profile.id);
                    }
                    m_profileMap.insert(profile.id, profile);
                }
            }
            file.close();
        } else {
            qWarning() << "Failed to open user profile:" << fileName;
        }
    }
}
