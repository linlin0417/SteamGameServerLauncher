#include "ServerSettingsPanel.h"
#include "../Core/ServerInstance.h"
#include "../Core/GameProfile.h"
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDebug>
#include <QJsonValue>

ServerSettingsPanel::ServerSettingsPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void ServerSettingsPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setStyleSheet(
        "QScrollArea { border: none; background-color: transparent; }"
        "QWidget#scrollContent { background-color: #171a21; }"
    );

    m_scrollContent = new QWidget();
    m_scrollContent->setObjectName(QStringLiteral("scrollContent"));
    QVBoxLayout *contentLayout = new QVBoxLayout(m_scrollContent);
    contentLayout->setSpacing(15);

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
        "QLineEdit, QSpinBox { "
        "    background-color: #101214; "
        "    color: #c7d5e0; "
        "    border: 1px solid #202d39; "
        "    padding: 4px; "
        "    border-radius: 2px; "
        "} "
        "QLineEdit:focus, QSpinBox:focus { "
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
        "QPushButton:pressed { background-color: #1b2838; }";

    QGroupBox *grpFixed = new QGroupBox(QStringLiteral("基本設定"));
    grpFixed->setStyleSheet(groupStyle);
    QFormLayout *fixedForm = new QFormLayout(grpFixed);
    fixedForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_editSteamCmdPath = new QLineEdit();
    m_editInstallDir = new QLineEdit();
    m_editServerExePath = new QLineEdit();
    m_editAdditionalArgs = new QLineEdit();
    m_editDiscordWebhook = new QLineEdit();
    m_btnTestWebhook = new QPushButton(QStringLiteral("測試 Webhook"));
    
    QHBoxLayout *webhookLayout = new QHBoxLayout();
    webhookLayout->addWidget(m_editDiscordWebhook);
    webhookLayout->addWidget(m_btnTestWebhook);

    fixedForm->addRow(QStringLiteral("SteamCMD 路徑:"), m_editSteamCmdPath);
    fixedForm->addRow(QStringLiteral("安裝目錄:"), m_editInstallDir);
    fixedForm->addRow(QStringLiteral("執行檔路徑:"), m_editServerExePath);
    fixedForm->addRow(QStringLiteral("額外啟動參數:"), m_editAdditionalArgs);
    fixedForm->addRow(QStringLiteral("Discord Webhook:"), webhookLayout);

    contentLayout->addWidget(grpFixed);

    m_grpDynamic = new QGroupBox(QStringLiteral("伺服器參數"));
    m_grpDynamic->setStyleSheet(groupStyle);
    m_dynamicForm = new QFormLayout(m_grpDynamic);
    m_dynamicForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    contentLayout->addWidget(m_grpDynamic);

    contentLayout->addStretch(1);

    m_scrollArea->setWidget(m_scrollContent);
    mainLayout->addWidget(m_scrollArea);

    QPushButton *btnSave = new QPushButton(QStringLiteral("儲存設定"));
    btnSave->setStyleSheet(
        "QPushButton { "
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #79a01b, stop:1 #5c7e10); "
        "    color: white; "
        "    border: none; "
        "    padding: 8px 16px; "
        "    border-radius: 2px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #8eb822, stop:1 #6b9313); "
        "} "
        "QPushButton:pressed { "
        "    background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #5c7e10, stop:1 #4a660d); "
        "}"
    );
    connect(btnSave, &QPushButton::clicked, this, &ServerSettingsPanel::saveSettingsFromUI);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(btnSave);
    mainLayout->addLayout(bottomLayout);

    connect(m_btnTestWebhook, &QPushButton::clicked, this, &ServerSettingsPanel::onTestWebhook);
}

void ServerSettingsPanel::bindInstance(ServerInstance *instance)
{
    if (m_instance == instance) return;
    
    m_instance = instance;
    if (m_instance) {
        rebuildDynamicForm();
        loadSettingsToUI();
    } else {
        while (QLayoutItem *item = m_dynamicForm->takeAt(0)) {
            if (QWidget *w = item->widget()) w->deleteLater();
            delete item;
        }
        m_dynamicWidgets.clear();
        m_editSteamCmdPath->clear();
        m_editInstallDir->clear();
        m_editServerExePath->clear();
        m_editAdditionalArgs->clear();
        m_editDiscordWebhook->clear();
    }
}

