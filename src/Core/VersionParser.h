#pragma once

#include <QString>

/// 版本後綴類型列舉
/// 優先順序（數字越大，同主副修版本時越「新」）：
/// Patch(5) > HotfixLike(4) > None(3, 正式版) > Preview(2) > Dev(1) > Unknown(0)
/// 解釋：
///   - 預覽版(Preview)與開發版(Dev) 屬於正式版發布「前」的測試版本，故小於 None。
///   - 熱修復(Hotfix/tmp)與補丁(Patch) 屬於正式版發布「後」的緊急修復，故大於 None。
enum class VersionSuffixType {
    Unknown    = 0,  // 無法辨識的後綴
    Dev        = 1,  // -devY（工程測試版，發布前）
    Preview    = 2,  // -preview（預覽版，發布前）
    None       = 3,  // 無後綴（正式版基準）
    HotfixLike = 4,  // -hotfixY / -tmpY / -quickfixY（零食補釘版，發布後）
    Patch      = 5,  // -patchY（特供補丁版，發布後）
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
