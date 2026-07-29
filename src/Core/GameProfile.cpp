#include "GameProfile.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QProcess>
#include <QJsonValue>
#include <QJsonArray>
#include <QVariant>
#include <QtGlobal>

QJsonObject GameProfile::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("id")] = id;
    obj[QStringLiteral("displayName")] = displayName;

    QString sourceStr;
    if (sourceType == SteamCMD) {
        sourceStr = QStringLiteral("steamcmd");
    } else if (sourceType == CustomScript) {
        sourceStr = QStringLiteral("custom_script");
    } else {
        sourceStr = QStringLiteral("manual");
    }
    obj[QStringLiteral("sourceType")] = sourceStr;
    obj[QStringLiteral("steamAppId")] = steamAppId;
    obj[QStringLiteral("installScript")] = installScript;
    obj[QStringLiteral("updateScript")] = updateScript;

    obj[QStringLiteral("exeRelativePath")] = exeRelativePath;
    obj[QStringLiteral("launchArgsTemplate")] = launchArgsTemplate;
    obj[QStringLiteral("isJavaApp")] = isJavaApp;

    QString configFormatStr;
    if (configFormat == INI) {
        configFormatStr = QStringLiteral("ini");
    } else if (configFormat == Properties) {
        configFormatStr = QStringLiteral("properties");
    } else if (configFormat == JSON_Config) {
        configFormatStr = QStringLiteral("json");
    } else {
        configFormatStr = QStringLiteral("none");
    }
    obj[QStringLiteral("configFormat")] = configFormatStr;
    obj[QStringLiteral("configFilePath")] = configFilePath;
    obj[QStringLiteral("configSection")] = configSection;
    obj[QStringLiteral("configMappings")] = configMappings;

    obj[QStringLiteral("savesRelativePath")] = savesRelativePath;
    obj[QStringLiteral("saveFilePatterns")] = QJsonArray::fromStringList(saveFilePatterns);

    obj[QStringLiteral("defaultPort")] = defaultPort;
    obj[QStringLiteral("defaultQueryPort")] = defaultQueryPort;
    obj[QStringLiteral("defaultMaxPlayers")] = defaultMaxPlayers;
    obj[QStringLiteral("extraDefaults")] = extraDefaults;

    return obj;
}

GameProfile GameProfile::fromJson(const QJsonObject &obj)
{
    GameProfile profile;
    
    profile.id = obj.value(QStringLiteral("id")).toString();
    profile.displayName = obj.value(QStringLiteral("displayName")).toString();

    QString sourceStr = obj.value(QStringLiteral("sourceType")).toString(QStringLiteral("manual"));
    if (sourceStr == QStringLiteral("steamcmd")) {
        profile.sourceType = SteamCMD;
    } else if (sourceStr == QStringLiteral("custom_script")) {
        profile.sourceType = CustomScript;
    } else {
        profile.sourceType = ManualOnly;
    }

    profile.steamAppId = obj.value(QStringLiteral("steamAppId")).toString();
    profile.installScript = obj.value(QStringLiteral("installScript")).toString();
    profile.updateScript = obj.value(QStringLiteral("updateScript")).toString();

    profile.exeRelativePath = obj.value(QStringLiteral("exeRelativePath")).toString();
    profile.launchArgsTemplate = obj.value(QStringLiteral("launchArgsTemplate")).toString();
    profile.isJavaApp = obj.value(QStringLiteral("isJavaApp")).toBool(false);

    QString configFormatStr = obj.value(QStringLiteral("configFormat")).toString(QStringLiteral("none"));
    if (configFormatStr == QStringLiteral("ini")) {
        profile.configFormat = INI;
    } else if (configFormatStr == QStringLiteral("properties")) {
        profile.configFormat = Properties;
    } else if (configFormatStr == QStringLiteral("json")) {
        profile.configFormat = JSON_Config;
    } else {
        profile.configFormat = None;
    }

    profile.configFilePath = obj.value(QStringLiteral("configFilePath")).toString();
    profile.configSection = obj.value(QStringLiteral("configSection")).toString();
    profile.configMappings = obj.value(QStringLiteral("configMappings")).toObject();

    profile.savesRelativePath = obj.value(QStringLiteral("savesRelativePath")).toString();
    
    QJsonArray patternsArray = obj.value(QStringLiteral("saveFilePatterns")).toArray();
    for (int i = 0; i < patternsArray.size(); ++i) {
        profile.saveFilePatterns.append(patternsArray.at(i).toString());
    }

    profile.defaultPort = obj.value(QStringLiteral("defaultPort")).toInt(0);
    profile.defaultQueryPort = obj.value(QStringLiteral("defaultQueryPort")).toInt(0);
    profile.defaultMaxPlayers = obj.value(QStringLiteral("defaultMaxPlayers")).toInt(0);
    profile.extraDefaults = obj.value(QStringLiteral("extraDefaults")).toObject();

    return profile;
}

