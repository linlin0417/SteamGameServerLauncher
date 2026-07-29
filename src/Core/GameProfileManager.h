#pragma once

#include <QObject>
#include <QList>
#include <QString>

struct GameProfile;

/// 管理所有遊戲設定檔的生命週期。
/// 從 Qt Resource 載入內建設定檔，從使用者目錄載入自訂設定檔。
class GameProfileManager : public QObject
{
    Q_OBJECT

public:
    explicit GameProfileManager(const QString &userProfilesDir, QObject *parent = nullptr);

    /// 重新載入所有設定檔（內建 + 使用者自訂）
    void reload();

    /// 取得所有設定檔（內建優先，然後自訂）
    QList<GameProfile> allProfiles() const;

    /// 根據 ID 取得設定檔，找不到返回空的 GameProfile
    GameProfile profileById(const QString &id) const;

    /// 儲存設定檔（新增或覆寫）
    /// 內建設定檔會覆寫到使用者目錄
    bool saveProfile(const GameProfile &profile);

    /// 刪除自訂設定檔（內建設定檔不可刪除）
    bool removeProfile(const QString &id);

    /// 檢查設定檔是否存在
    bool hasProfile(const QString &id) const;

    /// 產生一個唯一的自訂設定檔 ID
    QString generateUniqueId() const;

signals:
    /// 設定檔清單變更時發送
    void profilesChanged();

private:
    /// 從 Qt Resource 載入內建設定檔
    void loadBuiltinProfiles();

    /// 從使用者目錄載入自訂/覆寫的設定檔
    void loadUserProfiles();

    QString m_userProfilesDir;
    QList<GameProfile> m_profiles;
};
