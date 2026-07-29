#include "GameProfileDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>

GameProfileDialog::GameProfileDialog(QWidget *parent)
    : QDialog(parent), m_isEditMode(false)
{
    setupUI();
    setWindowTitle(QStringLiteral("新增遊戲設定檔"));
}

GameProfileDialog::GameProfileDialog(const GameProfile &profile, QWidget *parent)
    : QDialog(parent), m_isEditMode(true), m_originalId(profile.id)
{
    setupUI();
    setWindowTitle(QStringLiteral("編輯遊戲設定檔"));
    loadFromProfile(profile);
}

GameProfile GameProfileDialog::resultProfile() const
{
    return m_result;
}

void GameProfileDialog::setupUI()
{
    setFixedSize(550, 650);

    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #1b2838; color: #c7d5e0; }"
        "QLabel { color: #c7d5e0; }"
        "QLineEdit, QSpinBox, QComboBox, QTextEdit {"
        "  background-color: #101214; color: #c7d5e0;"
        "  border: 1px solid #202d39; border-radius: 3px; padding: 4px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTextEdit:focus {"
        "  border: 1px solid #66c0f4;"
        "}"
        "QPushButton {"
        "  background-color: #22303f; color: #c7d5e0;"
        "  border: 1px solid #202d39; border-radius: 3px; padding: 6px 12px;"
        "}"
        "QPushButton:hover { background-color: #2a3f54; }"
        "QPushButton#btnAccept {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #79a01b, stop:1 #5c7e10);"
        "  color: white; border: none;"
        "}"
        "QPushButton#btnAccept:hover {"
        "  background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8cb91f, stop:1 #6b9312);"
        "}"
    ));

    auto *mainLayout = new QVBoxLayout(this);
    auto *formLayout = new QFormLayout();

    m_editId = new QLineEdit(this);
    if (m_isEditMode) {
        m_editId->setReadOnly(true);
        m_editId->setStyleSheet(QStringLiteral("background-color: #202d39; color: #8f98a0;"));
    }
    formLayout->addRow(QStringLiteral("設定檔 ID:"), m_editId);

    m_editDisplayName = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("顯示名稱:"), m_editDisplayName);

    m_comboSourceType = new QComboBox(this);
    m_comboSourceType->addItem(QStringLiteral("SteamCMD"), GameProfile::SteamCMD);
    m_comboSourceType->addItem(QStringLiteral("自訂腳本"), GameProfile::CustomScript);
    m_comboSourceType->addItem(QStringLiteral("手動"), GameProfile::ManualOnly);
    formLayout->addRow(QStringLiteral("安裝來源:"), m_comboSourceType);

    m_sourceStack = new QStackedWidget(this);

    // SteamCMD page (Index 0 in combo, but let's make it index 0 in stack)
    auto *pageSteamCMD = new QWidget();
    auto *layoutSteamCMD = new QFormLayout(pageSteamCMD);
    layoutSteamCMD->setContentsMargins(0, 0, 0, 0);
    m_editSteamAppId = new QLineEdit(pageSteamCMD);
    layoutSteamCMD->addRow(QStringLiteral("Steam App ID:"), m_editSteamAppId);
    m_sourceStack->addWidget(pageSteamCMD);

    // CustomScript page (Index 1)
    auto *pageCustomScript = new QWidget();
    auto *layoutCustomScript = new QFormLayout(pageCustomScript);
    layoutCustomScript->setContentsMargins(0, 0, 0, 0);
    m_editInstallScript = new QLineEdit(pageCustomScript);
    m_editUpdateScript = new QLineEdit(pageCustomScript);

    auto createBrowseBtn = [this](QLineEdit *edit) {
        auto *layout = new QHBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(edit);
        auto *btn = new QPushButton(QStringLiteral("瀏覽..."), this);
        layout->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, edit]() {
            QString path = QFileDialog::getOpenFileName(this, QStringLiteral("選擇腳本"), QString(), QStringLiteral("Scripts (*.bat *.cmd *.ps1 *.sh);;All Files (*)"));
            if (!path.isEmpty()) {
                edit->setText(path);
            }
        });
        return layout;
    };

    layoutCustomScript->addRow(QStringLiteral("安裝腳本:"), createBrowseBtn(m_editInstallScript));
    layoutCustomScript->addRow(QStringLiteral("更新腳本:"), createBrowseBtn(m_editUpdateScript));
    m_sourceStack->addWidget(pageCustomScript);

    // Manual page (Index 2)
    auto *pageManual = new QWidget();
    m_sourceStack->addWidget(pageManual);

    formLayout->addRow(QStringLiteral(""), m_sourceStack);

    m_editExeRelativePath = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("執行檔相對路徑:"), m_editExeRelativePath);

    m_editLaunchArgsTemplate = new QTextEdit(this);
    m_editLaunchArgsTemplate->setFixedHeight(60);
    m_editLaunchArgsTemplate->setPlaceholderText(QStringLiteral("說明: 使用 {variableName} 語法"));
    formLayout->addRow(QStringLiteral("啟動參數模板:"), m_editLaunchArgsTemplate);

    m_comboConfigFormat = new QComboBox(this);
    m_comboConfigFormat->addItem(QStringLiteral("無"), GameProfile::None);
    m_comboConfigFormat->addItem(QStringLiteral("INI"), GameProfile::INI);
    m_comboConfigFormat->addItem(QStringLiteral("Properties"), GameProfile::Properties);
    m_comboConfigFormat->addItem(QStringLiteral("JSON"), GameProfile::JSON_Config);
    formLayout->addRow(QStringLiteral("設定檔格式:"), m_comboConfigFormat);

    m_editConfigFilePath = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("設定檔路徑:"), m_editConfigFilePath);

    m_editConfigSection = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("INI 區段名:"), m_editConfigSection);
    
    // Hide config section by default
    m_editConfigSection->setVisible(false);
    if (QLabel *lbl = qobject_cast<QLabel*>(formLayout->labelForField(m_editConfigSection))) {
        lbl->setVisible(false);
    }

    m_editSavesRelativePath = new QLineEdit(this);
    formLayout->addRow(QStringLiteral("存檔目錄相對路徑:"), m_editSavesRelativePath);

    m_editSaveFilePatterns = new QLineEdit(this);
    m_editSaveFilePatterns->setPlaceholderText(QStringLiteral("例如: *.json, *.json.backup"));
    formLayout->addRow(QStringLiteral("存檔檔案模式:"), m_editSaveFilePatterns);

    m_spinDefaultPort = new QSpinBox(this);
    m_spinDefaultPort->setRange(0, 65535);
    formLayout->addRow(QStringLiteral("預設埠號:"), m_spinDefaultPort);

    m_spinDefaultQueryPort = new QSpinBox(this);
    m_spinDefaultQueryPort->setRange(0, 65535);
    formLayout->addRow(QStringLiteral("預設查詢埠:"), m_spinDefaultQueryPort);

    m_spinDefaultMaxPlayers = new QSpinBox(this);
    m_spinDefaultMaxPlayers->setRange(0, 1000);
    formLayout->addRow(QStringLiteral("預設最大人數:"), m_spinDefaultMaxPlayers);

    mainLayout->addLayout(formLayout);

    auto *btnLayout = new QHBoxLayout();
    auto *btnCancel = new QPushButton(QStringLiteral("取消"), this);
    auto *btnAccept = new QPushButton(QStringLiteral("確定"), this);
    btnAccept->setObjectName(QStringLiteral("btnAccept"));
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnAccept);
    mainLayout->addLayout(btnLayout);

    connect(m_comboSourceType, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &GameProfileDialog::onSourceTypeChanged);

    connect(m_comboConfigFormat, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, formLayout](int index) {
                GameProfile::ConfigFormat format = static_cast<GameProfile::ConfigFormat>(m_comboConfigFormat->itemData(index).toInt());
                bool isIni = (format == GameProfile::INI);
                m_editConfigSection->setVisible(isIni);
                if (QLabel *lbl = qobject_cast<QLabel*>(formLayout->labelForField(m_editConfigSection))) {
                    lbl->setVisible(isIni);
                }
            });

    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    connect(btnAccept, &QPushButton::clicked, this, &GameProfileDialog::onAccept);
    
    // Initial sync
    onSourceTypeChanged(m_comboSourceType->currentIndex());
}

