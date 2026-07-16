/// Updater.exe — Independent update program for SteamGameServerLauncher.
///
/// Usage:
///   Updater.exe --zip <path> --target <dir> --exe <launcher.exe> --pid <pid>
///
/// 1. Wait for the launcher process (--pid) to exit.
/// 2. Extract the zip to the target directory.
/// 3. Re-launch the launcher executable.
/// 4. Clean up and exit.

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <QTextStream>

#include <cstdio>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static QTextStream sOut(stdout);

static void log(const QString &msg)
{
    sOut << msg << "\n";
    sOut.flush();
}

/// Wait for a process to exit (Windows-specific).
static bool waitForProcessExit(qint64 pid, int timeoutMs = 30000)
{
#ifdef Q_OS_WIN
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (hProcess == NULL) {
        // Process might have already exited.
        return true;
    }
    DWORD result = WaitForSingleObject(hProcess, static_cast<DWORD>(timeoutMs));
    CloseHandle(hProcess);
    return result == WAIT_OBJECT_0;
#else
    // Simple poll fallback for non-Windows
    Q_UNUSED(pid)
    QThread::msleep(timeoutMs > 3000 ? 3000 : timeoutMs);
    return true;
#endif
}

/// Extract a zip file using PowerShell.
static bool extractZip(const QString &zipPath, const QString &destDir)
{
    QProcess ps;
    const QString cmd = QStringLiteral(
        "Expand-Archive -Path '%1' -DestinationPath '%2' -Force")
        .arg(QDir::toNativeSeparators(zipPath),
             QDir::toNativeSeparators(destDir));

    ps.start("powershell",
             QStringList() << "-NoProfile" << "-Command" << cmd);

    if (!ps.waitForFinished(120000)) {  // 2 min timeout
        log(QStringLiteral("ERROR: Extraction timed out."));
        return false;
    }
    return ps.exitCode() == 0;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("SteamGameServerLauncher Updater");

    QCommandLineParser parser;
    parser.setApplicationDescription("Updates the SteamGameServerLauncher.");
    parser.addHelpOption();

    QCommandLineOption zipOpt("zip", "Path to the update zip.", "path");
    QCommandLineOption targetOpt("target", "Target installation directory.", "dir");
    QCommandLineOption exeOpt("exe", "Launcher executable to restart.", "path");
    QCommandLineOption pidOpt("pid", "PID of the launcher to wait for.", "pid");

    parser.addOption(zipOpt);
    parser.addOption(targetOpt);
    parser.addOption(exeOpt);
    parser.addOption(pidOpt);
    parser.process(app);

    const QString zipPath   = parser.value(zipOpt);
    const QString targetDir = parser.value(targetOpt);
    const QString exePath   = parser.value(exeOpt);
    const qint64  pid       = parser.value(pidOpt).toLongLong();

    if (zipPath.isEmpty() || targetDir.isEmpty() || exePath.isEmpty()) {
        log("ERROR: Missing required arguments. Use --help for usage.");
        return 1;
    }

    log(QStringLiteral("=== SteamGameServerLauncher Updater ==="));
    log(QStringLiteral("  Zip:    %1").arg(zipPath));
    log(QStringLiteral("  Target: %1").arg(targetDir));
    log(QStringLiteral("  Exe:    %1").arg(exePath));
    log(QStringLiteral("  PID:    %1").arg(pid));

    // Step 1: Wait for launcher to exit
    if (pid > 0) {
        log("Waiting for launcher to exit...");
        if (!waitForProcessExit(pid)) {
            log("WARNING: Timed out waiting for launcher. Proceeding anyway.");
        }
        // Extra safety delay
        QThread::msleep(1000);
    } else {
        QThread::msleep(2000);
    }

    // Step 2: Extract update
    log("Extracting update...");
    if (!extractZip(zipPath, targetDir)) {
        log("ERROR: Failed to extract update package.");
        return 2;
    }
    log("Extraction complete.");

    // Step 3: Clean up zip
    QFile::remove(zipPath);
    log("Cleaned up update package.");

    // Step 4: Restart launcher
    log(QStringLiteral("Restarting: %1").arg(exePath));
    bool started = QProcess::startDetached(exePath, {}, targetDir);
    if (!started) {
        log("ERROR: Failed to restart launcher.");
        return 3;
    }

    log("Update complete. Exiting updater.");
    return 0;
}
