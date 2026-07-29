#pragma once

#include <QDialog>
#include "../Core/GameProfile.h"

class QLineEdit;
class QSpinBox;
class QComboBox;
class QTextEdit;
class QPushButton;
class QStackedWidget;

/// 建立/編輯遊戲設定檔的對話框。
class GameProfileDialog : public QDialog
{
    Q_OBJECT

public:
    /// 建立新設定檔
    explicit GameProfileDialog(QWidget *parent = nullptr);
    /// 編輯現有設定檔
    explicit GameProfileDialog(const GameProfile &profile, QWidget *parent = nullptr);

    /// 取得編輯後的設定檔
    GameProfile resultProfile() const;

private:
    void setupUI();
    void loadFromProfile(const GameProfile &p);
    GameProfile buildProfile() const;
    void onSourceTypeChanged(int index);
    void onAccept();

    bool m_isEditMode = false;
    QString m_originalId;

    // UI
    QLineEdit *m_editId = nullptr;
    QLineEdit *m_editDisplayName = nullptr;
    QComboBox *m_comboSourceType = nullptr;
    QStackedWidget *m_sourceStack = nullptr;
    // SteamCMD page
    QLineEdit *m_editSteamAppId = nullptr;
    // CustomScript page
    QLineEdit *m_editInstallScript = nullptr;
    QLineEdit *m_editUpdateScript = nullptr;
    // Manual page - empty
    QLineEdit *m_editExeRelativePath = nullptr;
    QTextEdit *m_editLaunchArgsTemplate = nullptr;
    QComboBox *m_comboConfigFormat = nullptr;
    QLineEdit *m_editConfigFilePath = nullptr;
    QLineEdit *m_editConfigSection = nullptr;
    QLineEdit *m_editSavesRelativePath = nullptr;
    QLineEdit *m_editSaveFilePatterns = nullptr;
    QSpinBox *m_spinDefaultPort = nullptr;
    QSpinBox *m_spinDefaultQueryPort = nullptr;
    QSpinBox *m_spinDefaultMaxPlayers = nullptr;

    GameProfile m_result;
};
