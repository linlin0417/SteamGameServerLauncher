#include "BootstrapWindow.h"
#include "../src/Core/VersionParser.h"
#include "../src/version.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QProgressBar>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <functional>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

BootstrapWindow::BootstrapWindow(bool fastLaunch, const QString &manualZipPath, QWidget *parent)
    : QWidget(parent)
    , m_fastLaunch(fastLaunch)
    , m_manualZipPath(manualZipPath)
    , m_network(new QNetworkAccessManager(this))
{
    setWindowTitle(QStringLiteral("SteamGameServerLauncher - 啟動自檢"));
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
    setFixedSize(480, 110);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 18, 20, 18);
    root->setSpacing(10);

    m_statusLabel = new QLabel(QStringLiteral("正在初始化..."), this);
    m_statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(8);

    root->addWidget(m_statusLabel);
    root->addWidget(m_progressBar);
}

void BootstrapWindow::start()
{
    QTimer::singleShot(80, this, [this]() { stageManualInstall(); });
}

// ============================================================
// 第 0 階段：掃描根目錄手動安裝包
// ============================================================
void BootstrapWindow::stageManualInstall()
{
    setStatus(QStringLiteral("正在掃描安裝包..."));
    setProgress(0);

    if (!m_manualZipPath.isEmpty() && QFileInfo::exists(m_manualZipPath)) {
        setStatus(QStringLiteral("偵測到手動安裝包，準備安裝..."));
        applyUpdate(m_manualZipPath);
        return;
    }

    const QDir dir(appDir());
    const QStringList updates = dir.entryList(
        QStringList() << QStringLiteral("*.sgsl_update"),
        QDir::Files, QDir::Time
    );

    if (!updates.isEmpty()) {
        const QString zipPath = dir.absoluteFilePath(updates.first());
        QString infoDesc;
        const QString infoPath = zipPath + QStringLiteral(".info.json");
        if (QFileInfo::exists(infoPath)) {
            QFile f(infoPath);
            if (f.open(QIODevice::ReadOnly)) {
                QJsonObject info = QJsonDocument::fromJson(f.readAll()).object();
                infoDesc = QStringLiteral("版本：%1\n類型：%2\n說明：%3")
                               .arg(info.value(QStringLiteral("version")).toString())
                               .arg(info.value(QStringLiteral("type")).toString())
                               .arg(info.value(QStringLiteral("description")).toString());
            }
        }

        const QString msg = infoDesc.isEmpty()
            ? QStringLiteral("偵測到本地更新包：\n%1\n\n是否立即安裝？").arg(updates.first())
            : QStringLiteral("偵測到本地更新包：\n%1\n\n%2\n\n是否立即安裝？").arg(updates.first(), infoDesc);

        if (QMessageBox::question(this, QStringLiteral("偵測到更新包"), msg,
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
            setStatus(QStringLiteral("正在安裝本地更新包..."));
            applyUpdate(zipPath);
            return;
        }
    }

    setProgress(25);
    stageHotfixCheck();
}

// ============================================================
// 第 1 階段：線上熱修復檢查
// ============================================================
void BootstrapWindow::stageHotfixCheck()
{
    if (m_fastLaunch) {
        setStatus(QStringLiteral("快速啟動模式，略過熱修復檢查..."));
        setProgress(50);
        QTimer::singleShot(300, this, [this]() { stageConfigCheck(); });
        return;
    }

    setStatus(QStringLiteral("正在檢查熱修復更新..."));

    const QString url = QStringLiteral("https://api.github.com/repos/%1/%2/releases")
                            .arg(AppConfig::GithubOwner, AppConfig::GithubRepo);
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "SteamGameServerLauncher-Bootstrap");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_activeReply = m_network->get(req);
    connect(m_activeReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_activeReply;
        m_activeReply = nullptr;
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            setStatus(QStringLiteral("熱修復檢查失敗（網路錯誤），繼續啟動..."));
            setProgress(50);
            QTimer::singleShot(600, this, [this]() { stageConfigCheck(); });
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isArray()) {
            setProgress(50);
            QTimer::singleShot(0, this, [this]() { stageConfigCheck(); });
            return;
        }

        const AppVersion local = VersionParser::parse(QStringLiteral(APP_VERSION));
        QString bestUrl;
        AppVersion bestVer;

        for (const QJsonValue &rv : doc.array()) {
            const QJsonObject rel = rv.toObject();
            if (rel.value(QStringLiteral("draft")).toBool()) { continue; }

            QString tag = rel.value(QStringLiteral("tag_name")).toString();
            if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) { tag = tag.mid(1); }

            const AppVersion remote = VersionParser::parse(tag);
            if (!VersionParser::isHotfixFor(remote, local)) { continue; }

            if (!bestVer.isValid || remote.suffixSeq > bestVer.suffixSeq) {
                bestVer = remote;
                for (const QJsonValue &av : rel.value(QStringLiteral("assets")).toArray()) {
                    const QJsonObject asset = av.toObject();
                    if (asset.value(QStringLiteral("name")).toString()
                            .endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
                        bestUrl = asset.value(QStringLiteral("browser_download_url")).toString();
                        break;
                    }
                }
            }
        }

        if (!bestVer.isValid || bestUrl.isEmpty()) {
            setStatus(QStringLiteral("無熱修復更新，繼續..."));
            setProgress(50);
            QTimer::singleShot(400, this, [this]() { stageConfigCheck(); });
            return;
        }

        setStatus(QStringLiteral("偵測到熱修復版本 %1，正在下載...").arg(VersionParser::toString(bestVer)));
        downloadFile(bestUrl,
            [this](const QString &savedPath) {
                setProgress(50);
                setStatus(QStringLiteral("熱修復下載完成，正在套用更新..."));
                applyUpdate(savedPath);
            },
            [this]() {
                setStatus(QStringLiteral("熱修復下載失敗，跳過並繼續啟動..."));
                setProgress(50);
                QTimer::singleShot(600, this, [this]() { stageConfigCheck(); });
            },
            25, 50
        );
    });
}

