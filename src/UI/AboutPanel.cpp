#include "AboutPanel.h"
#include "../Core/GithubUpdater.h"
#include "../version.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QDateTime>

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
    m_btnDownloadUpdate->setEnabled(false);
    updateBtnRow->addWidget(m_btnCheckUpdate);
    updateBtnRow->addWidget(m_btnDownloadUpdate);
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

    connect(m_btnCheckUpdate, &QPushButton::clicked, this, &AboutPanel::onCheckForUpdate);
    connect(m_btnDownloadUpdate, &QPushButton::clicked, this, &AboutPanel::onDownloadUpdate);
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