void GameProfileDialog::loadFromProfile(const GameProfile &p)
{
    m_editId->setText(p.id);
    m_editDisplayName->setText(p.displayName);

    int srcIdx = m_comboSourceType->findData(p.sourceType);
    if (srcIdx >= 0) {
        m_comboSourceType->setCurrentIndex(srcIdx);
    }

    m_editSteamAppId->setText(p.steamAppId);
    m_editInstallScript->setText(p.installScript);
    m_editUpdateScript->setText(p.updateScript);
    m_editExeRelativePath->setText(p.exeRelativePath);
    m_editLaunchArgsTemplate->setPlainText(p.launchArgsTemplate);

    int cfgIdx = m_comboConfigFormat->findData(p.configFormat);
    if (cfgIdx >= 0) {
        m_comboConfigFormat->setCurrentIndex(cfgIdx);
    }

    m_editConfigFilePath->setText(p.configFilePath);
    m_editConfigSection->setText(p.configSection);
    m_editSavesRelativePath->setText(p.savesRelativePath);

    m_editSaveFilePatterns->setText(p.saveFilePatterns.join(QStringLiteral(", ")));

    m_spinDefaultPort->setValue(p.defaultPort);
    m_spinDefaultQueryPort->setValue(p.defaultQueryPort);
    m_spinDefaultMaxPlayers->setValue(p.defaultMaxPlayers);
}

