#pragma once

#include <QWidget>

class QComboBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class QCheckBox;
class ServerInstance;

/// 存檔管理面板。
/// 根據 GameProfile 的 savesRelativePath 和 saveFilePatterns 動態運作。
class SaveManagerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit SaveManagerPanel(QWidget *parent = nullptr);

    void bindInstance(ServerInstance *instance);
    void unbindInstance();

signals:
    void logMessage(const QString &msg);

private slots:
    void refreshSaveList();
    void onExportSave();
    void onImportSave();
    void onBrowsePreview();

private:
    void setupUI();
    QString savesDir() const;

    ServerInstance *m_instance = nullptr;

    // 匯出區塊
    QComboBox *m_comboSaves = nullptr;
    QLineEdit *m_editPackageName = nullptr;
    QTextEdit *m_editNotes = nullptr;
    QLineEdit *m_editPreviewPath = nullptr;
    QPushButton *m_btnRefresh = nullptr;
    QPushButton *m_btnExport = nullptr;
    QCheckBox *m_chkLegacyFormat = nullptr; // Icarus 用: 同時匯出 .IcarusMap

    // 匯入區塊
    QPushButton *m_btnImport = nullptr;
    QLabel *m_importInfoLabel = nullptr;
    QLabel *m_previewImage = nullptr;

    // 無存檔設定時的提示
    QLabel *m_noSaveLabel = nullptr;
    QWidget *m_contentWidget = nullptr;
};
