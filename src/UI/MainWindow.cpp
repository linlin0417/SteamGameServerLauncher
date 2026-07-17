#include "MainWindow.h"
#include "../version.h"
#include "../Core/SteamCmdManager.h"
#include "../Core/ServerManager.h"
#include "../Core/GithubUpdater.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

// ═══════════════════════════════════════════════════════════════════
//  Construction / Destruction
// ═══════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1  v%2").arg(AppConfig::AppName, APP_VERSION));
    setMinimumSize(800, 600);
    resize(960, 700);

    applyDarkTheme();

    // --- Core modules ---
    m_steamCmd  = new SteamCmdManager(this);
    m_serverMgr = new ServerManager(this);
    m_updater   = new GithubUpdater(
        AppConfig::GithubOwner, AppConfig::GithubRepo, APP_VERSION, this);

    // Load settings first to get the configured paths
    const QJsonObject settings = ServerManager::loadSettings(settingsFilePath());
    QString steamPath = settings.value("steamCmdPath").toString();
    if (steamPath.isEmpty()) {
        steamPath = QCoreApplication::applicationDirPath() + "/" + AppConfig::SteamCmdSubDir;
    }
    m_steamCmd->setSteamCmdDir(steamPath);

    // --- Central widget with tabs ---
    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(createControlTab(),  tr("  伺服器控制  "));
    tabs->addTab(createSettingsTab(), tr("  伺服器設定  "));
    tabs->addTab(createAboutTab(),    tr("  關於 / 更新  "));
    setCentralWidget(tabs);

    // --- Connect core signals to the log ---
    connect(m_steamCmd,  &SteamCmdManager::logMessage,
            this, &MainWindow::appendLog);
    connect(m_serverMgr, &ServerManager::logMessage,
            this, &MainWindow::appendLog);
    connect(m_updater,   &GithubUpdater::logMessage,
            this, &MainWindow::appendLog);

    // SteamCMD operation finished
    connect(m_steamCmd, &SteamCmdManager::operationFinished,
            this, [this](bool success, const QString &msg) {
        m_btnInstallCmd->setEnabled(true);
        m_btnUpdateServer->setEnabled(true);
        if (success)
            appendLog(tr("✔ %1").arg(msg));
        else
            appendLog(tr("✘ %1").arg(msg));
    });

    connect(m_steamCmd, &SteamCmdManager::operationStarted,
            this, [this](const QString &op) {
        m_btnInstallCmd->setEnabled(false);
        m_btnUpdateServer->setEnabled(false);
        appendLog(tr("▶ %1").arg(op));
    });

    // Server state changes
    connect(m_serverMgr, &ServerManager::stateChanged,
            this, [this](ServerManager::ServerState) {
        updateServerStateUI();
    });

    connect(m_serverMgr, &ServerManager::serverCrashed,
            this, [this](int code) {
        appendLog(tr("⚠ Server crashed with exit code %1").arg(code));
    });

    // GitHub updater signals
    connect(m_updater, &GithubUpdater::updateAvailable,
            this, [this](const QString &ver, const QString &url, const QString &notes) {
        m_updateStatusLabel->setText(
            tr("<span style='color:#66c0f4;'>New version available: <b>%1</b></span>").arg(ver));
        m_pendingDownloadUrl = url;
        m_btnDownloadUpdate->setEnabled(!url.isEmpty());
        m_btnCheckUpdate->setEnabled(true);
        appendLog(tr("[Updater] New version %1 available!").arg(ver));
        if (!notes.isEmpty())
            appendLog(tr("[Updater] Release notes: %1").arg(notes));
    });

    connect(m_updater, &GithubUpdater::noUpdateAvailable,
            this, [this]() {
        m_updateStatusLabel->setText(
            tr("<span style='color:#c7d5e0;'>You are up to date.</span>"));
        m_btnCheckUpdate->setEnabled(true);
    });

    connect(m_updater, &GithubUpdater::downloadProgress,
            this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_updateProgress->setMaximum(static_cast<int>(total));
            m_updateProgress->setValue(static_cast<int>(received));
        }
    });

    connect(m_updater, &GithubUpdater::downloadFinished,
            this, [this](const QString &path) {
        m_pendingZipPath = path;
        appendLog(tr("[Updater] Download complete: %1").arg(path));
        m_updateStatusLabel->setText(
            tr("<span style='color:#66c0f4;'>Download complete. Ready to apply.</span>"));
        m_btnDownloadUpdate->setText(tr("Apply Update && Restart"));
        m_btnDownloadUpdate->setEnabled(true);
    });

    connect(m_updater, &GithubUpdater::updateError,
            this, [this](const QString &err) {
        m_updateStatusLabel->setText(
            tr("<span style='color:#eb4b4b;'>Error: %1</span>").arg(err));
        m_btnCheckUpdate->setEnabled(true);
        m_btnDownloadUpdate->setEnabled(false);
    });

    // Load settings
    loadSettingsToUI();
    updateServerStateUI();
}