GameProfile GameProfileDialog::buildProfile() const
{
    GameProfile p;
    p.id = m_editId->text().trimmed();
    p.displayName = m_editDisplayName->text().trimmed();
    p.sourceType = static_cast<GameProfile::SourceType>(m_comboSourceType->currentData().toInt());
    p.steamAppId = m_editSteamAppId->text().trimmed();
    p.installScript = m_editInstallScript->text().trimmed();
    p.updateScript = m_editUpdateScript->text().trimmed();
    p.exeRelativePath = m_editExeRelativePath->text().trimmed();
    p.launchArgsTemplate = m_editLaunchArgsTemplate->toPlainText();
    p.configFormat = static_cast<GameProfile::ConfigFormat>(m_comboConfigFormat->currentData().toInt());
    p.configFilePath = m_editConfigFilePath->text().trimmed();
    p.configSection = m_editConfigSection->text().trimmed();
    p.savesRelativePath = m_editSavesRelativePath->text().trimmed();

    QStringList rawPatterns = m_editSaveFilePatterns->text().split(QStringLiteral(","), Qt::SkipEmptyParts);
    QStringList patterns;
    for (const QString &s : rawPatterns) {
        QString t = s.trimmed();
        if (!t.isEmpty()) {
            patterns.append(t);
        }
    }
    p.saveFilePatterns = patterns;

    p.defaultPort = m_spinDefaultPort->value();
    p.defaultQueryPort = m_spinDefaultQueryPort->value();
    p.defaultMaxPlayers = m_spinDefaultMaxPlayers->value();

    return p;
}

void GameProfileDialog::onSourceTypeChanged(int index)
{
    GameProfile::SourceType type = static_cast<GameProfile::SourceType>(m_comboSourceType->itemData(index).toInt());
    switch (type) {
    case GameProfile::SteamCMD:
        m_sourceStack->setCurrentIndex(0);
        break;
    case GameProfile::CustomScript:
        m_sourceStack->setCurrentIndex(1);
        break;
    case GameProfile::ManualOnly:
    default:
        m_sourceStack->setCurrentIndex(2);
        break;
    }
}

void GameProfileDialog::onAccept()
{
    QString idText = m_editId->text().trimmed();
    QString nameText = m_editDisplayName->text().trimmed();
    
    if (idText.isEmpty() || nameText.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("錯誤"), QStringLiteral("設定檔 ID 與顯示名稱不能為空。"));
        return;
    }

    m_result = buildProfile();
    accept();
}
