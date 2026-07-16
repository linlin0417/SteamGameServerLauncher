#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QStringList>

class QProcess;

/// Controls the game server process and manages its settings.
class ServerManager : public QObject
{
    Q_OBJECT

public:
    enum class ServerState {
        Stopped,
        Starting,
        Running,
        Stopping
    };
    Q_ENUM(ServerState)

    explicit ServerManager(QObject *parent = nullptr);
    ~ServerManager() override;

    /// Set the full path to the game server executable.
    void setServerExecutable(const QString &exePath);
    QString serverExecutable() const { return m_serverExe; }

    void startServer(const QStringList &extraArgs = {});
    void stopServer();

    ServerState state() const { return m_state; }
    bool isRunning() const;

    // --- Settings persistence ---
    static QJsonObject loadSettings(const QString &filePath);
    static bool        saveSettings(const QString &filePath, const QJsonObject &settings);
    static QJsonObject defaultSettings();

    /// Build command-line arguments from a settings JSON object.
    static QStringList buildLaunchArgs(const QJsonObject &settings);

signals:
    void stateChanged(ServerManager::ServerState newState);
    void logMessage(const QString &message);
    void serverCrashed(int exitCode);

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onProcessReadyRead();

private:
    void setState(ServerState s);

    QProcess   *m_process   = nullptr;
    QString     m_serverExe;
    ServerState m_state     = ServerState::Stopped;
};
