#include "VersionParser.h"

#include <QRegularExpression>

AppVersion VersionParser::parse(const QString &versionString)
{
    AppVersion ver;
    if (versionString.isEmpty()) {
        return ver;
    }

    // 去除前綴的 'v' 或 'V'
    QString input = versionString.trimmed();
    if (input.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) {
        input = input.mid(1);
    }

    // 正規表達式：比對 X.Y.Z 或 X.Y.Z-suffix（suffix 可含數字）
    static const QRegularExpression re(
        QStringLiteral(R"(^(\d+)\.(\d+)\.(\d+)(?:-([a-zA-Z]+)(\d*))?$)")
    );

    const QRegularExpressionMatch m = re.match(input);
    if (!m.hasMatch()) {
        return ver;
    }

    ver.major = m.captured(1).toInt();
    ver.minor = m.captured(2).toInt();
    ver.patch = m.captured(3).toInt();
    ver.isValid = true;

    // 解析後綴
    const QString suffixWord = m.captured(4).toLower(); // 例如 'hotfix'、'dev'、'preview'
    const QString suffixNum  = m.captured(5);           // 例如 '1'、'3'（可為空）

    if (suffixWord.isEmpty()) {
        ver.suffixType = VersionSuffixType::None;
        ver.suffixSeq  = 0;
    } else {
        ver.suffixRaw = suffixWord + suffixNum;
        ver.suffixSeq = suffixNum.isEmpty() ? 0 : suffixNum.toInt();

        if (suffixWord == QLatin1String("hotfix") ||
            suffixWord == QLatin1String("quickfix") ||
            suffixWord == QLatin1String("tmp")) {
            ver.suffixType = VersionSuffixType::HotfixLike;
        } else if (suffixWord == QLatin1String("dev")) {
            ver.suffixType = VersionSuffixType::Dev;
        } else if (suffixWord == QLatin1String("patch")) {
            ver.suffixType = VersionSuffixType::Patch;
        } else if (suffixWord == QLatin1String("preview") ||
                   suffixWord == QLatin1String("beta") ||
                   suffixWord == QLatin1String("rc")) {
            ver.suffixType = VersionSuffixType::Preview;
        } else {
            ver.suffixType = VersionSuffixType::Unknown;
        }
    }

    return ver;
}

bool VersionParser::isNewerThan(const AppVersion &remote, const AppVersion &local)
{
    if (!remote.isValid || !local.isValid) {
        return false;
    }

    // 先比對主版本、次版本、修訂號
    if (remote.major != local.major) { return remote.major > local.major; }
    if (remote.minor != local.minor) { return remote.minor > local.minor; }
    if (remote.patch != local.patch) { return remote.patch > local.patch; }

    // 主副修版本相同時，比對後綴優先等級
    const int remotePriority = static_cast<int>(remote.suffixType);
    const int localPriority  = static_cast<int>(local.suffixType);

    if (remotePriority != localPriority) {
        return remotePriority > localPriority;
    }

    // 後綴類型相同時，比對流水號
    return remote.suffixSeq > local.suffixSeq;
}

bool VersionParser::isHotfixFor(const AppVersion &remote, const AppVersion &local)
{
    if (!remote.isValid || !local.isValid) {
        return false;
    }

    // 必須是 HotfixLike 類型
    if (remote.suffixType != VersionSuffixType::HotfixLike) {
        return false;
    }

    // Major 與 Minor 必須完全相符
    if (remote.major != local.major || remote.minor != local.minor) {
        return false;
    }

    // Patch 必須相同或遠端更高（相同 patch 下的 hotfix，或 patch 修正後的 hotfix）
    if (remote.patch < local.patch) {
        return false;
    }

    // 若 local 也是同類型 hotfix，則遠端的流水號必須更大
    if (local.suffixType == VersionSuffixType::HotfixLike &&
        remote.patch == local.patch) {
        return remote.suffixSeq > local.suffixSeq;
    }

    return true;
}

QString VersionParser::toString(const AppVersion &ver)
{
    if (!ver.isValid) {
        return QStringLiteral("invalid");
    }

    QString base = QStringLiteral("%1.%2.%3")
                       .arg(ver.major).arg(ver.minor).arg(ver.patch);

    if (ver.suffixType != VersionSuffixType::None) {
        base += QLatin1Char('-') + ver.suffixRaw;
    }

    return base;
}

QString VersionParser::typeLabel(const AppVersion &ver)
{
    if (!ver.isValid) {
        return QStringLiteral("無效版本");
    }

    switch (ver.suffixType) {
    case VersionSuffixType::None:
        return QStringLiteral("正式版");
    case VersionSuffixType::Preview:
        return QStringLiteral("預覽版");
    case VersionSuffixType::HotfixLike:
        return QStringLiteral("零食補釘版");
    case VersionSuffixType::Dev:
        return QStringLiteral("工程測試版");
    case VersionSuffixType::Patch:
        return QStringLiteral("特供補丁版");
    case VersionSuffixType::Unknown:
    default:
        return QStringLiteral("未知類型");
    }
}