MainWindow::~MainWindow() = default;

// ═══════════════════════════════════════════════════════════════════
//  Dark Theme
// ═══════════════════════════════════════════════════════════════════

void MainWindow::applyDarkTheme()
{
    const QString style = QStringLiteral(R"(
        /* === Global === */
        QMainWindow, QWidget {
            background-color: #171a21; /* Steam main background */
            color: #c7d5e0; /* Steam text color */
            font-family: "Segoe UI", "Microsoft JhengHei", sans-serif;
            font-size: 10pt;
        }

        /* === Tab Widget === */
        QTabWidget::pane {
            border: 1px solid #1b2838;
            background-color: #171a21;
            border-radius: 4px;
        }
        QTabBar::tab {
            background-color: transparent;
            color: #8f98a0;
            padding: 12px 24px;
            margin-right: 2px;
            border-bottom: 3px solid transparent;
            font-weight: bold;
            font-size: 11pt;
        }
        QTabBar::tab:selected {
            color: #ffffff;
            border-bottom: 3px solid #66c0f4; /* Steam Blue */
        }
        QTabBar::tab:hover:!selected {
            color: #c7d5e0;
        }

        /* === Buttons === */
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #475a6f, stop:1 #2d3b4b);
            color: #c7d5e0;
            border: 1px solid #1b2838;
            padding: 10px 20px;
            border-radius: 3px;
            font-weight: bold;
            min-width: 100px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5c748c, stop:1 #3e4f63);
            color: #ffffff;
        }
        QPushButton:pressed {
            background: #2a3746;
        }
        QPushButton:disabled {
            background: #2a2f35;
            color: #555555;
            border: none;
        }

        /* === Log / TextEdit === */
        QTextEdit {
            background-color: #101214; /* Darker console background */
            color: #a0b1c0;
            border: 1px solid #202d39;
            border-radius: 4px;
            font-family: "Cascadia Code", "Consolas", monospace;
            font-size: 9pt;
            padding: 6px;
            selection-background-color: #264f78;
        }

        /* === Inputs === */
        QLineEdit, QSpinBox {
            background-color: #101214;
            color: #c7d5e0;
            border: 1px solid #202d39;
            padding: 8px;
            border-radius: 3px;
            font-size: 10pt;
        }
        QLineEdit:focus, QSpinBox:focus {
            border: 1px solid #66c0f4;
        }

        /* === Labels === */
        QLabel {
            color: #c7d5e0;
        }

        /* === GroupBox === */
        QGroupBox {
            border: 1px solid #202d39;
            border-radius: 4px;
            margin-top: 14px;
            padding-top: 18px;
            font-weight: bold;
            color: #66c0f4;
            background-color: #1b2838; /* Slightly lighter inner panel */
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 14px;
            padding: 0 6px;
            background-color: transparent;
        }

        /* === Progress Bar === */
        QProgressBar {
            border: 1px solid #202d39;
            border-radius: 3px;
            text-align: center;
            background-color: #101214;
            color: #ffffff;
            height: 20px;
        }
        QProgressBar::chunk {
            background: qlineargradient(
                x1:0, y1:0, x2:1, y2:0,
                stop:0 #66c0f4, stop:1 #2a475e
            );
            border-radius: 2px;
        }

        /* === ScrollBar === */
        QScrollBar:vertical {
            background: #101214;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background: #2a3746;
            min-height: 30px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical:hover {
            background: #3e4f63;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
    qApp->setStyleSheet(style);
}

// ═══════════════════════════════════════════════════════════════════
//  Tab: Server Control
// ═══════════════════════════════════════════════════════════════════

QWidget *MainWindow::createControlTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);

    // Status bar
    QHBoxLayout *statusRow = new QHBoxLayout;
    m_statusLabel = new QLabel(tr("● 已停止"));
    m_statusLabel->setStyleSheet(
        "font-size: 14pt; font-weight: bold; color: #888;");
    statusRow->addWidget(m_statusLabel);
    statusRow->addStretch();
    layout->addLayout(statusRow);

    // Button row
    QHBoxLayout *btnRow = new QHBoxLayout;
    m_btnInstallCmd = new QPushButton(tr("安裝 SteamCMD"));
    m_btnUpdateServer = new QPushButton(tr("更新伺服器"));
    m_btnStart = new QPushButton(tr("▶  啟動伺服器"));
    m_btnStop = new QPushButton(tr("■  停止伺服器"));

    m_btnStart->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #79a01b, stop:1 #5c7e10); color: #d2efa9; border: 1px solid #455e09; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8ab41f, stop:1 #6b9313); color: #ffffff; }"
        "QPushButton:disabled { background: #2a2f35; color: #555; border: none; }");
    m_btnStop->setStyleSheet(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c2a2a, stop:1 #2e1c1c); color: #e8a7a7; border: 1px solid #281515; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4f3636, stop:1 #3e2626); color: #ffffff; }"
        "QPushButton:disabled { background: #2a2f35; color: #555; border: none; }");

    btnRow->addWidget(m_btnInstallCmd);
    btnRow->addWidget(m_btnUpdateServer);
    btnRow->addStretch();
    btnRow->addWidget(m_btnStart);
    btnRow->addWidget(m_btnStop);
    layout->addLayout(btnRow);

    // Progress bar (hidden by default — for future SteamCMD progress)
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    // Log area
    m_logOutput = new QTextEdit;
    m_logOutput->setReadOnly(true);
    m_logOutput->setPlaceholderText(tr("Console output will appear here..."));
    layout->addWidget(m_logOutput, 1); // stretch factor = 1

    // Connections
    connect(m_btnInstallCmd,   &QPushButton::clicked, this, &MainWindow::onInstallSteamCmd);
    connect(m_btnUpdateServer, &QPushButton::clicked, this, &MainWindow::onUpdateServer);
    connect(m_btnStart,        &QPushButton::clicked, this, &MainWindow::onStartServer);
    connect(m_btnStop,         &QPushButton::clicked, this, &MainWindow::onStopServer);

    return page;
}