QString GameProfile::expandTemplate(const QString &tpl, const QJsonObject &vars)
{
    QString result = tpl;
    QRegularExpression re(QStringLiteral("\\{(\\w+)\\}"));
    QRegularExpressionMatchIterator i = re.globalMatch(tpl);
    
    // 收集所有匹配項目，從後往前替換以避免影響未處理的索引
    QList<QRegularExpressionMatch> matches;
    while (i.hasNext()) {
        matches.append(i.next());
    }
    
    for (int j = matches.size() - 1; j >= 0; --j) {
        QRegularExpressionMatch match = matches.at(j);
        QString varName = match.captured(1);
        if (vars.contains(varName)) {
            QJsonValue val = vars.value(varName);
            QString replacement;
            if (val.isString()) {
                replacement = val.toString();
            } else if (val.isDouble()) {
                replacement = val.toVariant().toString();
            } else if (val.isBool()) {
                replacement = val.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            } else {
                continue;
            }
            result.replace(match.capturedStart(), match.capturedLength(), replacement);
        }
    }
    return result;
}

QStringList GameProfile::buildLaunchArgs(const QJsonObject &settings) const
{
    QString expanded = expandTemplate(launchArgsTemplate, settings);
    QStringList args;
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    args = QProcess::splitCommand(expanded);
#else
    args = expanded.split(QLatin1Char(' '), Qt::SkipEmptyParts);
#endif

    if (isJavaApp) {
        args.prepend(exeRelativePath);
        args.prepend(QStringLiteral("-jar"));
        args.prepend(QStringLiteral("java"));
    }
    
    return args;
}

QStringList GameProfile::extractVariableNames() const
{
    QStringList vars;
    QRegularExpression re(QStringLiteral("\\{(\\w+)\\}"));
    
    // 提取 launchArgsTemplate 中的變數
    QRegularExpressionMatchIterator it1 = re.globalMatch(launchArgsTemplate);
    while (it1.hasNext()) {
        QRegularExpressionMatch match = it1.next();
        vars.append(match.captured(1));
    }
    
    // 提取 configMappings 中的變數
    for (const QString &key : configMappings.keys()) {
        QJsonValue val = configMappings.value(key);
        if (val.isString()) {
            QRegularExpressionMatchIterator it2 = re.globalMatch(val.toString());
            while (it2.hasNext()) {
                QRegularExpressionMatch match = it2.next();
                vars.append(match.captured(1));
            }
        }
    }
    
    vars.removeDuplicates();
    return vars;
}

bool GameProfile::isBuiltin() const
{
    return builtinProfileIds().contains(id);
}

bool GameProfile::isValid() const
{
    return !id.isEmpty() && !displayName.isEmpty();
}

QStringList GameProfile::builtinProfileIds()
{
    return {
        QStringLiteral("icarus"),
        QStringLiteral("palworld"),
        QStringLiteral("rust"),
        QStringLiteral("minecraft_paper")
    };
}
