#include "SteamCmdManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

static const QString STEAMCMD_URL =
    QStringLiteral("https://steamcdn-a.akamaihd.net/client/installer/steamcmd.zip");

SteamCmdManager::SteamCmdManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
{
}

SteamCmdManager::~SteamCmdManager()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(3000);
    }
}

void SteamCmdManager::setSteamCmdDir(const QString &dir)
{
    m_steamCmdDir = dir;
}

bool SteamCmdManager::isSteamCmdInstalled() const
{
    if (m_steamCmdDir.isEmpty()) return false;
    return QFileInfo::exists(m_steamCmdDir + "/steamcmd.exe");
}

void SteamCmdManager::downloadSteamCmd()
{
    if (m_busy) {
        emit logMessage(tr("[SteamCMD] Another operation is in progress."));
        return;
    }

    m_busy = true;
    m_currentOperation = tr("Downloading SteamCMD");
    emit operationStarted(m_currentOperation);
    emit logMessage(tr("[SteamCMD] Downloading SteamCMD from %1 ...").arg(STEAMCMD_URL));

    QNetworkRequest request(STEAMCMD_URL);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage(tr("[SteamCMD] Download failed: %1").arg(reply->errorString()));
            m_busy = false;
            emit operationFinished(false, reply->errorString());
            return;
        }

        // Ensure destination directory exists
        QDir dir;
        if (!dir.mkpath(m_steamCmdDir)) {
            emit logMessage(tr("[SteamCMD] Failed to create directory: %1").arg(m_steamCmdDir));
            emit logMessage(tr("[SteamCMD] Please check folder permissions or choose a different path in Settings."));
            m_busy = false;
            emit operationFinished(false, tr("Failed to create SteamCMD directory (permission denied?)."));
            return;
        }

        // Save zip file
        const QString zipPath = m_steamCmdDir + "/steamcmd.zip";
        QFile zipFile(zipPath);
        if (!zipFile.open(QIODevice::WriteOnly)) {
            emit logMessage(tr("[SteamCMD] Failed to write: %1").arg(zipPath));
            emit logMessage(tr("[SteamCMD] Reason: %1").arg(zipFile.errorString()));
            emit logMessage(tr("[SteamCMD] Please check folder permissions or choose a different path in Settings."));
            m_busy = false;
            emit operationFinished(false, tr("Failed to write zip file: %1").arg(zipFile.errorString()));
            return;
        }
        zipFile.write(reply->readAll());
        zipFile.close();

        emit logMessage(tr("[SteamCMD] Download complete. Extracting..."));
        extractZip(zipPath, m_steamCmdDir);
    });
}

void SteamCmdManager::extractZip(const QString &zipPath, const QString &destDir)
{
    // Use PowerShell to extract the zip (avoids external library dependency)
    QProcess *ps = new QProcess(this);
    connect(ps, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, ps, zipPath](int exitCode, QProcess::ExitStatus) {
        ps->deleteLater();
        // Remove zip after extraction
        QFile::remove(zipPath);

        if (exitCode == 0 && isSteamCmdInstalled()) {
            emit logMessage(tr("[SteamCMD] Extraction complete. SteamCMD is ready."));
            m_busy = false;
            emit operationFinished(true, tr("SteamCMD installed successfully."));
        } else {
            emit logMessage(tr("[SteamCMD] Extraction failed (exit code %1).").arg(exitCode));
            m_busy = false;
            emit operationFinished(false, tr("Failed to extract SteamCMD."));
        }
    });

    const QString cmd = QStringLiteral(
        "Expand-Archive -Path '%1' -DestinationPath '%2' -Force"
    ).arg(QDir::toNativeSeparators(zipPath),
          QDir::toNativeSeparators(destDir));

    ps->start("powershell", QStringList() << "-NoProfile" << "-Command" << cmd);
}

void SteamCmdManager::installOrUpdateServer(const QString &appId,
                                            const QString &installDir)
{
    if (m_busy) {
        emit logMessage(tr("[SteamCMD] Another operation is in progress."));
        return;
    }

    if (!isSteamCmdInstalled()) {
        emit logMessage(tr("[SteamCMD] SteamCMD not found. Please install it first."));
        emit operationFinished(false, tr("SteamCMD not installed."));
        return;
    }

    // Ensure the server install directory exists
    QDir().mkpath(installDir);

    m_isCheckingUpdate = false;
    m_currentOperation = tr("Installing/Updating server (App %1)").arg(appId);
    emit operationStarted(m_currentOperation);
    emit logMessage(tr("[SteamCMD] Starting update for App %1 -> %2")
                        .arg(appId, QDir::toNativeSeparators(installDir)));

    const QStringList args = {
        "+force_install_dir", QDir::toNativeSeparators(installDir),
        "+login", "anonymous",
        "+app_update", appId, "validate",
        "+quit"
    };
    runSteamCmd(args);
}

void SteamCmdManager::validateServer(const QString &appId,
                                     const QString &installDir)
{
    installOrUpdateServer(appId, installDir);  // validate flag is included
}

