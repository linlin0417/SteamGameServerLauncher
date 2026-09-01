#include "LogColorizer.h"
#include <QStringList>

QString LogColorizer::formatToHtml(const QString &rawLog)
{
    // 如果包含 ANSI Escape Sequences
    bool hasAnsi = rawLog.contains(QLatin1String("\x1b[")) || rawLog.contains(QLatin1String("\033["));
    
    if (!hasAnsi) {
        // 機制 A：關鍵字整行上色
        QString lowerLog = rawLog.toLower();
        QString color;
        
        if (lowerLog.contains(QLatin1String("error")) || lowerLog.contains(QLatin1String("failed")) ||
            lowerLog.contains(QLatin1String("fail")) || lowerLog.contains(QLatin1String("crash")) ||
            lowerLog.contains(QLatin1String("exception")) || lowerLog.contains(QLatin1String("fatal")) ||
            lowerLog.contains(QLatin1String("[fail]"))) {
            color = QLatin1String("#ff6b6b"); // 紅色
        } else if (lowerLog.contains(QLatin1String("warn")) || lowerLog.contains(QLatin1String("warning"))) {
            color = QLatin1String("#f39c12"); // 黃色
        } else if (lowerLog.contains(QLatin1String("success")) || lowerLog.contains(QLatin1String("done")) ||
                   lowerLog.contains(QLatin1String("ready")) || lowerLog.contains(QLatin1String("started")) ||
                   lowerLog.contains(QLatin1String("[ok]"))) {
            color = QLatin1String("#2ecc71"); // 綠色
        }

        if (!color.isEmpty()) {
            return QStringLiteral("<span style=\"color: %1;\">%2</span>").arg(color, rawLog.toHtmlEscaped());
        }
        
        // 若無關鍵字，直接回傳跳脫後的純文字
        return rawLog.toHtmlEscaped();
    }

    // 機制 B：ANSI 色碼解析
    QString result;
    int i = 0;
    int len = rawLog.length();
    QString currentColor;
    bool inSpan = false;

    while (i < len) {
        if (rawLog[i] == QLatin1Char('\x1b') || rawLog[i] == QLatin1Char('\033')) {
            if (i + 1 < len && rawLog[i + 1] == QLatin1Char('[')) {
                int j = i + 2;
                QString codeStr;
                while (j < len && (rawLog[j].isDigit() || rawLog[j] == QLatin1Char(';'))) {
                    codeStr += rawLog[j];
                    j++;
                }
                if (j < len && rawLog[j] == QLatin1Char('m')) {
                    // 解析色碼
                    QStringList codes = codeStr.split(QLatin1Char(';'));
                    for (const QString &c : codes) {
                        int code = c.toInt();
                        if (code == 0) {
                            if (inSpan) {
                                result += QLatin1String("</span>");
                                inSpan = false;
                            }
                            currentColor = QLatin1String("");
                        } else if (code >= 30 && code <= 37) {
                            if (inSpan) {
                                result += QLatin1String("</span>");
                                inSpan = false;
                            }
                            switch (code) {
                                case 30: currentColor = QLatin1String("black"); break;
                                case 31: currentColor = QLatin1String("#ff6b6b"); break; // Red
                                case 32: currentColor = QLatin1String("#2ecc71"); break; // Green
                                case 33: currentColor = QLatin1String("#f39c12"); break; // Yellow
                                case 34: currentColor = QLatin1String("#3498db"); break; // Blue
                                case 35: currentColor = QLatin1String("#9b59b6"); break; // Magenta
                                case 36: currentColor = QLatin1String("#1abc9c"); break; // Cyan
                                case 37: currentColor = QLatin1String("white"); break;
                            }
                        }
                    }
                    if (!currentColor.isEmpty() && !inSpan) {
                        result += QStringLiteral("<span style=\"color: %1;\">").arg(currentColor);
                        inSpan = true;
                    }
                    i = j + 1;
                    continue;
                }
            }
        }
        
        // 手動跳脫 HTML 字元
        QChar ch = rawLog[i];
        if (ch == QLatin1Char('<')) {
            result += QLatin1String("&lt;");
        } else if (ch == QLatin1Char('>')) {
            result += QLatin1String("&gt;");
        } else if (ch == QLatin1Char('&')) {
            result += QLatin1String("&amp;");
        } else if (ch == QLatin1Char('"')) {
            result += QLatin1String("&quot;");
        } else {
            result += ch;
        }
        
        i++;
    }
    
    if (inSpan) {
        result += QLatin1String("</span>");
    }
    
    return result;
}
