#pragma once

#include <QString>

/// 版本後綴類型列舉
/// 優先順序（數字越大，同主副修版本時越「新」／「正式」）：
/// Stable=None(5) > Preview(4) > Patch(3) > HotfixLike(2) > Dev(1) > Unknown(0)
/// 解釋：正式版（無後綴）是同 X.Y.Z 下最終完成狀態，因此優先等級最高。
///       hotfix 是在正式版「發布前」的補丁，屬於半成品，所以正式版 > hotfix。
enum class VersionSuffixType {
    Unknown    = 0,  // 無法辨識的後綴
    Dev        = 1,  // -devY（工程測試版）
    HotfixLike = 2,  // -hotfixY / -tmpY / -quickfixY（零食補釘版）
    Patch      = 3,  // -patchY（特供補丁版，保留）
    Preview    = 4,  // -preview（預覽版）
    None       = 5,  // 無後綴（正式版，同 X.Y.Z 下最高優先）
};

/// 版本資訊結構
struct AppVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;

    VersionSuffixType suffixType = VersionSuffixType::None;
    QString           suffixRaw;     // 後綴的完整原始字串，例如 'hotfix1'
    int               suffixSeq = 0; // 流水號，例如 -hotfix3 的 3

    bool isValid = false;

    /// 是否為穩定的正式版（無後綴）
    bool isStable() const { return isValid && suffixType == VersionSuffixType::None; }

    /// 是否為熱修復類型（hotfix / tmp / quickfix）
    bool isHotfixLike() const { return isValid && suffixType == VersionSuffixType::HotfixLike; }
};

/// 版本解析與比對核心工具類別
class VersionParser
{
public:
    /// 從字串解析版本，支援帶 'v' 前綴與各類後綴
    /// 範例輸入：'2.0.20'、'v2.0.20-hotfix1'、'2.0.20-dev3'、'2.0.20-preview'
    static AppVersion parse(const QString &versionString);

    /// 判斷 remote 是否比 local 更新（用於一般自動更新流程）
    static bool isNewerThan(const AppVersion &remote, const AppVersion &local);

    /// 判斷 remote 是否為 local 的熱修復版本（Bootstrap 熱修復強制更新使用）
    /// 條件：Major 與 Minor 相符、suffixType 為 HotfixLike、流水號 > local 同類型流水號
    static bool isHotfixFor(const AppVersion &remote, const AppVersion &local);

    /// 將 AppVersion 轉回人類可讀的字串
    static QString toString(const AppVersion &ver);

    /// 取得版本分類的中文說明字串（用於 UI 顯示）
    static QString typeLabel(const AppVersion &ver);
};
