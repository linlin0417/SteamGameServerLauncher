#include "MapPackager.h"
#include "miniz.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QBuffer>
#include <QImageReader>

#include <cstring>

// ---------------------------------------------------------------------------
// MapMetadata 序列化與反序列化
// ---------------------------------------------------------------------------

QJsonObject MapMetadata::toJson() const
{
    QJsonObject obj;
    obj["packageName"]          = packageName;
    obj["originalProspectName"] = originalProspectName;
    obj["notes"]                = notes;
    obj["timestamp"]            = timestamp;
    obj["launcherVersion"]      = launcherVersion;
    obj["hasPreview"]           = hasPreview;
    
    obj["formatVersion"]        = formatVersion;
    obj["gameProfileId"]        = gameProfileId;
    obj["gameDisplayName"]      = gameDisplayName;
    
    QJsonArray patternsArray;
    for (const QString &pattern : saveFilePatterns) {
        patternsArray.append(pattern);
    }
    obj["saveFilePatterns"]     = patternsArray;
    
    return obj;
}

MapMetadata MapMetadata::fromJson(const QJsonObject &obj)
{
    MapMetadata m;
    m.packageName          = obj.value("packageName").toString();
    m.originalProspectName = obj.value("originalProspectName").toString();
    m.notes                = obj.value("notes").toString();
    m.timestamp            = obj.value("timestamp").toString();
    m.launcherVersion      = obj.value("launcherVersion").toString();
    m.hasPreview           = obj.value("hasPreview").toBool(false);
    
    m.formatVersion        = obj.value("formatVersion").toString();
    if (m.formatVersion.isEmpty()) {
        m.gameProfileId = "icarus";
    } else {
        m.gameProfileId = obj.value("gameProfileId").toString();
    }
    m.gameDisplayName      = obj.value("gameDisplayName").toString();
    
    QJsonArray patternsArray = obj.value("saveFilePatterns").toArray();
    for (const QJsonValue &val : patternsArray) {
        m.saveFilePatterns.append(val.toString());
    }
    
    return m;
}

// ---------------------------------------------------------------------------
// exportMap - 將地圖存檔打包為 .IcarusMap
// ---------------------------------------------------------------------------

