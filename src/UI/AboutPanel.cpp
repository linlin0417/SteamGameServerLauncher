#include "AboutPanel.h"
#include "../Core/GithubUpdater.h"
#include "../Core/VersionParser.h"
#include "../version.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QDateTime>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>

AboutPanel::AboutPanel(GithubUpdater *updater, QWidget *parent)
    : QWidget(parent)
    , m_updater(updater)
{
    setupUI();

    if (m_updater) {
        connect(m_updater, &GithubUpdater::updateAvailable,
                this, [this](const QString &ver, const QString &url, const QString &notes, qint64 sizeBytes, const QString &publishDate, const QString &sha256Url) {
            
            QString sizeStr = (sizeBytes > 0) ? QString::number(sizeBytes / (1024.0 * 1024.0), 'f', 1) + QStringLiteral(" MB") : QStringLiteral("Unknown Size");
            QString dateStr = !publishDate.isEmpty() ? QDateTime::fromString(publishDate, Qt::ISODate).toString(QStringLiteral("yyyy-MM-dd")) : QStringLiteral("Unknown Date");

            m_updateStatusLabel->setText(
                tr("<span style='color:#66c0f4;'>New version available: <b>%1</b> (%2, Released: %3)</span>").arg(ver, sizeStr, dateStr));
            
            m_pendingDownloadUrl = url;
            m_pendingSha256Url = sha256Url;
            
            m_btnDownloadUpdate->setEnabled(!url.isEmpty());
            m_btnCheckUpdate->setEnabled(true);
            emit logMessage(tr("[Updater] New version %1 available!").arg(ver));
            
            if (!notes.isEmpty()) {
                emit logMessage(tr("[Updater] Release notes: %1").arg(notes));
                m_updateNotesEdit->setMarkdown(notes);
                m_updateNotesEdit->setVisible(true);
            }
        });

        connect(m_updater, &GithubUpdater::noUpdateAvailable,
                this, [this]() {
            m_updateStatusLabel->setText(
                tr("<span style='color:#c7d5e0;'>You are up to date.</span>"));
            m_btnCheckUpdate->setEnabled(true);
            m_updateNotesEdit->setVisible(false);
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
            emit logMessage(tr("[Updater] Download complete: %1").arg(path));
            m_updateStatusLabel->setText(
                tr("<span style='color:#66c0f4;'>Download complete. Applying update...</span>"));
            m_btnDownloadUpdate->setText(tr("Applying Update..."));
            m_btnDownloadUpdate->setEnabled(false);
            
            m_updater->applyUpdate(path);
        });

        connect(m_updater, &GithubUpdater::updateError,
                this, [this](const QString &err) {
            m_updateStatusLabel->setText(
                tr("<span style='color:#eb4b4b;'>Error: %1</span>").arg(err));
            m_btnCheckUpdate->setEnabled(true);
            m_btnDownloadUpdate->setEnabled(false);
        });
    }
}

void AboutPanel::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(16);
    layout->setContentsMargins(32, 32, 32, 32);

    QLabel *titleLabel = new QLabel(
        QStringLiteral("<h1 style='color:#ffffff;'>%1</h1>").arg(AppConfig::AppName));
    layout->addWidget(titleLabel);

    m_versionLabel = new QLabel(
        tr("Version: <b>%1</b>").arg(APP_VERSION));
    m_versionLabel->setStyleSheet(QStringLiteral("font-size: 12pt;"));
    layout->addWidget(m_versionLabel);

    QLabel *descLabel = new QLabel(
        tr("A Steam game server launcher"));
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet(QStringLiteral("color: #aaa; font-size: 10pt;"));
    layout->addWidget(descLabel);

    layout->addSpacing(16);

    QGroupBox *grpUpdate = new QGroupBox(tr("Self Update"));
    QVBoxLayout *updateLayout = new QVBoxLayout(grpUpdate);
    updateLayout->setSpacing(10);
    updateLayout->setContentsMargins(16, 24, 16, 16);

    m_updateStatusLabel = new QLabel(tr("Click the button to check for updates."));
    updateLayout->addWidget(m_updateStatusLabel);

    m_updateNotesEdit = new QTextEdit;
    m_updateNotesEdit->setReadOnly(true);
    m_updateNotesEdit->setVisible(false);
    m_updateNotesEdit->setMinimumHeight(150);
    updateLayout->addWidget(m_updateNotesEdit);

    m_updateProgress = new QProgressBar;
    m_updateProgress->setValue(0);
    m_updateProgress->setVisible(false);
    updateLayout->addWidget(m_updateProgress);

    QHBoxLayout *updateBtnRow = new QHBoxLayout;
    m_btnCheckUpdate = new QPushButton(tr("Check for Updates"));
    m_btnDownloadUpdate = new QPushButton(tr("Download Update"));
    m_btnInstallLocal = new QPushButton(tr("手動安裝更新檔"));
    m_btnDownloadUpdate->setEnabled(false);
    updateBtnRow->addWidget(m_btnCheckUpdate);
    updateBtnRow->addWidget(m_btnDownloadUpdate);
    updateBtnRow->addWidget(m_btnInstallLocal);
    updateBtnRow->addStretch();
    updateLayout->addLayout(updateBtnRow);

    layout->addWidget(grpUpdate);

    layout->addStretch();

    QLabel *footerLabel = new QLabel(
        QStringLiteral("<a href='https://github.com/%1/%2' style='color:#58a6ff;'>"
                       "GitHub Repository</a>")
            .arg(AppConfig::GithubOwner, AppConfig::GithubRepo));
    footerLabel->setOpenExternalLinks(true);
    layout->addWidget(footerLabel);

    connect(m_btnCheckUpdate,    &QPushButton::clicked, this, &AboutPanel::onCheckForUpdate);
    connect(m_btnDownloadUpdate, &QPushButton::clicked, this, &AboutPanel::onDownloadUpdate);
    connect(m_btnInstallLocal,   &QPushButton::clicked, this, &AboutPanel::onInstallLocalUpdate);
}

