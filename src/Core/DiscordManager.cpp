#include "DiscordManager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QUrl>

const int DiscordManager::ColorInfo    = 0x3498DB; // Blue
const int DiscordManager::ColorSuccess = 0x2ECC71; // Green
const int DiscordManager::ColorWarning = 0xF1C40F; // Yellow
const int DiscordManager::ColorError   = 0xE74C3C; // Red

DiscordManager::DiscordManager(QObject *parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this))
{
}

DiscordManager::~DiscordManager()
{
}

void DiscordManager::setWebhookUrl(const QString &url)
{
    m_webhookUrl = url;
}

void DiscordManager::sendEmbedMessage(const QString &title, const QString &description, int color)
{
    if (m_webhookUrl.isEmpty()) {
        return;
    }

    QUrl url(m_webhookUrl);
    if (!url.isValid()) {
        emit logMessage(QString("[FAIL] Discord Webhook URL 無效"));
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject embed;
    embed["title"] = title;
    embed["description"] = description;
    embed["color"] = color;

    QJsonArray embedsArray;
    embedsArray.append(embed);

    QJsonObject payload;
    payload["embeds"] = embedsArray;

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = m_networkManager->post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void DiscordManager::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit logMessage(QString("[WARN] Discord Webhook 發送失敗: %1").arg(reply->errorString()));
    }
    reply->deleteLater();
}
