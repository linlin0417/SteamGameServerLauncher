#pragma once

#include <QWidget>
#include <QList>
#include <QString>

class QVBoxLayout;
class QLabel;
class QPushButton;
class QListWidget;
class QListWidgetItem;

struct GameProfile;

/// 側邊導覽列元件。
/// 顯示遊戲設定檔清單、狀態指示、功能切換。
class SidebarWidget : public QWidget
{
    Q_OBJECT

public:
    enum PanelType {
        PanelControl,
        PanelSettings,
        PanelSaveManager,
        PanelAbout
    };

    explicit SidebarWidget(QWidget *parent = nullptr);

    /// 更新設定檔清單
    void setProfiles(const QList<GameProfile> &profiles);

    /// 更新指定設定檔的伺服器狀態
    void updateServerState(const QString &profileId, int state);
    // state: 0=Stopped, 1=Starting, 2=Running, 3=Stopping

    /// 取得目前選中的設定檔 ID
    QString currentProfileId() const;

    /// 設定目前選中的設定檔
    void setCurrentProfileId(const QString &id);

signals:
    void profileSelected(const QString &profileId);
    void panelRequested(PanelType type);
    void addProfileRequested();

private:
    void setupUI();
    void onProfileClicked(QListWidgetItem *item);
    
    void setItemState(QListWidgetItem *item, int state);

    QLabel *m_titleLabel = nullptr;
    QLabel *m_versionLabel = nullptr;
    QListWidget *m_profileList = nullptr;
    QPushButton *m_btnAddProfile = nullptr;
    QPushButton *m_btnControl = nullptr;
    QPushButton *m_btnSettings = nullptr;
    QPushButton *m_btnSaves = nullptr;
    QPushButton *m_btnAbout = nullptr;
    
    QString m_currentProfileId;
};