// ============================================================
// 第 2 階段：Config 驗證
// ============================================================
void BootstrapWindow::stageConfigCheck()
{
    setStatus(QStringLiteral("正在驗證設定檔..."));
    setProgress(50);

    if (!loadConfig()) {
        setStatus(QStringLiteral("設定檔異常，已備份並重建，繼續啟動..."));
    } else {
        setStatus(QStringLiteral("設定檔驗證通過。"));
    }

    setProgress(75);
    QTimer::singleShot(200, this, [this]() { stageUpdateCheck(); });
}

bool BootstrapWindow::loadConfig()
{
    const QString path = configPath();
    QFile file(path);

    if (!file.exists()) {
        m_autoCheckUpdate   = true;
        m_autoExecuteUpdate = false;
        return true;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        m_autoCheckUpdate   = true;
        m_autoExecuteUpdate = false;
        return false;
    }

    const QByteArray raw = file.readAll();
    file.close();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);

    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        const QString backupPath = path + QStringLiteral(".BKbackup.cfg");
        QFile::remove(backupPath);
        QFile::copy(path, backupPath);

        m_autoCheckUpdate   = true;
        m_autoExecuteUpdate = false;

        QMessageBox::warning(this, QStringLiteral("設定檔異常"),
            QStringLiteral(
                "launcher_settings.json 格式錯誤，可能已損毀。\n\n"
                "已將損毀的設定檔備份至：\n%1\n\n"
                "程式將以預設值繼續啟動。"
            ).arg(backupPath));
        return false;
    }

    const QJsonObject obj = doc.object();
    m_autoCheckUpdate   = obj.value(QStringLiteral("autoCheckUpdate")).toBool(true);
    m_autoExecuteUpdate = obj.value(QStringLiteral("autoExecuteUpdate")).toBool(false);
    return true;
}