void SteamCmdManager::checkServerUpdate(const QString &appId, const QString &acfFilePath)
{
    if (m_busy) {
        emit logMessage(tr("[SteamCMD] Another operation is in progress."));
        return;
    }

    if (!isSteamCmdInstalled()) {
        emit logMessage(tr("[SteamCMD] SteamCMD not found. Please install it first."));
        emit updateCheckFinished(false, QString(), QString(), tr("SteamCMD not installed."));
        return;
    }

    m_localBuildIdCache.clear();
    QFile file(acfFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QRegularExpression rx(QStringLiteral(R"(\"buildid\"\s+\"(\d+)\")"));
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                m_localBuildIdCache = match.captured(1);
                break;
            }
        }
    }

    if (m_localBuildIdCache.isEmpty()) {
        m_localBuildIdCache = tr("Unknown (Not Installed)");
    }

    m_isCheckingUpdate = true;
    m_outputCache.clear();
    m_currentOperation = tr("Checking for server updates (App %1)").arg(appId);
    emit operationStarted(m_currentOperation);
    emit logMessage(tr("[SteamCMD] Checking online version for App %1 ...").arg(appId));

    const QStringList args = {
        QStringLiteral("+login"), QStringLiteral("anonymous"),
        QStringLiteral("+app_info_update"), QStringLiteral("1"),
        QStringLiteral("+app_info_print"), appId,
        QStringLiteral("+quit")
    };
    runSteamCmd(args);
}

void SteamCmdManager::runSteamCmd(const QStringList &args)
{
    m_busy = true;

    if (m_process) {
        m_process->deleteLater();
    }
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_process, &QProcess::readyRead,
            this, &SteamCmdManager::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SteamCmdManager::onProcessFinished);

    const QString exe = m_steamCmdDir + "/steamcmd.exe";
    emit logMessage(tr("[SteamCMD] Running: %1 %2").arg(exe, args.join(' ')));
    m_process->start(exe, args);
}

void SteamCmdManager::onProcessReadyRead()
{
    while (m_process->canReadLine()) {
        const QString line =
            QString::fromUtf8(m_process->readLine()).trimmed();
        if (!line.isEmpty()) {
            if (m_isCheckingUpdate) {
                m_outputCache.append(line + '\n');
            } else {
                emit logMessage(QStringLiteral("[SteamCMD] %1").arg(line));
            }
        }
    }
}

void SteamCmdManager::onProcessFinished(int exitCode,
                                        QProcess::ExitStatus status)
{
    // Read any remaining output
    if (m_process->bytesAvailable() > 0) {
        const QString remaining =
            QString::fromUtf8(m_process->readAll()).trimmed();
        if (!remaining.isEmpty()) {
            for (const QString &line : remaining.split('\n')) {
                if (m_isCheckingUpdate) {
                    m_outputCache.append(line.trimmed() + '\n');
                } else {
                    emit logMessage(QStringLiteral("[SteamCMD] %1").arg(line.trimmed()));
                }
            }
        }
    }

    m_busy = false;

    if (m_isCheckingUpdate) {
        m_isCheckingUpdate = false;
        
        // steamcmd checking updates may return exitCode 0 or 7 (usually 7 if it updated itself first)
        if (status == QProcess::CrashExit) {
            emit logMessage(tr("[SteamCMD] Update check crashed."));
            emit updateCheckFinished(false, m_localBuildIdCache, QString(), tr("SteamCMD crashed during check."));
            return;
        }

        QString onlineBuildId;
        QStringList lines = m_outputCache.split('\n');
        bool inBranches = false;
        bool inPublic = false;
        QRegularExpression rx(QStringLiteral(R"(\"buildid\"\s+\"(\d+)\")"));
        
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed == QStringLiteral("\"branches\"")) {
                inBranches = true;
            } else if (inBranches && trimmed == QStringLiteral("\"public\"")) {
                inPublic = true;
            } else if (inPublic && trimmed.startsWith(QStringLiteral("\"buildid\""))) {
                QRegularExpressionMatch match = rx.match(trimmed);
                if (match.hasMatch()) {
                    onlineBuildId = match.captured(1);
                    break;
                }
            } else if (trimmed == QStringLiteral("}")) {
                if (inPublic) {
                    inPublic = false;
                } else if (inBranches) {
                    inBranches = false;
                }
            }
        }

        if (onlineBuildId.isEmpty()) {
            emit logMessage(tr("[SteamCMD] Could not parse online BuildID."));
            emit updateCheckFinished(false, m_localBuildIdCache, QString(), tr("Failed to parse online info."));
            return;
        }

        bool hasUpdate = false;
        if (m_localBuildIdCache != tr("Unknown (Not Installed)") && m_localBuildIdCache != onlineBuildId) {
            hasUpdate = true;
        } else if (m_localBuildIdCache == tr("Unknown (Not Installed)")) {
            hasUpdate = true; // Not installed means needs update/install
        }

        emit updateCheckFinished(hasUpdate, m_localBuildIdCache, onlineBuildId, tr("Update check completed."));
        return;
    }

    if (status == QProcess::CrashExit) {
        emit logMessage(tr("[SteamCMD] Process crashed."));
        emit operationFinished(false, tr("SteamCMD process crashed."));
    } else if (exitCode != 0) {
        // SteamCMD sometimes returns non-zero for benign reasons.
        // exitCode 7 is common for first-run updates.
        emit logMessage(tr("[SteamCMD] Finished with exit code %1.").arg(exitCode));
        emit operationFinished(exitCode == 7, tr("SteamCMD finished (code %1).").arg(exitCode));
    } else {
        emit logMessage(tr("[SteamCMD] Operation completed successfully."));
        emit operationFinished(true, tr("Operation completed successfully."));
    }
}