// ═══════════════════════════════════════════════════════════════════
//  Tab: Server Settings
// ═══════════════════════════════════════════════════════════════════

QWidget *MainWindow::createSettingsTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *outer = new QVBoxLayout(page);
    outer->setSpacing(16);
    outer->setContentsMargins(16, 16, 16, 16);

    // --- Server Parameters group ---
    QGroupBox *grpParams = new QGroupBox(tr("伺服器參數"));
    QFormLayout *form = new QFormLayout(grpParams);
    form->setSpacing(10);
    form->setContentsMargins(16, 24, 16, 16);

    m_editServerName = new QLineEdit;
    m_editServerName->setPlaceholderText(tr("My Icarus Server"));
    form->addRow(tr("伺服器名稱:"), m_editServerName);

    m_editPassword = new QLineEdit;
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setPlaceholderText(tr("留空 = 不設密碼"));
    form->addRow(tr("密碼:"), m_editPassword);

    m_editAdminPassword = new QLineEdit;
    m_editAdminPassword->setEchoMode(QLineEdit::Password);
    form->addRow(tr("管理員密碼:"), m_editAdminPassword);

    m_spinMaxPlayers = new QSpinBox;
    m_spinMaxPlayers->setRange(1, 64);
    m_spinMaxPlayers->setValue(AppConfig::DefaultMaxPlayers);
    form->addRow(tr("最大玩家數:"), m_spinMaxPlayers);

    m_spinPort = new QSpinBox;
    m_spinPort->setRange(1, 65535);
    m_spinPort->setValue(AppConfig::DefaultPort);
    form->addRow(tr("遊戲埠 (Port):"), m_spinPort);

    m_spinQueryPort = new QSpinBox;
    m_spinQueryPort->setRange(1, 65535);
    m_spinQueryPort->setValue(AppConfig::DefaultQueryPort);
    form->addRow(tr("查詢埠 (Query Port):"), m_spinQueryPort);

    outer->addWidget(grpParams);

    // --- Paths group ---
    QGroupBox *grpPaths = new QGroupBox(tr("路徑設定"));
    QFormLayout *pathForm = new QFormLayout(grpPaths);
    pathForm->setSpacing(10);
    pathForm->setContentsMargins(16, 24, 16, 16);

    QHBoxLayout *steamRow = new QHBoxLayout;
    m_editSteamCmdPath = new QLineEdit;
    m_editSteamCmdPath->setPlaceholderText(tr("SteamCMD 根目錄..."));
    QPushButton *btnBrowseSteam = new QPushButton(tr("瀏覽..."));
    btnBrowseSteam->setMinimumWidth(60);
    connect(btnBrowseSteam, &QPushButton::clicked, this, &MainWindow::onBrowseSteamCmdPath);
    steamRow->addWidget(m_editSteamCmdPath, 1);
    steamRow->addWidget(btnBrowseSteam);
    pathForm->addRow(tr("SteamCMD 路徑:"), steamRow);

    QHBoxLayout *baseRow = new QHBoxLayout;
    m_editServerBasePath = new QLineEdit;
    m_editServerBasePath->setPlaceholderText(tr("伺服器安裝母目錄 (例如: /data)"));
    QPushButton *btnBrowseBase = new QPushButton(tr("瀏覽..."));
    btnBrowseBase->setMinimumWidth(60);
    connect(btnBrowseBase, &QPushButton::clicked, this, &MainWindow::onBrowseServerBasePath);
    baseRow->addWidget(m_editServerBasePath, 1);
    baseRow->addWidget(btnBrowseBase);
    pathForm->addRow(tr("安裝母目錄:"), baseRow);

    QHBoxLayout *exeRow = new QHBoxLayout;
    m_editServerExePath = new QLineEdit;
    m_editServerExePath->setPlaceholderText(tr("伺服器執行檔路徑..."));
    QPushButton *btnBrowse = new QPushButton(tr("瀏覽..."));
    btnBrowse->setMinimumWidth(60);
    connect(btnBrowse, &QPushButton::clicked, this, &MainWindow::onBrowseServerExe);
    exeRow->addWidget(m_editServerExePath, 1);
    exeRow->addWidget(btnBrowse);
    pathForm->addRow(tr("伺服器執行檔:"), exeRow);

    m_editAdditionalArgs = new QLineEdit;
    m_editAdditionalArgs->setPlaceholderText(tr("額外的啟動參數 (可選)"));
    pathForm->addRow(tr("額外參數:"), m_editAdditionalArgs);

    outer->addWidget(grpPaths);

    // Buttons
    QHBoxLayout *btnRow = new QHBoxLayout;
    QPushButton *btnSave = new QPushButton(tr("💾  儲存設定"));
    QPushButton *btnReset = new QPushButton(tr("↺  重置為預設"));
    btnRow->addStretch();
    btnRow->addWidget(btnSave);
    btnRow->addWidget(btnReset);
    outer->addLayout(btnRow);

    outer->addStretch();

    connect(btnSave, &QPushButton::clicked, this, &MainWindow::saveSettingsFromUI);
    connect(btnReset, &QPushButton::clicked, this, [this]() {
        QJsonObject defaults = ServerManager::defaultSettings();
        // Temporarily save defaults, then reload
        ServerManager::saveSettings(settingsFilePath(), defaults);
        loadSettingsToUI();
        appendLog(tr("Settings reset to defaults."));
    });

    return page;
}