bool MapPackager::exportMap(const QString &exportPath,
                            const QString &prospectDir,
                            const QString &prospectName,
                            const MapMetadata &metadata,
                            const QString &previewImagePath,
                            QString *errorOut)
{
    // 初始化 ZIP 寫入器
    mz_zip_archive archive;
    memset(&archive, 0, sizeof (archive));

    std::string exportPathUtf8 = exportPath.toUtf8().constData();
    if (!mz_zip_writer_init_file(&archive, exportPathUtf8.c_str(), 0)) {
        if (errorOut) {
            *errorOut = QString("無法建立匯出檔案: %1").arg(exportPath);
        }
        return false;
    }

    // --- 寫入 metadata.json ---
    QJsonDocument metaDoc(metadata.toJson());
    QByteArray metaBytes = metaDoc.toJson(QJsonDocument::Indented);

    if (!mz_zip_writer_add_mem(&archive,
                               "metadata.json",
                               metaBytes.constData(),
                               static_cast<size_t>(metaBytes.size()),
                               MZ_DEFAULT_COMPRESSION)) {
        if (errorOut) {
            *errorOut = QString("寫入 metadata.json 至壓縮檔時失敗");
        }
        mz_zip_writer_end(&archive);
        return false;
    }

    // --- 若有預覽圖片則寫入 preview.png ---
    if (!previewImagePath.isEmpty() && QFileInfo::exists(previewImagePath)) {
        std::string previewUtf8 = previewImagePath.toUtf8().constData();
        if (!mz_zip_writer_add_file(&archive,
                                    "preview.png",
                                    previewUtf8.c_str(),
                                    nullptr,
                                    0,
                                    MZ_DEFAULT_COMPRESSION)) {
            if (errorOut) {
                *errorOut = QString("寫入預覽圖片至壓縮檔時失敗: %1").arg(previewImagePath);
            }
            mz_zip_writer_end(&archive);
            return false;
        }
    }

    // --- 根據 patterns 找出需要打包的主存檔與備份檔 ---
    QDir dir(prospectDir);
    QStringList patterns = metadata.saveFilePatterns;
    if (patterns.isEmpty()) {
        patterns << QStringLiteral("*.json") << QStringLiteral("*.json.backup"); // 舊版相容
    }

    QStringList targetFiles;
    for (const QString &pattern : patterns) {
        if (pattern.startsWith(QStringLiteral("*."))) {
            QString suffix = pattern.mid(1);
            QString exactFileName = prospectName + suffix;
            if (QFileInfo::exists(dir.filePath(exactFileName))) {
                targetFiles.append(exactFileName);
            }
        } else {
            // 如果不是 *. 結尾，直接嘗試加入 (可能支援其他模式)
            if (QFileInfo::exists(dir.filePath(pattern))) {
                targetFiles.append(pattern);
            }
        }
    }

    if (targetFiles.isEmpty()) {
        if (errorOut) {
            *errorOut = QString("找不到來源存檔: %1").arg(prospectName);
        }
        mz_zip_writer_end(&archive);
        return false;
    }

    // --- 寫入目標檔案至 saves 目錄 ---
    for (const QString &fileName : targetFiles) {
        QString savePath = dir.filePath(fileName);
        std::string savePathUtf8 = savePath.toUtf8().constData();
        std::string archiveName  = (QString("saves/") + fileName).toUtf8().constData();

        if (!mz_zip_writer_add_file(&archive,
                                    archiveName.c_str(),
                                    savePathUtf8.c_str(),
                                    nullptr,
                                    0,
                                    MZ_DEFAULT_COMPRESSION)) {
            if (errorOut) {
                *errorOut = QString("寫入存檔至壓縮檔時失敗: %1").arg(savePath);
            }
            mz_zip_writer_end(&archive);
            return false;
        }
    }

    // --- 完成壓縮並關閉 ---
    if (!mz_zip_writer_finalize_archive(&archive)) {
        if (errorOut) {
            *errorOut = QString("無法完成壓縮檔的寫入作業");
        }
        mz_zip_writer_end(&archive);
        return false;
    }

    mz_zip_writer_end(&archive);
    return true;
}

// ---------------------------------------------------------------------------
// readMetadata - 從 .IcarusMap 讀取中繼資料
// ---------------------------------------------------------------------------

MapMetadata MapPackager::readMetadata(const QString &icarusMapPath, bool *ok)
{
    MapMetadata result;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof (archive));

    std::string pathUtf8 = icarusMapPath.toUtf8().constData();
    if (!mz_zip_reader_init_file(&archive, pathUtf8.c_str(), 0)) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    // 嘗試從壓縮檔中提取 metadata.json
    size_t bufSize = 0;
    void *buf = mz_zip_reader_extract_file_to_heap(&archive,
                                                    "metadata.json",
                                                    &bufSize,
                                                    0);
    if (!buf) {
        if (ok) {
            *ok = false;
        }
        mz_zip_reader_end(&archive);
        return result;
    }

    // 解析 JSON
    QByteArray jsonData(static_cast<const char *>(buf), static_cast<int>(bufSize));
    mz_free(buf);
    mz_zip_reader_end(&archive);

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) {
            *ok = false;
        }
        return result;
    }

    result = MapMetadata::fromJson(doc.object());
    if (ok) {
        *ok = true;
    }
    return result;
}

// ---------------------------------------------------------------------------
// readPreview - 從 .IcarusMap 讀取預覽圖片
// ---------------------------------------------------------------------------

