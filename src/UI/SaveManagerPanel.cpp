#include "SaveManagerPanel.h"
#include "../Core/ServerInstance.h"
#include "../Core/GameProfile.h"
#include "../Core/MapPackager.h"
#include "../Core/ServerManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfoList>
#include <QDateTime>
#include <QMessageBox>

SaveManagerPanel::SaveManagerPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void SaveManagerPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QString groupStyle = 
        "QGroupBox { "
        "    color: #66c0f4; "
        "    font-weight: bold; "
        "    border: 1px solid #202d39; "
        "    border-radius: 4px; "
        "    margin-top: 12px; "
        "    background-color: #1b2838; "
        "} "
        "QGroupBox::title { "
        "    subcontrol-origin: margin; "
        "    left: 10px; "
        "    padding: 0 3px 0 3px; "
        "} "
        "QLineEdit, QTextEdit, QComboBox { "
        "    background-color: #101214; "
        "    color: #c7d5e0; "
        "    border: 1px solid #202d39; "
        "    padding: 4px; "
        "    border-radius: 2px; "
        "} "
        "QLineEdit:focus, QTextEdit:focus, QComboBox:focus { "
        "    border: 1px solid #66c0f4; "
        "} "
        "QLabel { color: #c7d5e0; } "
        "QPushButton { "
        "    background-color: #22303f; "
        "    color: #c7d5e0; "
        "    border: 1px solid #202d39; "
        "    padding: 6px 12px; "
        "    border-radius: 2px; "
        "} "
        "QPushButton:hover { background-color: #2a3f54; border: 1px solid #66c0f4; } "
        "QPushButton:pressed { background-color: #1b2838; }"
        "QPushButton:disabled { color: #8f98a0; border: 1px solid #202d39; }";

    m_noSaveLabel = new QLabel(QStringLiteral("此遊戲未設定存檔管理"));
    m_noSaveLabel->setAlignment(Qt::AlignCenter);
    m_noSaveLabel->setStyleSheet(QStringLiteral("color: #8f98a0; font-size: 16px;"));
    mainLayout->addWidget(m_noSaveLabel);

    m_contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(m_contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    QGroupBox *grpExport = new QGroupBox(QStringLiteral("匯出存檔"));
    grpExport->setStyleSheet(groupStyle);
    QFormLayout *exportForm = new QFormLayout(grpExport);
    exportForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QHBoxLayout *comboLayout = new QHBoxLayout();
    m_comboSaves = new QComboBox();
    comboLayout->addWidget(m_comboSaves, 1);
    m_btnRefresh = new QPushButton(QStringLiteral("重新整理"));
    comboLayout->addWidget(m_btnRefresh);
    exportForm->addRow(QStringLiteral("選擇存檔:"), comboLayout);

    m_editPackageName = new QLineEdit();
    exportForm->addRow(QStringLiteral("地圖包名稱:"), m_editPackageName);

    m_editNotes = new QTextEdit();
    m_editNotes->setMaximumHeight(60);
    exportForm->addRow(QStringLiteral("備註:"), m_editNotes);

    QHBoxLayout *previewLayout = new QHBoxLayout();
    m_editPreviewPath = new QLineEdit();
    QPushButton *btnBrowsePreview = new QPushButton(QStringLiteral("瀏覽..."));
    previewLayout->addWidget(m_editPreviewPath);
    previewLayout->addWidget(btnBrowsePreview);
    exportForm->addRow(QStringLiteral("預覽圖片:"), previewLayout);

    m_chkLegacyFormat = new QCheckBox(QStringLiteral("同時匯出 .IcarusMap 格式 (相容舊版)"));
    m_chkLegacyFormat->setStyleSheet(QStringLiteral("color: #c7d5e0;"));
    m_chkLegacyFormat->setVisible(false);
    exportForm->addRow(QStringLiteral(""), m_chkLegacyFormat);

    m_btnExport = new QPushButton(QStringLiteral("匯出地圖包"));
    m_btnExport->setStyleSheet(
        "QPushButton { "
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #79a01b, stop:1 #5c7e10); "
        "    color: white; "
        "    border: none; font-weight: bold; padding: 8px; border-radius: 2px; "
        "} "
        "QPushButton:hover:!disabled { "
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8eb822, stop:1 #6b9313); "
        "} "
        "QPushButton:disabled { background-color: #3c3d3e; color: #8f98a0; }"
    );
    exportForm->addRow(QStringLiteral(""), m_btnExport);

    contentLayout->addWidget(grpExport);

    QGroupBox *grpImport = new QGroupBox(QStringLiteral("匯入存檔"));
    grpImport->setStyleSheet(groupStyle);
    QVBoxLayout *importLayout = new QVBoxLayout(grpImport);

    m_btnImport = new QPushButton(QStringLiteral("選擇地圖包並匯入..."));
    importLayout->addWidget(m_btnImport);

    m_importInfoLabel = new QLabel(QStringLiteral("支援 .SGSLMap 與 .IcarusMap 格式。匯入將覆寫同名存檔。"));
    m_importInfoLabel->setWordWrap(true);
    m_importInfoLabel->setStyleSheet(QStringLiteral("color: #8f98a0;"));
    importLayout->addWidget(m_importInfoLabel);

    m_previewImage = new QLabel();
    m_previewImage->setAlignment(Qt::AlignCenter);
    m_previewImage->setMinimumHeight(150);
    m_previewImage->setStyleSheet(QStringLiteral("background-color: #101214; border: 1px solid #202d39;"));
    importLayout->addWidget(m_previewImage);

    contentLayout->addWidget(grpImport);
    contentLayout->addStretch(1);

    mainLayout->addWidget(m_contentWidget);
    m_contentWidget->setVisible(false);

    connect(m_btnRefresh, &QPushButton::clicked, this, &SaveManagerPanel::refreshSaveList);
    connect(m_btnExport, &QPushButton::clicked, this, &SaveManagerPanel::onExportSave);
    connect(m_btnImport, &QPushButton::clicked, this, &SaveManagerPanel::onImportSave);
    connect(btnBrowsePreview, &QPushButton::clicked, this, &SaveManagerPanel::onBrowsePreview);
}