void ServerSettingsPanel::unbindInstance()
{
    bindInstance(nullptr);
}

void ServerSettingsPanel::rebuildDynamicForm()
{
    while (QLayoutItem *item = m_dynamicForm->takeAt(0)) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    m_dynamicWidgets.clear();

    if (!m_instance) return;

    QStringList varNames = m_instance->profile().extractVariableNames();
    for (const QString &varName : varNames) {
        QWidget *inputWidget = nullptr;

        if (varName == QStringLiteral("serverName")) {
            QLineEdit *le = new QLineEdit();
            inputWidget = le;
        } else if (varName.contains(QStringLiteral("password"), Qt::CaseInsensitive)) {
            QLineEdit *le = new QLineEdit();
            le->setEchoMode(QLineEdit::PasswordEchoOnEdit);
            inputWidget = le;
        } else if (varName.contains(QStringLiteral("port"), Qt::CaseInsensitive)) {
            QSpinBox *sb = new QSpinBox();
            sb->setRange(1, 65535);
            inputWidget = sb;
        } else if (varName == QStringLiteral("maxPlayers")) {
            QSpinBox *sb = new QSpinBox();
            sb->setRange(1, 999);
            inputWidget = sb;
        } else if (varName.contains(QStringLiteral("memory"), Qt::CaseInsensitive)) {
            QSpinBox *sb = new QSpinBox();
            sb->setRange(1, 64);
            sb->setSuffix(QStringLiteral(" GB"));
            inputWidget = sb;
        } else if (varName == QStringLiteral("javaPath")) {
            QWidget *w = new QWidget();
            QHBoxLayout *hl = new QHBoxLayout(w);
            hl->setContentsMargins(0, 0, 0, 0);
            QLineEdit *le = new QLineEdit();
            QPushButton *btn = new QPushButton(QStringLiteral("瀏覽..."));
            hl->addWidget(le);
            hl->addWidget(btn);
            connect(btn, &QPushButton::clicked, this, [this, le]() {
                QString path = QFileDialog::getOpenFileName(this, QStringLiteral("選擇 Java 執行檔"));
                if (!path.isEmpty()) {
                    le->setText(path);
                }
            });
            inputWidget = w;
            m_dynamicWidgets.insert(varName, le);
        } else {
            QLineEdit *le = new QLineEdit();
            inputWidget = le;
        }

        // 設置淺灰色預設值 (Placeholder)
        if (QLineEdit *le = qobject_cast<QLineEdit*>(inputWidget)) {
            QJsonValue defVal = m_instance->profile().extraDefaults.value(varName);
            if (!defVal.isUndefined()) {
                if (defVal.isString()) le->setPlaceholderText(defVal.toString());
                else if (defVal.isDouble()) le->setPlaceholderText(defVal.toVariant().toString());
                else if (defVal.isBool()) le->setPlaceholderText(defVal.toBool() ? QStringLiteral("True") : QStringLiteral("False"));
            }
        } else if (QSpinBox *sb = qobject_cast<QSpinBox*>(inputWidget)) {
            // SpinBox 不支援 Placeholder，略過
        }

        if (!m_dynamicWidgets.contains(varName)) {
            m_dynamicWidgets.insert(varName, inputWidget);
        }

        m_dynamicForm->addRow(varName + QStringLiteral(":"), inputWidget);
    }
    
    m_grpDynamic->setVisible(!varNames.isEmpty());
}

