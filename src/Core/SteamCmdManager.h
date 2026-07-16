#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QProcess;
class QNetworkAccessManager;
class QNetworkReply;

/// Manages SteamCMD installation and game server updates.
class SteamCmdManager : public QObject
{
    Q_OBJECT

public:
    explicit SteamCmdManager(QObject *parent = nullptr);
    ~SteamCmdManager() override;

    /// Set the directory where SteamCMD will be stored.
    void setSteamCmdDir(const QString &dir);
    QString steamCmdDir() const { return m_steamCmdDir; }

    /// Returns true if steamcmd.exe is found in the configured directory.
    bool isSteamCmdInstalled() const;

    /// Download and extract SteamCMD from Valve's CDN.
    void downloadSteamCmd();

    /// Install or update a game server via SteamCMD.
    void installOrUpdateServer(const QString &appId, const QString &installDir);

    /// Validate server file integrity via SteamCMD.
    void validateServer(const QString &appId, const QString &installDir);

    /// Returns true if an operation is currently in progress.
    bool isBusy() const { return m_busy; }

signals:
    void logMessage(const QString &message);
    void operationStarted(const QString &operation);
    void operationFinished(bool success, const QString &message);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
    void runSteamCmd(const QStringList &args);
    void extractZip(const QString &zipPath, const QString &destDir);

    QString m_steamCmdDir;
    QProcess *m_process = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    bool m_busy = false;
    QString m_currentOperation;
};