void SaveManagerPanel::bindInstance(ServerInstance *instance)
{
    m_instance = instance;
    if (!m_instance) {
        m_noSaveLabel->setVisible(true);
        m_contentWidget->setVisible(false);
        return;
    }

    GameProfile profile = m_instance->profile();
    if (profile.savesRelativePath.isEmpty()) {
        m_noSaveLabel->setVisible(true);
        m_contentWidget->setVisible(false);
    } else {
        m_noSaveLabel->setVisible(false);
        m_contentWidget->setVisible(true);
        m_chkLegacyFormat->setVisible(profile.id == QStringLiteral("icarus"));
        refreshSaveList();
    }
}

void SaveManagerPanel::unbindInstance()
{
    bindInstance(nullptr);
}

QString SaveManagerPanel::savesDir() const
{
    if (!m_instance) return QString();
    
    QJsonObject settings = m_instance->mergedSettings();
    QString installDir = settings.value(QStringLiteral("installDir")).toString();
    if (installDir.isEmpty()) return QString();
    
    QString relPath = m_instance->profile().savesRelativePath;
    if (relPath.isEmpty()) return QString();
    
    QDir dir(installDir);
    return dir.filePath(relPath);
}

void SaveManagerPanel::refreshSaveList()
{
    m_comboSaves->clear();
    
    QString dirPath = savesDir();
    if (dirPath.isEmpty()) return;
    
    QDir dir(dirPath);
    if (!dir.exists()) return;
    
    QStringList patterns = m_instance->profile().saveFilePatterns;
    if (patterns.isEmpty()) {
        patterns << QStringLiteral("*");
    }
    
    QStringList files = dir.entryList(patterns, QDir::Files | QDir::NoDotAndDotDot);
    QStringList uniqueNames;
    
    for (const QString &file : files) {
        QString prospectName = file;
        
        // 嘗試根據 pattern 移除副檔名
        bool matched = false;
        for (const QString &pattern : patterns) {
            if (pattern.startsWith(QStringLiteral("*."))) {
                QString suffix = pattern.mid(1); // e.g. ".json", ".json.backup"
                if (file.endsWith(suffix, Qt::CaseInsensitive)) {
                    prospectName = file.left(file.length() - suffix.length());
                    matched = true;
                    break;
                }
            }
        }
        
        // 若沒有被 pattern 匹配或移去副檔名，則使用預設方法
        if (!matched) {
            QFileInfo fi(file);
            prospectName = fi.completeBaseName();
        }
        
        if (!uniqueNames.contains(prospectName)) {
            uniqueNames.append(prospectName);
        }
    }
    
    uniqueNames.sort();
    for (const QString &name : uniqueNames) {
        m_comboSaves->addItem(name);
    }
    
    if (m_comboSaves->count() > 0) {
        m_comboSaves->setCurrentIndex(0);
    }
}