QPixmap MapPackager::readPreview(const QString &icarusMapPath)
{
    QPixmap pixmap;

    mz_zip_archive archive;
    memset(&archive, 0, sizeof (archive));

    std::string pathUtf8 = icarusMapPath.toUtf8().constData();
    if (!mz_zip_reader_init_file(&archive, pathUtf8.c_str(), 0)) {
        return pixmap;
    }

    // 嘗試提取 preview.png
    size_t bufSize = 0;
    void *buf = mz_zip_reader_extract_file_to_heap(&archive,
                                                    "preview.png",
                                                    &bufSize,
                                                    0);
    if (!buf) {
        // 壓縮檔中沒有預覽圖片，回傳空的 QPixmap
        mz_zip_reader_end(&archive);
        return pixmap;
    }

    // 從記憶體載入圖片
    QByteArray imageData(static_cast<const char *>(buf), static_cast<int>(bufSize));
    mz_free(buf);
    mz_zip_reader_end(&archive);

    pixmap.loadFromData(imageData);
    return pixmap;
}

// ---------------------------------------------------------------------------
// isLegacyFormat - 檢查是否為 v1.x .IcarusMap 格式
// ---------------------------------------------------------------------------

bool MapPackager::isLegacyFormat(const QString &filePath)
{
    bool ok = false;
    MapMetadata meta = readMetadata(filePath, &ok);
    if (!ok) {
        return false;
    }
    return meta.formatVersion.isEmpty();
}

// ---------------------------------------------------------------------------
// importMap - 將 .IcarusMap 中的存檔解壓至目標目錄
// ---------------------------------------------------------------------------

bool MapPackager::importMap(const QString &icarusMapPath,
                            const QString &targetProspectDir,
                            bool overwrite,
                            QString *errorOut)
{
    // 確保目標目錄存在
    if (!QDir().mkpath(targetProspectDir)) {
        if (errorOut) {
            *errorOut = QString("無法建立目標目錄: %1").arg(targetProspectDir);
        }
        return false;
    }

    mz_zip_archive archive;
    memset(&archive, 0, sizeof (archive));

    std::string pathUtf8 = icarusMapPath.toUtf8().constData();
    if (!mz_zip_reader_init_file(&archive, pathUtf8.c_str(), 0)) {
        if (errorOut) {
            *errorOut = QString("無法開啟 .IcarusMap 檔案: %1").arg(icarusMapPath);
        }
        return false;
    }

    const mz_uint fileCount = mz_zip_reader_get_num_files(&archive);
    const QString savesPrefix = QString("saves/");

    for (mz_uint i = 0; i < fileCount; ++i) {
        // 取得壓縮檔內的檔案名稱
        char entryName[512];
        if (!mz_zip_reader_get_filename(&archive, i, entryName, sizeof (entryName))) {
            if (errorOut) {
                *errorOut = QString("無法讀取壓縮檔中索引 %1 的檔案名稱").arg(i);
            }
            mz_zip_reader_end(&archive);
            return false;
        }

        QString entryStr = QString::fromUtf8(entryName);

        // 只處理 saves/ 目錄下的檔案
        if (!entryStr.startsWith(savesPrefix)) {
            continue;
        }

        // 取得 saves/ 之後的相對檔名
        QString fileName = entryStr.mid(savesPrefix.length());
        if (fileName.isEmpty()) {
            continue;
        }

        QString targetPath = targetProspectDir + '/' + fileName;

        // 檢查是否已存在同名檔案
        if (!overwrite && QFileInfo::exists(targetPath)) {
            if (errorOut) {
                *errorOut = QString("目標檔案已存在且未允許覆寫: %1").arg(targetPath);
            }
            mz_zip_reader_end(&archive);
            return false;
        }

        // 解壓至目標路徑
        std::string targetPathUtf8 = targetPath.toUtf8().constData();
        if (!mz_zip_reader_extract_file_to_file(&archive,
                                                 entryName,
                                                 targetPathUtf8.c_str(),
                                                 0)) {
            if (errorOut) {
                *errorOut = QString("解壓檔案失敗: %1 -> %2").arg(entryStr, targetPath);
            }
            mz_zip_reader_end(&archive);
            return false;
        }
    }

    mz_zip_reader_end(&archive);
    return true;
}
