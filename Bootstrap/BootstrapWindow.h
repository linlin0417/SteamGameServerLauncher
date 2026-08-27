#pragma once

#include <QWidget>
#include <QString>

class QLabel;
class QProgressBar;
class QNetworkAccessManager;
class QNetworkReply;

/// Bootstrap 主視窗：全程顯示自檢進度條與狀態文字
class BootstrapWindow : public QWidget
{
    Q_OBJECT

public:
    explicit BootstrapWindow(bool fastLaunch, const QString &manualZipPath, QWidget *parent = nullptr);

    /// 開始執行自檢流程
    void start();

private:
    // --- UI ---
    QLabel       *m_statusLabel  = nullptr;
    QProgressBar *m_progressBar  = nullptr;

    // --- 狀態 ---
    bool    m_fastLaunch    = false;
    QString m_manualZipPath;
    QNetworkAccessManager *m_network    = nullptr;
    QNetworkReply         *m_activeReply = nullptr;

    // --- 設定值（從 launcher_settings.json 讀取） ---
    bool m_autoCheckUpdate   = true;
    bool m_autoExecuteUpdate = false;

    // --- 流程步驟（全部用 lambda + QTimer 串接，無需額外 slot） ---
    void stageManualInstall();
    void stageHotfixCheck();
    void stageConfigCheck();
    void stageUpdateCheck();
    void stageLaunchApp();

    // --- 工具方法 ---
    void setStatus(const QString &msg);
    void setProgress(int value, int total = 100);
    void applyUpdate(const QString &zipPath);
    bool loadConfig();
    QString configPath() const;
    QString appDir() const;

    // --- 下載輔助 ---
    void downloadFile(const QString &url,
                      std::function<void(const QString &savedPath)> onSuccess,
                      std::function<void()> onFail,
                      int progressStart = 0,
                      int progressEnd   = 100);
};