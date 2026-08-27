#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTextEdit;
class QProgressBar;
class GithubUpdater;

/// 關於/更新面板。
class AboutPanel : public QWidget
{
    Q_OBJECT

public:
    explicit AboutPanel(GithubUpdater *updater, QWidget *parent = nullptr);

signals:
    void logMessage(const QString &msg);

private:
    void setupUI();
    void onCheckForUpdate();
    void onDownloadUpdate();
    void onInstallLocalUpdate();   // 手動選擇本地更新包安裝

    GithubUpdater *m_updater = nullptr;

    QLabel       *m_versionLabel      = nullptr;
    QLabel       *m_updateStatusLabel = nullptr;
    QTextEdit    *m_updateNotesEdit   = nullptr;
    QPushButton  *m_btnCheckUpdate    = nullptr;
    QPushButton  *m_btnDownloadUpdate = nullptr;
    QPushButton  *m_btnInstallLocal   = nullptr;   // 手動安裝本地更新包
    QProgressBar *m_updateProgress    = nullptr;

    QString m_pendingDownloadUrl;
    QString m_pendingSha256Url;
};