// ============================================================
// 第 3 階段：一般更新檢查
// ============================================================
void BootstrapWindow::stageUpdateCheck()
{
    if (!m_autoCheckUpdate) {
        setStatus(QStringLiteral("已略過更新檢查（設定關閉）。"));
        setProgress(100);
        QTimer::singleShot(300, this, [this]() { stageLaunchApp(); });
        return;
    }

    setStatus(QStringLiteral("正在檢查最新版本..."));

    const QString url = QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
                            .arg(AppConfig::GithubOwner, AppConfig::GithubRepo);
    QNetworkRequest req(url);
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "SteamGameServerLauncher-Bootstrap");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    m_activeReply = m_network->get(req);
    connect(m_activeReply, &QNetworkReply::finished, this, [this]() {
        QNetworkReply *reply = m_activeReply;
        m_activeReply = nullptr;
        reply->deleteLater();

        auto skip = [this]() {
            setProgress(100);
            QTimer::singleShot(0, this, [this]() { stageLaunchApp(); });
        };

        if (reply->error() != QNetworkReply::NoError) { skip(); return; }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) { skip(); return; }

        const QJsonObject root = doc.object();
        QString tag = root.value(QStringLiteral("tag_name")).toString();
        if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive)) { tag = tag.mid(1); }

        const AppVersion remote = VersionParser::parse(tag);
        const AppVersion local  = VersionParser::parse(QStringLiteral(APP_VERSION));

        if (!VersionParser::isNewerThan(remote, local)) {
            setStatus(QStringLiteral("目前已是最新版本。"));
            setProgress(100);
            QTimer::singleShot(500, this, [this]() { stageLaunchApp(); });
            return;
        }

        const QString newVerStr = VersionParser::toString(remote);
        setStatus(QStringLiteral("發現新版本：%1").arg(newVerStr));

        if (!m_autoExecuteUpdate) {
            setProgress(100);
            QTimer::singleShot(600, this, [this]() { stageLaunchApp(); });
            return;
        }

        QString downloadUrl;
        for (const QJsonValue &av : root.value(QStringLiteral("assets")).toArray()) {
            const QJsonObject asset = av.toObject();
            if (asset.value(QStringLiteral("name")).toString()
                    .endsWith(QStringLiteral(".zip"), Qt::CaseInsensitive)) {
                downloadUrl = asset.value(QStringLiteral("browser_download_url")).toString();
                break;
            }
        }

        if (downloadUrl.isEmpty()) { skip(); return; }

        setStatus(QStringLiteral("正在自動下載更新 %1...").arg(newVerStr));
        downloadFile(downloadUrl,
            [this](const QString &savedPath) {
                setProgress(100);
                setStatus(QStringLiteral("下載完成，正在套用更新..."));
                applyUpdate(savedPath);
            },
            [this]() { stageLaunchApp(); },
            75, 100
        );
    });
}

// ============================================================
// 最終步驟：啟動主程式
// ============================================================
void BootstrapWindow::stageLaunchApp()
{
    setStatus(QStringLiteral("正在啟動程式..."));
    setProgress(100);

    const QString exePath = appDir() + QStringLiteral("/") + AppConfig::LauncherExeName;

    if (!QFileInfo::exists(exePath)) {
        QMessageBox::critical(this, QStringLiteral("啟動失敗"),
            QStringLiteral("找不到主程式：\n%1").arg(exePath));
        QApplication::quit();
        return;
    }

#ifdef Q_OS_WIN
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask      = SEE_MASK_DEFAULT;
    sei.hwnd       = NULL;
    sei.lpVerb     = L"open";
    sei.lpFile     = reinterpret_cast<LPCWSTR>(exePath.utf16());
    sei.lpDirectory = reinterpret_cast<LPCWSTR>(appDir().utf16());
    sei.nShow      = SW_SHOWNORMAL;
    ShellExecuteExW(&sei);
#else
    QProcess::startDetached(exePath, QStringList(), appDir());
#endif

    QTimer::singleShot(200, this, []() { QApplication::quit(); });
}

