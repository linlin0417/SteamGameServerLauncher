#pragma once

#include <QMainWindow>

class QStackedWidget;
class SidebarWidget;
class ServerControlPanel;
class ServerSettingsPanel;
class SaveManagerPanel;
class AboutPanel;
class GameProfileManager;
class ServerInstance;
class SteamCmdManager;
class GithubUpdater;

/// 主視窗，側邊欄 + 內容區域佈局。
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void applyDarkTheme();
    void setupUI();
    void loadGlobalSettings();
    void saveGlobalSettings();
    QString dataRootDir() const;
    QString globalSettingsPath() const;

    // 設定檔切換
    void onProfileSelected(const QString &profileId);
    void switchToProfile(const QString &profileId);

    // 面板切換
    void onPanelRequested(int type);

    // 新增設定檔
    void onAddProfile();

    // 過渡性遷移檢查
    void checkMigration();

    // Core 模組
    SteamCmdManager *m_steamCmd = nullptr;
    GithubUpdater *m_updater = nullptr;
    GameProfileManager *m_profileMgr = nullptr;
    ServerInstance *m_currentInstance = nullptr;

    // UI 元件
    SidebarWidget *m_sidebar = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    ServerControlPanel *m_controlPanel = nullptr;
    ServerSettingsPanel *m_settingsPanel = nullptr;
    SaveManagerPanel *m_savePanel = nullptr;
    AboutPanel *m_aboutPanel = nullptr;

    QString m_currentProfileId;
};