void AboutPanel::onCheckForUpdate()
{
    if (m_updater) {
        m_btnCheckUpdate->setEnabled(false);
        m_updateStatusLabel->setText(tr("<span style='color:#c7d5e0;'>Checking for updates...</span>"));
        m_updater->checkForUpdate();
    }
}

void AboutPanel::onDownloadUpdate()
{
    if (m_updater && !m_pendingDownloadUrl.isEmpty()) {
        m_updateProgress->setVisible(true);
        m_updateProgress->setValue(0);
        m_updater->downloadUpdate(m_pendingDownloadUrl, m_pendingSha256Url);
        m_btnDownloadUpdate->setEnabled(false);
        m_btnCheckUpdate->setEnabled(false);
    }
}

void AboutPanel::onInstallLocalUpdate()
{
    // 開啟檔案選擇對話框，同時支援 .sgsl_update 與 .zip 格式
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("選擇更新包"),
        QCoreApplication::applicationDirPath(),
        tr("更新包 (*.sgsl_update *.zip)")
    );

    if (filePath.isEmpty()) { return; }

    // 嘗試讀取 .info.json 附帶的版本資訊
    QString infoText;
    const QString infoPath = filePath + QStringLiteral(".info.json");
    if (QFileInfo::exists(infoPath)) {
        QFile f(infoPath);
        if (f.open(QIODevice::ReadOnly)) {
            const QJsonObject info = QJsonDocument::fromJson(f.readAll()).object();
            const QString ver  = info.value(QStringLiteral("version")).toString();
            const QString type = info.value(QStringLiteral("type")).toString();
            const QString desc = info.value(QStringLiteral("description")).toString();

            if (!ver.isEmpty()) {
                const AppVersion parsed = VersionParser::parse(ver);
                infoText = QStringLiteral(
                    "版本：<b>%1</b>（%2）<br>"
                    "類型：%3<br>"
                    "說明：%4"
                ).arg(ver, VersionParser::typeLabel(parsed), type, desc.isEmpty() ? tr("無") : desc);
            }
        }
    }

    const QString confirmMsg = infoText.isEmpty()
        ? tr("確認安裝以下更新包？\n\n%1").arg(QFileInfo(filePath).fileName())
        : tr("確認安裝以下更新包？\n\n檔案：%1\n\n%2")
              .arg(QFileInfo(filePath).fileName(), infoText);

    // 顯示確認對話框（使用純文字，去除 HTML 標籤供 QMessageBox 使用）
    QString plainMsg = confirmMsg;
    plainMsg.remove(QRegularExpression(QStringLiteral("<[^>]*>")));

    if (QMessageBox::question(this, tr("確認安裝"), plainMsg,
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 委由 GithubUpdater::applyUpdate() 呼叫 Updater.exe 安裝
    if (m_updater) {
        emit logMessage(tr("[Updater] 開始手動安裝：%1").arg(filePath));
        m_updateStatusLabel->setText(
            tr("<span style='color:#66c0f4;'>正在套用手動更新包，程式即將重啟...</span>"));
        m_btnInstallLocal->setEnabled(false);
        m_btnCheckUpdate->setEnabled(false);
        m_btnDownloadUpdate->setEnabled(false);
        m_updater->applyUpdate(filePath);
    }
}
