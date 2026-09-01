#include "ServerControlPanel.h"
#include "../Core/ServerInstance.h"
#include "../Core/ServerManager.h"
#include "../Core/GameProfile.h"
#include "../Core/LogColorizer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QTextDocument>

ServerControlPanel::ServerControlPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ServerControlPanel::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(16, 16, 16, 16);

    // 狀態列
    QHBoxLayout *statusRow = new QHBoxLayout;
    m_gameNameLabel = new QLabel(tr("[尚未選擇]"));
    m_gameNameLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #c7d5e0;"));
    
    m_statusLabel = new QLabel(tr("狀態: 已停止"));
    m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #888;"));
    
    statusRow->addWidget(m_gameNameLabel);
    statusRow->addStretch();
    statusRow->addWidget(m_statusLabel);
    layout->addLayout(statusRow);

    // 按鈕列
    QHBoxLayout *btnRow = new QHBoxLayout;
    
    m_btnInstallCmd = new QPushButton(tr("安裝 SteamCMD"));
    m_btnUpdateServer = new QPushButton(tr("更新伺服器"));
    m_btnCheckUpdate = new QPushButton(tr("檢查更新"));
    
    m_btnRunInstallScript = new QPushButton(tr("執行安裝腳本"));
    m_btnRunUpdateScript = new QPushButton(tr("執行更新腳本"));
    
    m_btnStart = new QPushButton(tr("啟動"));
    m_btnStop = new QPushButton(tr("停止"));

    m_btnStart->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #79a01b, stop:1 #5c7e10); color: #d2efa9; border: 1px solid #455e09; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8ab41f, stop:1 #6b9313); color: #ffffff; }"
        "QPushButton:disabled { background: #2a2f35; color: #555; border: none; }"));
        
    m_btnStop->setStyleSheet(QStringLiteral(
        "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #3c2a2a, stop:1 #2e1c1c); color: #e8a7a7; border: 1px solid #281515; }"
        "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4f3636, stop:1 #3e2626); color: #ffffff; }"
        "QPushButton:disabled { background: #2a2f35; color: #555; border: none; }"));

    btnRow->addWidget(m_btnInstallCmd);
    btnRow->addWidget(m_btnUpdateServer);
    btnRow->addWidget(m_btnCheckUpdate);
    btnRow->addWidget(m_btnRunInstallScript);
    btnRow->addWidget(m_btnRunUpdateScript);
    btnRow->addStretch();
    btnRow->addWidget(m_btnStart);
    btnRow->addWidget(m_btnStop);
    layout->addLayout(btnRow);

    // 進度條
    m_progressBar = new QProgressBar;
    m_progressBar->setVisible(false);
    layout->addWidget(m_progressBar);

    // 日誌區
    m_logOutput = new QTextEdit;
    m_logOutput->setReadOnly(true);
    m_logOutput->setPlaceholderText(tr("Console 日誌區..."));
    m_logOutput->document()->setMaximumBlockCount(10000);
    m_logOutput->setStyleSheet(QStringLiteral(
        "QTextEdit { "
        "background-color: #101214; "
        "color: #a0b1c0; "
        "border: 1px solid #202d39; "
        "border-radius: 4px; "
        "font-family: 'Cascadia Code', 'Consolas', monospace; "
        "font-size: 9pt; "
        "padding: 6px; "
        "selection-background-color: #264f78; "
        "}"));
    layout->addWidget(m_logOutput, 1);

    // 信號連接
    connect(m_btnInstallCmd, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->downloadSteamCmd();
    });
    connect(m_btnUpdateServer, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->installOrUpdate();
    });
    connect(m_btnCheckUpdate, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->checkUpdate();
    });
    connect(m_btnRunInstallScript, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->installOrUpdate();
    });
    connect(m_btnRunUpdateScript, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->installOrUpdate();
    });
    connect(m_btnStart, &QPushButton::clicked, this, [this]() {
        emit startRequested();
        if (m_instance) m_instance->startServer();
    });
    connect(m_btnStop, &QPushButton::clicked, this, [this]() {
        if (m_instance) m_instance->stopServer();
    });

    updateButtonVisibility();
    updateStateUI();
}