// ============================================================
// 工具方法
// ============================================================
void BootstrapWindow::downloadFile(
    const QString &url,
    std::function<void(const QString &)> onSuccess,
    std::function<void()> onFail,
    int progressStart,
    int progressEnd)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setRawHeader("User-Agent", "SteamGameServerLauncher-Bootstrap");

    m_activeReply = m_network->get(req);

    connect(m_activeReply, &QNetworkReply::downloadProgress, this,
        [this, progressStart, progressEnd](qint64 received, qint64 total) {
            if (total > 0) {
                const int prog = progressStart +
                    static_cast<int>(received * (progressEnd - progressStart) / total);
                setProgress(prog);
            }
        });

    connect(m_activeReply, &QNetworkReply::finished, this,
        [this, onSuccess, onFail]() {
            QNetworkReply *reply = m_activeReply;
            m_activeReply = nullptr;
            reply->deleteLater();

            if (reply->error() != QNetworkReply::NoError) {
                onFail();
                return;
            }

            const QString tempDir  = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            const QString savePath = QDir(tempDir).filePath(QStringLiteral("sgsl_dl_tmp.zip"));
            QFile f(savePath);
            if (!f.open(QIODevice::WriteOnly)) {
                onFail();
                return;
            }
            f.write(reply->readAll());
            f.close();
            onSuccess(savePath);
        });
}

void BootstrapWindow::applyUpdate(const QString &zipPath)
{
    const QString updaterExe  = appDir() + QStringLiteral("/") + AppConfig::UpdaterExeName;
    const QString bootstrapExe = QCoreApplication::applicationFilePath();

    if (!QFileInfo::exists(updaterExe)) {
        QMessageBox::warning(this, QStringLiteral("更新失敗"),
            QStringLiteral("找不到更新程式：%1\n將略過本次更新繼續啟動。").arg(updaterExe));
        stageConfigCheck();
        return;
    }

    const QString argsStr = QStringLiteral("--zip \"%1\" --target \"%2\" --exe \"%3\" --pid 0")
                                .arg(QDir::toNativeSeparators(zipPath))
                                .arg(QDir::toNativeSeparators(appDir()))
                                .arg(QDir::toNativeSeparators(bootstrapExe));
#ifdef Q_OS_WIN
    bool needsElevation = true;
    QFile testFile(appDir() + QStringLiteral("/.write_test"));
    if (testFile.open(QIODevice::WriteOnly)) {
        testFile.close();
        testFile.remove();
        needsElevation = false;
    }

    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask        = SEE_MASK_DEFAULT;
    sei.hwnd         = NULL;
    sei.lpVerb       = needsElevation ? L"runas" : L"open";
    sei.lpFile       = reinterpret_cast<LPCWSTR>(updaterExe.utf16());
    sei.lpParameters = reinterpret_cast<LPCWSTR>(argsStr.utf16());
    sei.lpDirectory  = reinterpret_cast<LPCWSTR>(appDir().utf16());
    sei.nShow        = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&sei)) {
        QMessageBox::warning(this, QStringLiteral("更新失敗"),
            QStringLiteral("無法啟動更新程式，將略過本次更新。"));
        stageConfigCheck();
        return;
    }
#else
    QProcess::startDetached(updaterExe,
        { QStringLiteral("--zip"), zipPath,
          QStringLiteral("--target"), appDir(),
          QStringLiteral("--exe"), bootstrapExe,
          QStringLiteral("--pid"), QStringLiteral("0") });
#endif

    QTimer::singleShot(100, this, []() { QApplication::quit(); });
}

void BootstrapWindow::setStatus(const QString &msg)
{
    if (m_statusLabel) { m_statusLabel->setText(msg); }
    QApplication::processEvents();
}

void BootstrapWindow::setProgress(int value, int total)
{
    if (m_progressBar) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(value);
    }
    QApplication::processEvents();
}

QString BootstrapWindow::appDir() const
{
    return QCoreApplication::applicationDirPath();
}

QString BootstrapWindow::configPath() const
{
    return appDir() + QStringLiteral("/GameData/") + AppConfig::ConfigFileName;
}