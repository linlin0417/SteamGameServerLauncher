#include "SidebarWidget.h"
#include "../Core/GameProfile.h"
#include "../version.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QListWidgetItem>
#include <QFrame>

SidebarWidget::SidebarWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void SidebarWidget::setupUI()
{
    setMinimumWidth(220);
    
    // 設定側邊欄背景，微漸層（非純平）
    setStyleSheet("SidebarWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1b2838, stop:1 #171d25); }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 標題區域
    QWidget *headerWidget = new QWidget(this);
    QVBoxLayout *headerLayout = new QVBoxLayout(headerWidget);
    // 標題微偏左，非完美置中對齊
    headerLayout->setContentsMargins(15, 20, 10, 20);
    headerLayout->setSpacing(5);

    m_titleLabel = new QLabel("SGSL", headerWidget);
    m_titleLabel->setStyleSheet("color: #ffffff; font-size: 16pt; font-weight: bold;");
    
    m_versionLabel = new QLabel(QStringLiteral("v%1").arg(APP_VERSION), headerWidget);
    m_versionLabel->setStyleSheet("color: #8f98a0; font-size: 9pt;");
    
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_versionLabel);
    
    mainLayout->addWidget(headerWidget);

    // 分隔線
    QFrame *line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("background-color: #202d39; max-height: 1px; border: none;");
    mainLayout->addWidget(line1);

    // QListWidget 區域
    m_profileList = new QListWidget(this);
    m_profileList->setFocusPolicy(Qt::NoFocus);
    m_profileList->setStyleSheet(
        "QListWidget {"
        "    background-color: transparent;"
        "    border: none;"
        "    outline: none;"
        "}"
        "QListWidget::item {"
        "    background-color: transparent;"
        "    border-left: 3px solid transparent;"
        "    padding: 0px;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #1e2a3b;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #22303f;"
        "    border-left: 3px solid #66c0f4;"
        "}"
    );
    
    connect(m_profileList, &QListWidget::itemClicked, this, &SidebarWidget::onProfileClicked);
    
    mainLayout->addWidget(m_profileList, 1);

    // 分隔線
    QFrame *line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("background-color: #202d39; max-height: 1px; border: none;");
    mainLayout->addWidget(line2);

    // 新增設定檔按鈕
    QWidget *addProfileWidget = new QWidget(this);
    QVBoxLayout *addProfileLayout = new QVBoxLayout(addProfileWidget);
    addProfileLayout->setContentsMargins(10, 10, 10, 10);
    
    m_btnAddProfile = new QPushButton("+ 新增設定檔...", addProfileWidget);
    m_btnAddProfile->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #c7d5e0;"
        "    border: none;"
        "    text-align: left;"
        "    padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "    color: #66c0f4;"
        "}"
    );
    connect(m_btnAddProfile, &QPushButton::clicked, this, [this]() {
        emit addProfileRequested();
    });
    addProfileLayout->addWidget(m_btnAddProfile);
    
    mainLayout->addWidget(addProfileWidget);

    // 分隔線
    QFrame *line3 = new QFrame(this);
    line3->setFrameShape(QFrame::HLine);
    line3->setStyleSheet("background-color: #202d39; max-height: 1px; border: none;");
    mainLayout->addWidget(line3);

    // 功能按鈕區域 (控制, 設定, 存檔)
    QWidget *panelsWidget = new QWidget(this);
    QHBoxLayout *panelsLayout = new QHBoxLayout(panelsWidget);
    panelsLayout->setContentsMargins(10, 10, 10, 10);
    panelsLayout->setSpacing(5);
    
    QString panelBtnStyle = 
        "QPushButton {"
        "    background-color: #1b2838;"
        "    color: #c7d5e0;"
        "    border: 1px solid #202d39;"
        "    border-radius: 3px;"
        "    padding: 5px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #22303f;"
        "    color: #66c0f4;"
        "    border: 1px solid #66c0f4;"
        "}";
    
    m_btnControl = new QPushButton("控制", panelsWidget);
    m_btnControl->setStyleSheet(panelBtnStyle);
    connect(m_btnControl, &QPushButton::clicked, this, [this]() {
        emit panelRequested(PanelControl);
    });
    
    m_btnSettings = new QPushButton("設定", panelsWidget);
    m_btnSettings->setStyleSheet(panelBtnStyle);
    connect(m_btnSettings, &QPushButton::clicked, this, [this]() {
        emit panelRequested(PanelSettings);
    });
    
    m_btnSaves = new QPushButton("存檔", panelsWidget);
    m_btnSaves->setStyleSheet(panelBtnStyle);
    connect(m_btnSaves, &QPushButton::clicked, this, [this]() {
        emit panelRequested(PanelSaveManager);
    });
    
    panelsLayout->addWidget(m_btnControl);
    panelsLayout->addWidget(m_btnSettings);
    panelsLayout->addWidget(m_btnSaves);
    
    mainLayout->addWidget(panelsWidget);

    // 分隔線
    QFrame *line4 = new QFrame(this);
    line4->setFrameShape(QFrame::HLine);
    line4->setStyleSheet("background-color: #202d39; max-height: 1px; border: none;");
    mainLayout->addWidget(line4);

    // 關於/更新區域
    QWidget *aboutWidget = new QWidget(this);
    QVBoxLayout *aboutLayout = new QVBoxLayout(aboutWidget);
    aboutLayout->setContentsMargins(10, 10, 10, 10);
    
    m_btnAbout = new QPushButton("關於 / 更新", aboutWidget);
    m_btnAbout->setStyleSheet(
        "QPushButton {"
        "    background-color: transparent;"
        "    color: #c7d5e0;"
        "    border: none;"
        "    text-align: left;"
        "    padding: 5px 10px;"
        "}"
        "QPushButton:hover {"
        "    color: #66c0f4;"
        "}"
    );
    connect(m_btnAbout, &QPushButton::clicked, this, [this]() {
        emit panelRequested(PanelAbout);
    });
    aboutLayout->addWidget(m_btnAbout);
    
    mainLayout->addWidget(aboutWidget);
}