// ═══════════════════════════════════════════════════════════════════
//  Tab: About / Update
// ═══════════════════════════════════════════════════════════════════

QWidget *MainWindow::createAboutTab()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setSpacing(16);
    layout->setContentsMargins(32, 32, 32, 32);

    // App name
    QLabel *titleLabel = new QLabel(
        QStringLiteral("<h1 style='color:#ffffff;'>%1</h1>").arg(AppConfig::AppName));
    layout->addWidget(titleLabel);

    m_versionLabel = new QLabel(
        tr("Version: <b>%1</b>").arg(APP_VERSION));
    m_versionLabel->setStyleSheet("font-size: 12pt;");
    layout->addWidget(m_versionLabel);

    QLabel *descLabel = new QLabel(
        tr("A Steam game server launcher supporting Icarus Dedicated Server.\n"
           "Manage SteamCMD, configure and launch your server from a single GUI."));
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: #aaa; font-size: 10pt;");
    layout->addWidget(descLabel);

    layout->addSpacing(16);

    // Update section
    QGroupBox *grpUpdate = new QGroupBox(tr("Self Update"));
    QVBoxLayout *updateLayout = new QVBoxLayout(grpUpdate);
    updateLayout->setSpacing(10);
    updateLayout->setContentsMargins(16, 24, 16, 16);

    m_updateStatusLabel = new QLabel(tr("Click the button to check for updates."));
    updateLayout->addWidget(m_updateStatusLabel);

    m_updateProgress = new QProgressBar;
    m_updateProgress->setValue(0);
    m_updateProgress->setVisible(false);
    updateLayout->addWidget(m_updateProgress);

    QHBoxLayout *updateBtnRow = new QHBoxLayout;
    m_btnCheckUpdate = new QPushButton(tr("Check for Updates"));
    m_btnDownloadUpdate = new QPushButton(tr("Download Update"));
    m_btnDownloadUpdate->setEnabled(false);
    updateBtnRow->addWidget(m_btnCheckUpdate);
    updateBtnRow->addWidget(m_btnDownloadUpdate);
    updateBtnRow->addStretch();
    updateLayout->addLayout(updateBtnRow);

    layout->addWidget(grpUpdate);

    layout->addStretch();

    // Footer
    QLabel *footerLabel = new QLabel(
        QStringLiteral("<a href='https://github.com/%1/%2' style='color:#58a6ff;'>"
                       "GitHub Repository</a>")
            .arg(AppConfig::GithubOwner, AppConfig::GithubRepo));
    footerLabel->setOpenExternalLinks(true);
    layout->addWidget(footerLabel);

    // Connections
    connect(m_btnCheckUpdate, &QPushButton::clicked, this, &MainWindow::onCheckForUpdate);
    connect(m_btnDownloadUpdate, &QPushButton::clicked, this, [this]() {
        if (!m_pendingZipPath.isEmpty()) {
            // We have already downloaded — apply the update
            m_updater->applyUpdate(m_pendingZipPath);
        } else if (!m_pendingDownloadUrl.isEmpty()) {
            m_updateProgress->setVisible(true);
            m_btnDownloadUpdate->setEnabled(false);
            m_updater->downloadUpdate(m_pendingDownloadUrl);
        }
    });

    return page;
}

