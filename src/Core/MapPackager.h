#pragma once

#include <QString>
#include <QPixmap>
#include <QJsonObject>

/// .IcarusMap 地圖包的中繼資料結構。
struct MapMetadata {
    QString packageName;          // 地圖包顯示名稱
    QString originalProspectName; // 原始存檔名（不含副檔名）
    QString notes;                // 使用者自訂備註
    QString timestamp;            // 匯出時間戳（ISO 8601 格式）
    QString launcherVersion;      // 匯出時的啟動器版本
    bool    hasPreview = false;   // 是否包含預覽圖片

    QJsonObject toJson() const;
    static MapMetadata fromJson(const QJsonObject &obj);
};

/// 提供 .IcarusMap 專有格式地圖包的打包與解包功能。
/// 此格式本質上是 ZIP 壓縮檔，內含遊戲存檔與額外中繼資料。
class MapPackager
{
public:
    /// 將指定的地圖存檔與中繼資料打包為 .IcarusMap 檔案。
    /// @param exportPath      匯出的目標檔案路徑（.IcarusMap）
    /// @param prospectDir     伺服器的 Prospects 目錄路徑
    /// @param prospectName    要匯出的存檔名稱（不含副檔名）
    /// @param metadata        中繼資料
    /// @param previewImagePath 預覽圖片路徑（空字串表示不附帶圖片）
    /// @param errorOut        錯誤訊息輸出（可為 nullptr）
    /// @return 是否成功
    static bool exportMap(const QString &exportPath,
                          const QString &prospectDir,
                          const QString &prospectName,
                          const MapMetadata &metadata,
                          const QString &previewImagePath,
                          QString *errorOut = nullptr);

    /// 從 .IcarusMap 檔案中讀取中繼資料（不解壓存檔）。
    static MapMetadata readMetadata(const QString &icarusMapPath,
                                    bool *ok = nullptr);

    /// 從 .IcarusMap 檔案中讀取預覽圖片（不解壓存檔）。
    /// 若無預覽圖片，回傳空的 QPixmap。
    static QPixmap readPreview(const QString &icarusMapPath);

    /// 將 .IcarusMap 中的存檔解壓至目標目錄。
    /// @param icarusMapPath    .IcarusMap 檔案路徑
    /// @param targetProspectDir 目標 Prospects 目錄路徑
    /// @param overwrite        是否覆寫同名檔案
    /// @param errorOut         錯誤訊息輸出（可為 nullptr）
    /// @return 是否成功
    static bool importMap(const QString &icarusMapPath,
                          const QString &targetProspectDir,
                          bool overwrite,
                          QString *errorOut = nullptr);

private:
    MapPackager() = delete;
};