void ServerSettingsPanel::loadSettingsToUI()
{
    if (!m_instance) return;

    // 讀取啟動器內部設定檔 (原始設定，不含 extraDefaults 合併)
    QJsonObject rawSettings = m_instance->settings();
    QJsonObject mergedSettings = m_instance->mergedSettings();
    GameProfile profile = m_instance->profile();

    // --- 雙向同步：從真正的伺服器設定檔讀取最新數值 ---
    QString formatStr;
    switch (profile.configFormat) {
        case GameProfile::INI: formatStr = QStringLiteral("ini"); break;
        case GameProfile::Properties: formatStr = QStringLiteral("properties"); break;
        case GameProfile::JSON_Config: formatStr = QStringLiteral("json"); break;
        default: formatStr = QStringLiteral("none"); break;
    }

    if (formatStr != QStringLiteral("none")) {
        QString configPath = m_instance->installDir() + '/' + profile.configFilePath;
        QJsonObject readSettings = ServerManager::readGameConfig(
            formatStr, configPath, profile.configSection, profile.configMappings);
        
        if (!readSettings.isEmpty()) {
            emit logMessage(QStringLiteral("[同步] 已載入並同步外部設定檔參數。"));
            for (auto it = readSettings.constBegin(); it != readSettings.constEnd(); ++it) {
                rawSettings.insert(it.key(), it.value());
                mergedSettings.insert(it.key(), it.value()); // 同時更新顯示用的 merged
            }
            m_instance->setSettings(rawSettings); // 回存至記憶體
        } else {
            emit logMessage(QStringLiteral("[同步] 伺服器設定檔尚未建立，使用預設參數。"));
        }
    }

    m_editSteamCmdPath->setText(mergedSettings.value(QStringLiteral("steamCmdPath")).toString());
    m_editInstallDir->setText(mergedSettings.value(QStringLiteral("installDir")).toString());
    m_editServerExePath->setText(mergedSettings.value(QStringLiteral("serverExePath")).toString());
    m_editAdditionalArgs->setText(mergedSettings.value(QStringLiteral("additionalArgs")).toString());
    m_editDiscordWebhook->setText(mergedSettings.value(QStringLiteral("discordWebhook")).toString());

    for (auto it = m_dynamicWidgets.begin(); it != m_dynamicWidgets.end(); ++it) {
        QString varName = it.key();
        QWidget *w = it.value();
        
        // 若原始設定或 INI 有給值，才填入輸入框，否則留空顯示 Placeholder
        if (rawSettings.contains(varName)) {
            QJsonValue val = rawSettings.value(varName);
            if (QLineEdit *le = qobject_cast<QLineEdit*>(w)) {
                le->setText(val.isString() ? val.toString() : val.toVariant().toString());
            } else if (QSpinBox *sb = qobject_cast<QSpinBox*>(w)) {
                sb->setValue(val.toInt());
            }
        } else {
            if (QLineEdit *le = qobject_cast<QLineEdit*>(w)) {
                le->clear();
            }
        }
    }
}

void ServerSettingsPanel::saveSettingsFromUI()
{
    if (!m_instance) return;

    QJsonObject settings;
    settings.insert(QStringLiteral("steamCmdPath"), m_editSteamCmdPath->text());
    settings.insert(QStringLiteral("installDir"), m_editInstallDir->text());
    settings.insert(QStringLiteral("serverExePath"), m_editServerExePath->text());
    settings.insert(QStringLiteral("additionalArgs"), m_editAdditionalArgs->text());
    settings.insert(QStringLiteral("discordWebhook"), m_editDiscordWebhook->text());

    for (auto it = m_dynamicWidgets.begin(); it != m_dynamicWidgets.end(); ++it) {
        QString varName = it.key();
        QWidget *w = it.value();

        if (QLineEdit *le = qobject_cast<QLineEdit*>(w)) {
            if (!le->text().isEmpty()) { // 空字串不存入，讓底層去 fallback 預設值
                settings.insert(varName, le->text());
            }
        } else if (QSpinBox *sb = qobject_cast<QSpinBox*>(w)) {
            settings.insert(varName, sb->value());
        }
    }

    m_instance->setSettings(settings);
    m_instance->saveSettings();
    
    // 立即套用回真實的伺服器設定檔 (INI)
    m_instance->applyGameConfig();
    
    emit logMessage(QStringLiteral("設定已儲存並同步至伺服器檔案。"));
}

void ServerSettingsPanel::resetToDefaults()
{
    // Reserved
}

void ServerSettingsPanel::onTestWebhook()
{
    emit logMessage(QStringLiteral("正在測試 Webhook... (尚未實作)"));
}

void ServerSettingsPanel::onBrowseJavaPath()
{
    // handled by lambda
}
