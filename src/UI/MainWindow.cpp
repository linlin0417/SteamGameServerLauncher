#include "MainWindow.h"
#include "../version.h"

#include "../Core/GameProfileManager.h"
#include "../Core/ServerInstance.h"
#include "../Core/SettingsMigrator.h"
#include "../Core/GithubUpdater.h"
#include "../Core/SteamCmdManager.h"

#include "SidebarWidget.h"
#include "ServerControlPanel.h"
#include "ServerSettingsPanel.h"
#include "SaveManagerPanel.h"
#include "AboutPanel.h"
#include "GameProfileDialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStackedWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("%1 v%2").arg(AppConfig::AppName, APP_VERSION));
    setMinimumSize(900, 650);
    resize(1050, 750);

    applyDarkTheme();

    m_steamCmd = new SteamCmdManager(this);
    m_updater = new GithubUpdater(AppConfig::GithubOwner, AppConfig::GithubRepo, APP_VERSION, this);
    m_profileMgr = new GameProfileManager(dataRootDir() + QStringLiteral("/") + AppConfig::ProfilesSubDir, this);

    checkMigration();
    setupUI();
    loadGlobalSettings();

    if (m_currentProfileId.isEmpty()) {
        const QList<GameProfile> profiles = m_profileMgr->allProfiles();
        if (!profiles.isEmpty()) {
            m_currentProfileId = profiles.first().id;
        }
    }

    switchToProfile(m_currentProfileId);
}

MainWindow::~MainWindow() = default;

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

void MainWindow::setupUI()
{
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_sidebar = new SidebarWidget(this);
    m_contentStack = new QStackedWidget(this);

    m_controlPanel = new ServerControlPanel(this);
    m_settingsPanel = new ServerSettingsPanel(this);
    m_savePanel = new SaveManagerPanel(this);
    m_aboutPanel = new AboutPanel(m_updater, this);

    m_contentStack->addWidget(m_controlPanel); // index 0
    m_contentStack->addWidget(m_settingsPanel); // index 1
    m_contentStack->addWidget(m_savePanel); // index 2
    m_contentStack->addWidget(m_aboutPanel); // index 3

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_contentStack, 1);

    setCentralWidget(centralWidget);

    connect(m_controlPanel, &ServerControlPanel::startRequested, m_settingsPanel, &ServerSettingsPanel::saveSettingsFromUI);

    connect(m_sidebar, &SidebarWidget::profileSelected, this, &MainWindow::onProfileSelected);
    connect(m_sidebar, &SidebarWidget::panelRequested, this, [this](SidebarWidget::PanelType type) {
        onPanelRequested(static_cast<int>(type));
    });
    connect(m_sidebar, &SidebarWidget::addProfileRequested, this, &MainWindow::onAddProfile);

    m_sidebar->setProfiles(m_profileMgr->allProfiles());

    connect(m_profileMgr, &GameProfileManager::profilesChanged, this, [this]() {
        m_sidebar->setProfiles(m_profileMgr->allProfiles());
    });
}

void MainWindow::onProfileSelected(const QString &profileId)
{
    switchToProfile(profileId);
    m_contentStack->setCurrentIndex(0);
}

void MainWindow::switchToProfile(const QString &profileId)
{
    if (m_currentInstance && m_currentInstance->isRunning()) {
        QMessageBox::warning(this, tr("伺服器運行中"), tr("請先停止目前正在運行的伺服器，再切換設定檔。"));
        m_sidebar->setCurrentProfileId(m_currentProfileId);
        return;
    }

    if (m_controlPanel) {
        m_controlPanel->unbindInstance();
    }
    if (m_settingsPanel) {
        m_settingsPanel->unbindInstance();
    }

    if (m_currentInstance) {
        m_currentInstance->deleteLater();
        m_currentInstance = nullptr;
    }

    if (profileId.isEmpty()) {
        return;
    }

    GameProfile profile = m_profileMgr->profileById(profileId);
    if (profile.id.isEmpty()) {
        return;
    }

    m_currentInstance = new ServerInstance(m_steamCmd, this);
    m_currentInstance->setProfile(profile);

    const QString installDir = dataRootDir() + QStringLiteral("/instances/") + profileId;
    m_currentInstance->setInstallDir(installDir);

    const QString settingsPath = installDir + QStringLiteral(".json");
    m_currentInstance->loadSettings(settingsPath);

    QString steamPath = m_currentInstance->settings().value(QStringLiteral("steamCmdPath")).toString();
    if (steamPath.isEmpty()) {
        steamPath = dataRootDir() + QStringLiteral("/") + AppConfig::SteamCmdSubDir;
    }
    m_steamCmd->setSteamCmdDir(steamPath);

    if (m_controlPanel) {
        m_controlPanel->bindInstance(m_currentInstance);
    }
    if (m_settingsPanel) {
        m_settingsPanel->bindInstance(m_currentInstance);
    }

    m_sidebar->setCurrentProfileId(profileId);
    m_currentProfileId = profileId;
}

void MainWindow::onPanelRequested(int type)
{
    m_contentStack->setCurrentIndex(type);
}

void MainWindow::onAddProfile()
{
    GameProfileDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        GameProfile newProfile = dialog.resultProfile();
        m_profileMgr->saveProfile(newProfile);
        m_profileMgr->reload();
        switchToProfile(newProfile.id);
    }
}

void MainWindow::checkMigration()
{
    if (SettingsMigrator::needsMigration(dataRootDir())) {
        if (SettingsMigrator::migrate(dataRootDir())) {
            m_profileMgr->reload();
        }
    }
}

void MainWindow::loadGlobalSettings()
{
    QFile file(globalSettingsPath());
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            m_currentProfileId = obj.value(QStringLiteral("lastSelectedProfile")).toString();
        }
    }
}

void MainWindow::saveGlobalSettings()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("lastSelectedProfile"), m_currentProfileId);

    QJsonDocument doc(obj);
    QFile file(globalSettingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_currentInstance && m_currentInstance->isRunning()) {
        QMessageBox::StandardButton btn = QMessageBox::warning(
            this,
            tr("伺服器運行中"),
            tr("伺服器目前正在運行中。您確定要關閉啟動器嗎？\n這將會強制停止伺服器。"),
            QMessageBox::Yes | QMessageBox::No
        );

        if (btn == QMessageBox::No) {
            event->ignore();
            return;
        }

        m_currentInstance->stopServer();
    }

    saveGlobalSettings();
    event->accept();
}

QString MainWindow::dataRootDir() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/GameData");
}

QString MainWindow::globalSettingsPath() const
{
    return dataRootDir() + QStringLiteral("/") + AppConfig::ConfigFileName;
}
