#include "BootstrapWindow.h"

#include <QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SteamGameServerLauncher Bootstrap"));

    // 確保工作目錄為執行檔所在目錄
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    bool fastLaunch = false;
    QString manualZipPath;

    // 解析命令列參數
    // -fastL           : 略過熱修復檢查直接啟動
    // --install <path> : 指定手動安裝包路徑（.sgsl_update 或 .zip）
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args[i] == QStringLiteral("-fastL")) {
            fastLaunch = true;
        } else if (args[i] == QStringLiteral("--install") && i + 1 < args.size()) {
            manualZipPath = args[++i];
        }
    }

    BootstrapWindow window(fastLaunch, manualZipPath);
    window.show();
    window.start();

    return app.exec();
}