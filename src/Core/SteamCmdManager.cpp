#include "SteamCmdManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
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
            QString::fromLocal8Bit(m_process->readLine()).trimmed();
        if (!line.isEmpty()) {
            emit logMessage(QStringLiteral("[SteamCMD] %1").arg(line));
        }
    }
}

void SteamCmdManager::onProcessFinished(int exitCode,
                                        QProcess::ExitStatus status)
{
    // Read any remaining output
    if (m_process->bytesAvailable() > 0) {
        const QString remaining =
            QString::fromLocal8Bit(m_process->readAll()).trimmed();
        if (!remaining.isEmpty()) {
            for (const QString &line : remaining.split('\n')) {
                emit logMessage(QStringLiteral("[SteamCMD] %1").arg(line.trimmed()));
            }
        }
    }

    m_busy = false;

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
