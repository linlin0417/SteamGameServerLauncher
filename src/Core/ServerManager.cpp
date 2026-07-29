#include "ServerManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>
#include <QTextStream>

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
                        .arg(m_serverExe, extraArgs.join(QLatin1Char(' '))));

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

QJsonObject ServerManager::loadSettings(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonObject();
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) {
        return QJsonObject();
    }
    return doc.object();
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

bool ServerManager::applyGameConfig(const QString &configFormat,
                                    const QString &configFilePath,
                                    const QString &configSection,
                                    const QJsonObject &mappings)
{
    if (configFormat == QStringLiteral("none")) {
        return true;
    }

    QFileInfo fi(configFilePath);
    if (!fi.absoluteDir().exists()) {
        fi.absoluteDir().mkpath(QStringLiteral("."));
    }

    if (configFormat == QStringLiteral("ini")) {
        QStringList lines;
        QFile file(configFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                lines.append(in.readLine());
            }
            file.close();
        }

        int sectionIdx = -1;
        for (int i = 0; i < lines.size(); ++i) {
            if (lines[i].trimmed() == configSection) {
                sectionIdx = i;
                break;
            }
        }

        if (sectionIdx == -1) {
            if (!lines.isEmpty() && !lines.last().isEmpty()) {
                lines.append(QStringLiteral(""));
            }
            lines.append(configSection);
            sectionIdx = lines.size() - 1;
        }

        auto updateKey = [&](const QString &key, const QString &val) {
            bool found = false;
            for (int i = sectionIdx + 1; i < lines.size(); ++i) {
                QString line = lines[i].trimmed();
                if (line.startsWith(QLatin1Char('['))) {
                    break;
                }
                if (line.startsWith(key + QLatin1Char('='))) {
                    lines[i] = key + QLatin1Char('=') + val;
                    found = true;
                    break;
                }
            }
            if (!found) {
                lines.insert(sectionIdx + 1, key + QLatin1Char('=') + val);
            }
        };

        for (auto it = mappings.constBegin(); it != mappings.constEnd(); ++it) {
            updateKey(it.key(), it.value().toString());
        }

        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            for (const QString &line : lines) {
                out << line << QLatin1Char('\n');
            }
            file.close();
            return true;
        }
        return false;
    } else if (configFormat == QStringLiteral("properties")) {
        QStringList lines;
        QFile file(configFilePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                lines.append(in.readLine());
            }
            file.close();
        }

        for (auto it = mappings.constBegin(); it != mappings.constEnd(); ++it) {
            QString key = it.key();
            QString val = it.value().toString();
            bool found = false;
            for (int i = 0; i < lines.size(); ++i) {
                QString line = lines[i].trimmed();
                if (line.startsWith(key + QLatin1Char('='))) {
                    lines[i] = key + QLatin1Char('=') + val;
                    found = true;
                    break;
                }
            }
            if (!found) {
                lines.append(key + QLatin1Char('=') + val);
            }
        }

        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&file);
            for (const QString &line : lines) {
                out << line << QLatin1Char('\n');
            }
            file.close();
            return true;
        }
        return false;
    } else if (configFormat == QStringLiteral("json")) {
        QJsonObject jsonObj;
        QFile file(configFilePath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            if (doc.isObject()) {
                jsonObj = doc.object();
            }
            file.close();
        }

        for (auto it = mappings.constBegin(); it != mappings.constEnd(); ++it) {
            jsonObj[it.key()] = it.value();
        }

        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(jsonObj).toJson(QJsonDocument::Indented));
            file.close();
            return true;
        }
        return false;
    }

    return false;
}
