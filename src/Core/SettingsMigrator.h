#pragma once

#include <QString>

/// 負責 v1.x -> v2.0.0 的設定檔自動遷移。
/// 偵測舊版 launcher_settings.json，將其轉換為 v2.0.0 的實例設定格式。
class SettingsMigrator
{
public:
    /// 檢查是否需要遷移（舊版設定檔存在且尚未遷移）
    static bool needsMigration(const QString &dataRootDir);

    /// 執行遷移
    /// @param dataRootDir GameData 目錄路徑
    /// @return 遷移是否成功
    static bool migrate(const QString &dataRootDir);

private:
    SettingsMigrator() = default;
};