void SidebarWidget::setProfiles(const QList<GameProfile> &profiles)
{
    m_profileList->clear();
    
    for (const GameProfile &profile : profiles) {
        QListWidgetItem *item = new QListWidgetItem(m_profileList);
        item->setData(Qt::UserRole, profile.id);
        
        QWidget *itemWidget = new QWidget(m_profileList);
        QHBoxLayout *layout = new QHBoxLayout(itemWidget);
        layout->setContentsMargins(15, 8, 15, 8);
        layout->setSpacing(10);
        
        QLabel *nameLabel = new QLabel(profile.displayName, itemWidget);
        nameLabel->setStyleSheet("color: #c7d5e0; font-size: 10pt;");
        nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        
        QLabel *stateLabel = new QLabel("已停止", itemWidget);
        stateLabel->setObjectName("stateLabel");
        stateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        stateLabel->setStyleSheet("color: #8f98a0; font-size: 9pt;");
        stateLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        
        layout->addWidget(nameLabel, 1);
        layout->addWidget(stateLabel, 0);
        
        item->setSizeHint(itemWidget->sizeHint());
        m_profileList->setItemWidget(item, itemWidget);
    }
}

void SidebarWidget::setItemState(QListWidgetItem *item, int state)
{
    if (!item) {
        return;
    }
    
    QWidget *widget = m_profileList->itemWidget(item);
    if (!widget) {
        return;
    }
    
    QLabel *stateLabel = widget->findChild<QLabel *>("stateLabel");
    if (!stateLabel) {
        return;
    }
    
    // 狀態對應: 0=Stopped, 1=Starting, 2=Running, 3=Stopping
    if (state == 0) {
        stateLabel->setText("已停止");
        stateLabel->setStyleSheet("color: #8f98a0; font-size: 9pt;");
    } else if (state == 1) {
        stateLabel->setText("啟動中");
        stateLabel->setStyleSheet("color: #c0a030; font-size: 9pt;");
    } else if (state == 2) {
        stateLabel->setText("運行中");
        stateLabel->setStyleSheet("color: #79a01b; font-size: 9pt;");
    } else if (state == 3) {
        stateLabel->setText("停止中");
        stateLabel->setStyleSheet("color: #c0a030; font-size: 9pt;");
    } else {
        stateLabel->setText("錯誤");
        stateLabel->setStyleSheet("color: #eb4b4b; font-size: 9pt;");
    }
}

void SidebarWidget::updateServerState(const QString &profileId, int state)
{
    for (int i = 0; i < m_profileList->count(); ++i) {
        QListWidgetItem *item = m_profileList->item(i);
        if (item && item->data(Qt::UserRole).toString() == profileId) {
            setItemState(item, state);
            break;
        }
    }
}

QString SidebarWidget::currentProfileId() const
{
    return m_currentProfileId;
}

void SidebarWidget::setCurrentProfileId(const QString &id)
{
    if (m_currentProfileId == id) {
        return;
    }
    
    m_currentProfileId = id;
    
    for (int i = 0; i < m_profileList->count(); ++i) {
        QListWidgetItem *item = m_profileList->item(i);
        if (item && item->data(Qt::UserRole).toString() == id) {
            m_profileList->setCurrentItem(item);
            break;
        }
    }
}

void SidebarWidget::onProfileClicked(QListWidgetItem *item)
{
    if (!item) {
        return;
    }
    
    QString id = item->data(Qt::UserRole).toString();
    if (m_currentProfileId != id) {
        m_currentProfileId = id;
        emit profileSelected(id);
    }
}
