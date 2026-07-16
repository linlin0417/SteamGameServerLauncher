#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

/// Checks for new releases on a GitHub repository and manages downloads.
class GithubUpdater : public QObject
{
    Q_OBJECT

public:
    explicit GithubUpdater(const QString &owner,
                           const QString &repo,
                           const QString &currentVersion,
                           QObject *parent = nullptr);
    ~GithubUpdater() override;

    /// Query the GitHub Releases API for the latest version.
    void checkForUpdate();

    /// Download a release asset from the given URL.
    void downloadUpdate(const QString &downloadUrl);

    /// Launch the external Updater.exe and quit the application.
    void applyUpdate(const QString &zipPath);

signals:
    void updateAvailable(const QString &newVersion,
                         const QString &downloadUrl,
                         const QString &releaseNotes);
    void noUpdateAvailable();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString &filePath);
    void updateError(const QString &errorMessage);
    void logMessage(const QString &message);

private:
    bool isNewerVersion(const QString &remoteTag, const QString &localVer) const;

    QNetworkAccessManager *m_networkManager;
    QString m_owner;
    QString m_repo;
    QString m_currentVersion;
    QNetworkReply *m_currentDownload = nullptr;
};
