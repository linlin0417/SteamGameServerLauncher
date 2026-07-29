#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QTextEdit;
class QProgressBar;
class ServerInstance;

/// 伺服器控制面板。
/// 顯示伺服器狀態、操作按鈕和 Console 日誌。
class ServerControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ServerControlPanel(QWidget *parent = nullptr);

    /// 綁定到 ServerInstance（切換設定檔時呼叫）
    void bindInstance(ServerInstance *instance);
    
    /// 解除綁定
    void unbindInstance();

private:
    void setupUI();
    void updateButtonVisibility();
    void updateStateUI();
    void appendLog(const QString &message);

    ServerInstance *m_instance = nullptr;

    // UI elements
    QLabel *m_gameNameLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QPushButton *m_btnInstallCmd = nullptr;
    QPushButton *m_btnUpdateServer = nullptr;
    QPushButton *m_btnCheckUpdate = nullptr;
    QPushButton *m_btnRunInstallScript = nullptr;
    QPushButton *m_btnRunUpdateScript = nullptr;
    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnStop = nullptr;
    QTextEdit *m_logOutput = nullptr;
    QProgressBar *m_progressBar = nullptr;
};