// ═══════════════════════════════════════════════════════════════════
//  Action Handlers
// ═══════════════════════════════════════════════════════════════════

void MainWindow::onInstallSteamCmd()
{
    appendLog(tr("--- Installing SteamCMD ---"));
    m_steamCmd->downloadSteamCmd();
}

void MainWindow::onUpdateServer()
{
    saveSettingsFromUI(); // persist first
    const QString installDir = serverInstallDir();
    appendLog(tr("--- Updating Icarus Server (App %1) ---").arg(AppConfig::IcarusAppId));
    m_steamCmd->installOrUpdateServer(AppConfig::IcarusAppId, installDir);
}

void MainWindow::onStartServer()
{
    saveSettingsFromUI();

    const QJsonObject settings =
        ServerManager::loadSettings(settingsFilePath());
    const QString exePath = m_editServerExePath->text().trimmed();

    if (exePath.isEmpty()) {
        appendLog(tr("Please set the server executable path in the Settings tab."));
        return;
    }

    m_serverMgr->setServerExecutable(exePath);
    m_serverMgr->startServer(ServerManager::buildLaunchArgs(settings));
}

void MainWindow::onStopServer()
{
    m_serverMgr->stopServer();
}

void MainWindow::onCheckForUpdate()
{
    m_btnCheckUpdate->setEnabled(false);
    m_updateStatusLabel->setText(tr("Checking..."));
    m_pendingDownloadUrl.clear();
    m_pendingZipPath.clear();
    m_btnDownloadUpdate->setText(tr("⬇  Download Update"));
    m_btnDownloadUpdate->setEnabled(false);
    m_updateProgress->setVisible(false);
    m_updateProgress->setValue(0);
    m_updater->checkForUpdate();
}

void MainWindow::onBrowseServerExe()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select Server Executable"), QString(),
        tr("Executable (*.exe);;All Files (*)"));
    if (!path.isEmpty()) {
        m_editServerExePath->setText(QDir::toNativeSeparators(path));
    }
}

void MainWindow::onBrowseSteamCmdPath()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("Select SteamCMD Directory"), m_editSteamCmdPath->text());
    if (!path.isEmpty()) {
        m_editSteamCmdPath->setText(QDir::toNativeSeparators(path));
    }
}

