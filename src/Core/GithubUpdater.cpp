#include "GithubUpdater.h"
#include "../version.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QVersionNumber>

GithubUpdater::GithubUpdater(const QString &owner,
                             const QString &repo,
                             const QString &currentVersion,
                             QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_owner(owner)
    , m_repo(repo)
    , m_currentVersion(currentVersion)
{
}

GithubUpdater::~GithubUpdater()
{
    if (m_currentDownload) {
        m_currentDownload->abort();
    }
}

void GithubUpdater::checkForUpdate()
{
    const QString url =
        QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
            .arg(m_owner, m_repo);

    emit logMessage(tr("[Updater] Checking for updates: %1").arg(url));

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("User-Agent", "SteamGameServerLauncher");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit updateError(tr("Failed to check for updates: %1")
                                 .arg(reply->errorString()));
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            emit updateError(tr("Invalid response from GitHub API."));
            return;
        }

        const QJsonObject root = doc.object();
        const QString tagName  = root.value("tag_name").toString();
        const QString body     = root.value("body").toString();

        // Strip leading 'v' from tag if present (e.g. "v1.2.0" -> "1.2.0")
        QString remoteVersion = tagName;
        if (remoteVersion.startsWith('v', Qt::CaseInsensitive))
            remoteVersion = remoteVersion.mid(1);

        emit logMessage(tr("[Updater] Latest release: %1  (local: %2)")
                            .arg(remoteVersion, m_currentVersion));

        if (isNewerVersion(remoteVersion, m_currentVersion)) {
            // Find a suitable download asset (look for .zip containing the platform)
            QString downloadUrl;
            const QJsonArray assets = root.value("assets").toArray();
            for (const QJsonValue &av : assets) {
                const QJsonObject asset = av.toObject();
                const QString name = asset.value("name").toString();
                if (name.endsWith(".zip", Qt::CaseInsensitive)) {
                    downloadUrl =
                        asset.value("browser_download_url").toString();
                    break;
                }
            }
            if (downloadUrl.isEmpty() && !assets.isEmpty()) {
                // Fallback: use the first asset
                downloadUrl = assets.first().toObject()
                                  .value("browser_download_url").toString();
            }
            emit updateAvailable(remoteVersion, downloadUrl, body);
        } else {
            emit noUpdateAvailable();
        }
    });
}

void GithubUpdater::downloadUpdate(const QString &downloadUrl)
{
    if (downloadUrl.isEmpty()) {
        emit updateError(tr("No download URL provided."));
        return;
    }

    emit logMessage(tr("[Updater] Downloading update from: %1").arg(downloadUrl));

    QNetworkRequest request(downloadUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "SteamGameServerLauncher");

    m_currentDownload = m_networkManager->get(request);

    connect(m_currentDownload, &QNetworkReply::downloadProgress,
            this, &GithubUpdater::downloadProgress);

    connect(m_currentDownload, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_currentDownload;
        m_currentDownload = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit updateError(tr("Download failed: %1").arg(reply->errorString()));
            return;
        }

        // Save to a temp file to avoid permission issues in Program Files
        const QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        const QString savePath =
            QDir(tempDir).filePath("SteamGameServerLauncher_update_package.zip");
        
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            emit updateError(tr("Cannot write to: %1").arg(savePath));
            return;
        }
        file.write(reply->readAll());
        file.close();

        emit logMessage(tr("[Updater] Download saved to: %1").arg(savePath));
        emit downloadFinished(savePath);
    });
}

void GithubUpdater::applyUpdate(const QString &zipPath)
{
    // Launch the external Updater.exe.
    // Args: --zip <path> --target <appDir> --exe <launcherExe>
    const QString appDir     = QCoreApplication::applicationDirPath();
    const QString updaterExe = appDir + "/" + AppConfig::UpdaterExeName;
    const QString launcherExe = appDir + "/" + AppConfig::LauncherExeName;

    if (!QFileInfo::exists(updaterExe)) {
        emit updateError(tr("Updater executable not found: %1").arg(updaterExe));
        return;
    }

    emit logMessage(tr("[Updater] Launching updater and closing launcher..."));

    QStringList args;
    args << "--zip" << QDir::toNativeSeparators(zipPath)
         << "--target" << QDir::toNativeSeparators(appDir)
         << "--exe" << QDir::toNativeSeparators(launcherExe)
         << "--pid" << QString::number(QCoreApplication::applicationPid());

    QProcess::startDetached(updaterExe, args);

    // Quit the application so the updater can replace files.
    QCoreApplication::quit();
}

bool GithubUpdater::isNewerVersion(const QString &remoteTag,
                                    const QString &localVer) const
{
    const QVersionNumber remote = QVersionNumber::fromString(remoteTag);
    const QVersionNumber local  = QVersionNumber::fromString(localVer);
    return remote > local;
}
