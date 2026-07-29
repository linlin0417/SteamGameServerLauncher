#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QMap>

class QFormLayout;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QGroupBox;
class QScrollArea;
class ServerInstance;
class DiscordManager;

/// 伺服器設定面板。
/// 根據 GameProfile 的啟動參數模板動態產生對應的輸入欄位。
class ServerSettingsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ServerSettingsPanel(QWidget *parent = nullptr);

    /// 綁定到 ServerInstance
    void bindInstance(ServerInstance *instance);
    void unbindInstance();

signals:
    void logMessage(const QString &msg);

private slots:
    void onTestWebhook();
    void onBrowseJavaPath();

private:
    void setupUI();
    void rebuildDynamicForm();
    void loadSettingsToUI();
    void saveSettingsFromUI();
    void resetToDefaults();

    ServerInstance *m_instance = nullptr;

    // 固定區塊
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_scrollContent = nullptr;
    
    // 動態表單區塊
    QGroupBox *m_grpDynamic = nullptr;
    QFormLayout *m_dynamicForm = nullptr;
    QMap<QString, QWidget*> m_dynamicWidgets; // variableName -> widget

    // 固定欄位
    QLineEdit *m_editSteamCmdPath = nullptr;
    QLineEdit *m_editInstallDir = nullptr;
    QLineEdit *m_editServerExePath = nullptr;
    QLineEdit *m_editAdditionalArgs = nullptr;
    QLineEdit *m_editDiscordWebhook = nullptr;
    QPushButton *m_btnTestWebhook = nullptr;
};