void ServerControlPanel::bindInstance(ServerInstance *instance)
{
    unbindInstance();
    m_instance = instance;

    if (!m_instance) {
        m_gameNameLabel->setText(tr("[尚未選擇]"));
        updateButtonVisibility();
        updateStateUI();
        return;
    }

    GameProfile profile = m_instance->profile();
    m_gameNameLabel->setText(QStringLiteral("[%1]").arg(profile.displayName));

    connect(m_instance, &ServerInstance::logMessage, this, &ServerControlPanel::appendLog);
    connect(m_instance, &ServerInstance::stateChanged, this, [this](ServerManager::ServerState) {
        updateStateUI();
    });
    connect(m_instance, &ServerInstance::operationFinished, this, [this](bool success, const QString &msg) {
        updateStateUI();
        QString prefix = success ? QStringLiteral("[OK] ") : QStringLiteral("[FAIL] ");
        appendLog(prefix + msg);
    });

    updateButtonVisibility();
    updateStateUI();
}

void ServerControlPanel::unbindInstance()
{
    if (m_instance) {
        disconnect(m_instance, nullptr, this, nullptr);
        m_instance = nullptr;
    }
}

void ServerControlPanel::updateButtonVisibility()
{
    m_btnInstallCmd->setVisible(false);
    m_btnUpdateServer->setVisible(false);
    m_btnCheckUpdate->setVisible(false);
    m_btnRunInstallScript->setVisible(false);
    m_btnRunUpdateScript->setVisible(false);

    if (!m_instance) {
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(false);
        return;
    }

    GameProfile profile = m_instance->profile();
    switch (profile.sourceType) {
        case GameProfile::SteamCMD:
            m_btnInstallCmd->setVisible(true);
            m_btnUpdateServer->setVisible(true);
            m_btnCheckUpdate->setVisible(true);
            break;
        case GameProfile::CustomScript:
            m_btnRunInstallScript->setVisible(true);
            m_btnRunUpdateScript->setVisible(true);
            break;
        case GameProfile::ManualOnly:
        default:
            break;
    }
}

void ServerControlPanel::updateStateUI()
{
    if (!m_instance) {
        m_statusLabel->setText(tr("狀態: 尚未選擇"));
        m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #888;"));
        m_btnStart->setEnabled(false);
        m_btnStop->setEnabled(false);
        return;
    }

    ServerManager::ServerState state = m_instance->serverState();
    
    switch (state) {
        case ServerManager::ServerState::Stopped:
            m_statusLabel->setText(tr("狀態: 已停止"));
            m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #888;"));
            m_btnStart->setEnabled(true);
            m_btnStop->setEnabled(false);
            break;
        case ServerManager::ServerState::Starting:
            m_statusLabel->setText(tr("狀態: 啟動中..."));
            m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #f39c12;"));
            m_btnStart->setEnabled(false);
            m_btnStop->setEnabled(false);
            break;
        case ServerManager::ServerState::Running:
            m_statusLabel->setText(tr("狀態: 已啟動"));
            m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #2ecc71;"));
            m_btnStart->setEnabled(false);
            m_btnStop->setEnabled(true);
            break;
        case ServerManager::ServerState::Stopping:
            m_statusLabel->setText(tr("狀態: 停止中..."));
            m_statusLabel->setStyleSheet(QStringLiteral("font-size: 14pt; font-weight: bold; color: #e74c3c;"));
            m_btnStart->setEnabled(false);
            m_btnStop->setEnabled(false);
            break;
    }
}

void ServerControlPanel::appendLog(const QString &message)
{
    if (m_logOutput) {
        m_logOutput->append(LogColorizer::formatToHtml(message));
    }
}
