#pragma once

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <QProcess>

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

    /// 根據遊戲設定檔的 configFormat 和 configMappings 寫入遊戲設定檔
    /// @param configFormat 設定檔格式 ("ini", "properties", "json", "none")
    /// @param configFilePath 設定檔完整路徑
    /// @param configSection INI 格式的目標區段名（僅 INI 格式使用）
    /// @param mappings key-value 對映（key 為設定檔中的鍵名，value 為已替換變數的值）
    /// @param configDefaultContent 設定檔預設內容（逐行），檔案不存在時以此建立
    static bool applyGameConfig(const QString &configFormat,
                               const QString &configFilePath,
                               const QString &configSection,
                               const QJsonObject &mappings,
                               const QStringList &configDefaultContent = {});

    /// 根據遊戲設定檔的反向映射讀取設定檔數值
    /// @param configFormat 設定檔格式 ("ini", "properties", "json", "none")
    /// @param configFilePath 設定檔完整路徑
    /// @param configSection INI 格式的目標區段名（僅 INI 格式使用）
    /// @param mappings key-value 對映（key 為設定檔中的鍵名，value 為目標變數名，如 "{maxPlayers}"）
    /// @return 讀取到的變數對映 (如 "maxPlayers" -> "8")
    static QJsonObject readGameConfig(const QString &configFormat,
                                      const QString &configFilePath,
                                      const QString &configSection,
                                      const QJsonObject &mappings);

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