void SaveManagerPanel::onBrowsePreview()
{
    QString path = QFileDialog::getOpenFileName(this, QStringLiteral("選擇預覽圖片"), QString(), QStringLiteral("Images (*.png *.jpg *.jpeg)"));
    if (!path.isEmpty()) {
        m_editPreviewPath->setText(path);
    }
}

void SaveManagerPanel::onExportSave()
{
    if (!m_instance) return;
    
    if (m_instance->isRunning()) {
        emit logMessage(QStringLiteral("錯誤: 伺服器執行中，無法匯出存檔。"));
        return;
    }
    
    QString prospectName = m_comboSaves->currentText();
    if (prospectName.isEmpty()) {
        emit logMessage(QStringLiteral("請先選擇要匯出的存檔。"));
        return;
    }
    
    QString exportPath = QFileDialog::getSaveFileName(this, QStringLiteral("匯出地圖包"), 
        prospectName + QStringLiteral(".SGSLMap"), QStringLiteral("SGSLMap Files (*.SGSLMap)"));
        
    if (exportPath.isEmpty()) return;
    
    MapMetadata meta;
    meta.packageName = m_editPackageName->text();
    if (meta.packageName.isEmpty()) {
        meta.packageName = prospectName;
    }
    meta.originalProspectName = prospectName;
    meta.notes = m_editNotes->toPlainText();
    meta.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    meta.launcherVersion = QStringLiteral("2.0.0");
    
    // UI 2.0.0 新增: 支援多遊戲設定檔
    meta.formatVersion = QStringLiteral("2.0.0");
    meta.gameProfileId = m_instance->profile().id;
    meta.gameDisplayName = m_instance->profile().displayName;
    meta.saveFilePatterns = m_instance->profile().saveFilePatterns;
    
    QString dir = savesDir();
    QString errorMsg;
    
    bool ok = MapPackager::exportMap(exportPath, dir, prospectName, meta, m_editPreviewPath->text(), &errorMsg);
    
    if (ok) {
        emit logMessage(QStringLiteral("成功匯出地圖包: ") + exportPath);
        
        if (m_chkLegacyFormat->isVisible() && m_chkLegacyFormat->isChecked()) {
            QString legacyPath = exportPath;
            legacyPath.replace(QStringLiteral(".SGSLMap"), QStringLiteral(".IcarusMap"));
            
            bool okLegacy = MapPackager::exportMap(legacyPath, dir, prospectName, meta, m_editPreviewPath->text(), &errorMsg);
            if (okLegacy) {
                emit logMessage(QStringLiteral("成功匯出舊版地圖包: ") + legacyPath);
            } else {
                emit logMessage(QStringLiteral("匯出舊版地圖包失敗: ") + errorMsg);
            }
        }
    } else {
        emit logMessage(QStringLiteral("匯出地圖包失敗: ") + errorMsg);
    }
}

void SaveManagerPanel::onImportSave()
{
    if (!m_instance) return;
    
    if (m_instance->isRunning()) {
        emit logMessage(QStringLiteral("錯誤: 伺服器執行中，無法匯入存檔。"));
        return;
    }
    
    QString importPath = QFileDialog::getOpenFileName(this, QStringLiteral("選擇地圖包"), QString(), QStringLiteral("Map Files (*.SGSLMap *.IcarusMap)"));
    if (importPath.isEmpty()) return;
    
    // Read meta to show preview
    bool ok = false;
    MapMetadata meta = MapPackager::readMetadata(importPath, &ok);
    if (ok) {
        QPixmap pixmap = MapPackager::readPreview(importPath);
        if (!pixmap.isNull()) {
            m_previewImage->setPixmap(pixmap.scaled(m_previewImage->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_previewImage->clear();
            m_previewImage->setText(QStringLiteral("無預覽圖片"));
        }
    }
    
    QString dir = savesDir();
    QString errorMsg;
    
    bool importOk = MapPackager::importMap(importPath, dir, true, &errorMsg);
    if (importOk) {
        emit logMessage(QStringLiteral("成功匯入地圖包: ") + importPath);
        refreshSaveList();
    } else {
        emit logMessage(QStringLiteral("匯入地圖包失敗: ") + errorMsg);
    }
}
