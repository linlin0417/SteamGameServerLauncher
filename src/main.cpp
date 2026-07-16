#include <QApplication>
#include <QDir>
#include "version.h"
#include "UI/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(AppConfig::AppName);
    app.setApplicationVersion(APP_VERSION);
    app.setOrganizationName(AppConfig::OrganizationName);

    // Ensure working directory is the executable's directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow window;
    window.show();

    return app.exec();
}
