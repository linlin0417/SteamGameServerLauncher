#pragma once

#include <QMainWindow>

class QTabWidget;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;
class QProgressBar;

class SteamCmdManager;
class ServerManager;
class GithubUpdater;

/// Main application window with tabbed interface.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    // --- Tab creation helpers ---
    QWidget *createControlTab();
    QWidget *createSettingsTab();
    QWidget *createAboutTab();

    // --- Helpers ---
    void appendLog(const QString &message);
    void updateServerStateUI();
    void loadSettingsToUI();
    void saveSettingsFromUI();
    QString settingsFilePath() const;
    QString serverInstallDir() const;
    void applyDarkTheme();

    // --- Actions ---
    void onInstallSteamCmd();
    void onUpdateServer();
    void onStartServer();
    void onStopServer();
    void onCheckForUpdate();
    void onBrowseServerExe();

    // --- Core modules ---
    SteamCmdManager *m_steamCmd   = nullptr;
    ServerManager   *m_serverMgr  = nullptr;
    GithubUpdater   *m_updater   = nullptr;

    // --- UI: Control tab ---
    QLabel       *m_statusLabel     = nullptr;
    QPushButton  *m_btnInstallCmd   = nullptr;
    QPushButton  *m_btnUpdateServer = nullptr;
    QPushButton  *m_btnStart        = nullptr;
    QPushButton  *m_btnStop         = nullptr;
    QTextEdit    *m_logOutput       = nullptr;
    QProgressBar *m_progressBar     = nullptr;

    // --- UI: Settings tab ---
    QLineEdit *m_editServerName     = nullptr;
    QLineEdit *m_editPassword       = nullptr;
    QLineEdit *m_editAdminPassword  = nullptr;
    QSpinBox  *m_spinMaxPlayers     = nullptr;
    QSpinBox  *m_spinPort           = nullptr;
    QSpinBox  *m_spinQueryPort      = nullptr;
    QLineEdit *m_editServerExePath  = nullptr;
    QLineEdit *m_editAdditionalArgs = nullptr;

    // --- UI: About tab ---
    QLabel *m_versionLabel        = nullptr;
    QLabel *m_updateStatusLabel   = nullptr;
    QPushButton *m_btnCheckUpdate = nullptr;
    QPushButton *m_btnDownloadUpdate = nullptr;
    QProgressBar *m_updateProgress = nullptr;

    QString m_pendingDownloadUrl;
    QString m_pendingZipPath;
};
