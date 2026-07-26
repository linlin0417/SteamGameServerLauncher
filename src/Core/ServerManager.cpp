#include "ServerManager.h"
#include "../version.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

ServerManager::ServerManager(QObject *parent)
    : QObject(parent)
{
}

ServerManager::~ServerManager()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(5000);
    }
}

void ServerManager::setServerExecutable(const QString &exePath)
{
    m_serverExe = exePath;
}

bool ServerManager::isRunning() const
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

void ServerManager::startServer(const QStringList &extraArgs)
{
    if (isRunning()) {
        emit logMessage(tr("[Server] Server is already running."));
        return;
    }

    if (m_serverExe.isEmpty() || !QFileInfo::exists(m_serverExe)) {
        emit logMessage(tr("[Server] Server executable not found: %1")
                            .arg(m_serverExe));
        return;
    }

    // Create a new QProcess
    if (m_process) {
        m_process->deleteLater();
    }
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    m_process->setWorkingDirectory(QFileInfo(m_serverExe).absolutePath());

    connect(m_process, &QProcess::started,
            this, &ServerManager::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &ServerManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &ServerManager::onProcessErrorOccurred);
    connect(m_process, &QProcess::readyRead,
            this, &ServerManager::onProcessReadyRead);

    setState(ServerState::Starting);
    emit logMessage(tr("[Server] Starting: %1 %2")
                        .arg(m_serverExe, extraArgs.join(' ')));

    m_process->start(m_serverExe, extraArgs);
}

void ServerManager::stopServer()
{
    if (!isRunning()) {
        emit logMessage(tr("[Server] Server is not running."));
        return;
    }

    setState(ServerState::Stopping);
    emit logMessage(tr("[Server] Stopping server..."));

    // Try graceful close first, then force kill after timeout
    m_process->terminate();

    // On Windows, terminate() sends WM_CLOSE.  If it doesn't work within
    // 10 seconds we force-kill.
    QTimer::singleShot(10000, this, [this]() {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            emit logMessage(tr("[Server] Force-killing server process."));
            m_process->kill();
        }
    });
}

void ServerManager::setState(ServerState s)
{
    if (m_state != s) {
        m_state = s;
        emit stateChanged(m_state);
    }
}

void ServerManager::onProcessStarted()
{
    setState(ServerState::Running);
    emit logMessage(tr("[Server] Server is now running (PID: %1).")
                        .arg(m_process->processId()));
}

void ServerManager::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        emit logMessage(tr("[Server] Server crashed! (exit code: %1)").arg(exitCode));
        setState(ServerState::Stopped);
        emit serverCrashed(exitCode);
    } else {
        emit logMessage(tr("[Server] Server stopped (exit code: %1).").arg(exitCode));
        setState(ServerState::Stopped);
    }
}

void ServerManager::onProcessErrorOccurred(QProcess::ProcessError error)
{
    emit logMessage(tr("[Server] Process error: %1").arg(static_cast<int>(error)));
}

void ServerManager::onProcessReadyRead()
{
    while (m_process && m_process->canReadLine()) {
        const QString line =
            QString::fromLocal8Bit(m_process->readLine()).trimmed();
        if (!line.isEmpty()) {
            emit logMessage(QStringLiteral("[Server] %1").arg(line));
        }
    }
}

// ---------------------------------------------------------------------------
// Settings helpers
// ---------------------------------------------------------------------------

QJsonObject ServerManager::defaultSettings()
{
    return QJsonObject{
        {"serverName",    QString(AppConfig::DefaultServerName)},
        {"password",      QString()},
        {"adminPassword", QString()},
        {"maxPlayers",    AppConfig::DefaultMaxPlayers},
        {"port",          AppConfig::DefaultPort},
        {"queryPort",     AppConfig::DefaultQueryPort},
        {"serverInstallDir", QString()},
        {"serverExePath",    QString()},
        {"additionalArgs",   QString()},
        {"discordWebhookUrl", QString()}
    };
}

QJsonObject ServerManager::loadSettings(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return defaultSettings();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        return defaultSettings();
    }

    // Merge loaded values over defaults so new keys always exist.
    QJsonObject defaults = defaultSettings();
    QJsonObject loaded   = doc.object();
    for (auto it = loaded.constBegin(); it != loaded.constEnd(); ++it) {
        defaults[it.key()] = it.value();
    }
    return defaults;
}

bool ServerManager::saveSettings(const QString &filePath,
                                  const QJsonObject &settings)
{
    // Ensure parent directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(QJsonDocument(settings).toJson(QJsonDocument::Indented));
    return true;
}

QStringList ServerManager::buildLaunchArgs(const QJsonObject &settings)
{
    QStringList args;

    const QString name = settings.value("serverName").toString();
    if (!name.isEmpty())
        args << QStringLiteral("-SteamServerName=%1").arg(name);

    const int port = settings.value("port").toInt(AppConfig::DefaultPort);
    args << QStringLiteral("-Port=%1").arg(port);

    const int qport = settings.value("queryPort").toInt(AppConfig::DefaultQueryPort);
    args << QStringLiteral("-QueryPort=%1").arg(qport);

    const int maxP = settings.value("maxPlayers").toInt(AppConfig::DefaultMaxPlayers);
    args << QStringLiteral("-MaxPlayers=%1").arg(maxP);

    // Note: Icarus no longer supports setting passwords via command line arguments.
    // The passwords must be written to ServerSettings.ini instead.

    // Additional free-form arguments
    const QString extra = settings.value("additionalArgs").toString().trimmed();
    if (!extra.isEmpty()) {
        args << extra.split(' ', Qt::SkipEmptyParts);
    }

    return args;
}

void ServerManager::applyIcarusSettings(const QJsonObject &settings)
{
    const QString installDir = settings.value("serverInstallDir").toString();
    if (installDir.isEmpty())
        return;

    const QString iniPath = installDir + "/Icarus/Saved/Config/WindowsServer/ServerSettings.ini";
    QFileInfo fi(iniPath);
    if (!fi.absoluteDir().exists()) {
        fi.absoluteDir().mkpath(".");
    }

    QStringList lines;
    QFile file(iniPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            lines.append(in.readLine());
        }
        file.close();
    }

    const QString section = "[/Script/Icarus.DedicatedServerSettings]";
    const QString pw = settings.value("password").toString();
    const QString adminPw = settings.value("adminPassword").toString();

    int sectionIdx = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].trimmed() == section) {
            sectionIdx = i;
            break;
        }
    }

    if (sectionIdx == -1) {
        // Section not found, append it
        if (!lines.isEmpty() && !lines.last().isEmpty()) {
            lines.append("");
        }
        lines.append(section);
        sectionIdx = lines.size() - 1;
    }

    // Update or insert keys
    auto updateKey = [&](const QString &key, const QString &val) {
        bool found = false;
        // Search inside the section until the next section or end of file
        for (int i = sectionIdx + 1; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.startsWith('[')) {
                break; // Reached next section
            }
            if (line.startsWith(key + "=")) {
                lines[i] = key + "=" + val;
                found = true;
                break;
            }
        }
        if (!found) {
            lines.insert(sectionIdx + 1, key + "=" + val);
        }
    };

    updateKey("JoinPassword", pw);
    updateKey("AdminPassword", adminPw);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&file);
        for (const QString &line : lines) {
            out << line << "\n";
        }
        file.close();
    }
}
