#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class DiscordManager : public QObject
{
    Q_OBJECT

public:
    explicit DiscordManager(QObject *parent = nullptr);
    ~DiscordManager() override;

    void setWebhookUrl(const QString &url);
    QString webhookUrl() const { return m_webhookUrl; }

    static const int ColorInfo;
    static const int ColorSuccess;
    static const int ColorWarning;
    static const int ColorError;

    void sendEmbedMessage(const QString &title, const QString &description, int color = ColorInfo);

signals:
    void logMessage(const QString &message);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QString m_webhookUrl;
    QNetworkAccessManager *m_networkManager;
};