void MainWindow::onBrowseServerBasePath()
{
    const QString path = QFileDialog::getExistingDirectory(
        this, tr("Select Server Base Directory"), m_editServerBasePath->text());
    if (!path.isEmpty()) {
        m_editServerBasePath->setText(QDir::toNativeSeparators(path));
    }
}

// ═══════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════

void MainWindow::appendLog(const QString &message)
{
    m_logOutput->append(message);
    // Auto-scroll to bottom
    QScrollBar *sb = m_logOutput->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::updateServerStateUI()
{
    using S = ServerManager::ServerState;
    switch (m_serverMgr->state()) {
    case S::Stopped:
        m_statusLabel->setText(tr("●  已停止"));
        m_statusLabel->setStyleSheet("font-size:14pt; font-weight:bold; color:#888;");
        m_btnStart->setEnabled(true);
        m_btnStop->setEnabled(false);
        break;
    case S::Starting:
        m_statusLabel->setText(tr("●  啟動中..."));
        m_statusLabel->setStyleSheet("font-size:14pt; font-weight:bold; color:#66c0f4;");
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(true);
        break;
    case S::Running:
        m_statusLabel->setText(tr("●  運行中"));
        m_statusLabel->setStyleSheet("font-size:14pt; font-weight:bold; color:#8ab41f;");
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(true);
        break;
    case S::Stopping:
        m_statusLabel->setText(tr("●  停止中..."));
        m_statusLabel->setStyleSheet("font-size:14pt; font-weight:bold; color:#eb4b4b;");
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(false);
        break;
    }
}

QString MainWindow::settingsFilePath() const
{
    return QCoreApplication::applicationDirPath() + "/" + AppConfig::ConfigFileName;
}

QString MainWindow::serverInstallDir() const
{
    const QJsonObject s = ServerManager::loadSettings(settingsFilePath());
    QString basePath = s.value("serverBasePath").toString();
    if (basePath.isEmpty()) {
        basePath = QCoreApplication::applicationDirPath() + "/" + AppConfig::ServersSubDir;
    }
    return basePath + "/icarus";
}

void MainWindow::loadSettingsToUI()
{
    const QJsonObject s = ServerManager::loadSettings(settingsFilePath());
    m_editServerName->setText(    s.value("serverName").toString());
    m_editPassword->setText(      s.value("password").toString());
    m_editAdminPassword->setText( s.value("adminPassword").toString());
    m_spinMaxPlayers->setValue(   s.value("maxPlayers").toInt(AppConfig::DefaultMaxPlayers));
    m_spinPort->setValue(         s.value("port").toInt(AppConfig::DefaultPort));
    m_spinQueryPort->setValue(    s.value("queryPort").toInt(AppConfig::DefaultQueryPort));
    m_editSteamCmdPath->setText(  s.value("steamCmdPath").toString());
    m_editServerBasePath->setText(s.value("serverBasePath").toString());
    m_editServerExePath->setText( s.value("serverExePath").toString());
    m_editAdditionalArgs->setText(s.value("additionalArgs").toString());
}

void MainWindow::saveSettingsFromUI()
{
    QJsonObject s;
    s["serverName"]     = m_editServerName->text();
    s["password"]       = m_editPassword->text();
    s["adminPassword"]  = m_editAdminPassword->text();
    s["maxPlayers"]     = m_spinMaxPlayers->value();
    s["port"]           = m_spinPort->value();
    s["queryPort"]      = m_spinQueryPort->value();
    s["steamCmdPath"]   = m_editSteamCmdPath->text();
    s["serverBasePath"] = m_editServerBasePath->text();
    s["serverExePath"]  = m_editServerExePath->text();
    s["additionalArgs"] = m_editAdditionalArgs->text();

    if (ServerManager::saveSettings(settingsFilePath(), s)) {
        appendLog(tr("Settings saved."));
        // Update SteamCmdManager with new path immediately
        QString steamPath = m_editSteamCmdPath->text();
        if (steamPath.isEmpty()) {
            steamPath = QCoreApplication::applicationDirPath() + "/" + AppConfig::SteamCmdSubDir;
        }
        m_steamCmd->setSteamCmdDir(steamPath);
    } else {
        appendLog(tr("⚠ Failed to save settings."));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_serverMgr->isRunning()) {
        const auto reply = QMessageBox::question(
            this, tr("Server Running"),
            tr("The game server is still running.\n"
               "Do you want to stop it and exit?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            m_serverMgr->stopServer();
            event->accept();
        } else {
            event->ignore();
        }
    } else {
        event->accept();
    }
}
