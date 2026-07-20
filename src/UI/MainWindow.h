#pragma once

#include <QMainWindow>

class QTabWidget;
class QTextEdit;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;
class QProgressBar;
class QComboBox;

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
    QWidget *createMapTab();
    QWidget *createAboutTab();

    // --- Helpers ---
    void appendLog(const QString &message);
    void updateServerStateUI();
    void loadSettingsToUI();
    void saveSettingsFromUI();
    QString dataRootDir() const;
    QString settingsFilePath() const;
    QString serverInstallDir() const;
    void applyDarkTheme();
    void autoDetectServerExe();

    // --- Actions ---
    void onInstallSteamCmd();
    void onUpdateServer();
    void onCheckServerUpdate();
    void onStartServer();
    void onStopServer();
    void onCheckForUpdate();
    void onBrowseSteamCmdPath();
    void onBrowseServerBasePath();
    void onBrowseServerExe();
    void onExportMap();
    void onImportMap();
    void refreshProspectList();
    QString prospectsDir() const;

    // --- Core modules ---
    SteamCmdManager *m_steamCmd   = nullptr;
    ServerManager   *m_serverMgr  = nullptr;
    GithubUpdater   *m_updater   = nullptr;

    // --- UI: Control tab ---
    QLabel       *m_statusLabel     = nullptr;
    QPushButton  *m_btnInstallCmd   = nullptr;
    QPushButton  *m_btnUpdateServer = nullptr;
    QPushButton  *m_btnCheckServerUpdate = nullptr;
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
    QLineEdit *m_editSteamCmdPath   = nullptr;
    QLineEdit *m_editServerBasePath = nullptr;
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

    // --- UI: Map Management tab ---
    QComboBox  *m_comboProspects      = nullptr;
    QLineEdit  *m_editMapPackageName  = nullptr;
    QTextEdit  *m_editMapNotes        = nullptr;
    QLineEdit  *m_editPreviewImagePath = nullptr;
    QPushButton *m_btnRefreshProspects = nullptr;
    QPushButton *m_btnExportMap        = nullptr;
    QPushButton *m_btnImportMap        = nullptr;
    QLabel     *m_previewImage         = nullptr;
    QLabel     *m_mapInfoLabel         = nullptr;
};
