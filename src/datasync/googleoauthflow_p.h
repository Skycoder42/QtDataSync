#ifndef QTDATASYNC_GOOGLEOAUTHFLOW_H
#define QTDATASYNC_GOOGLEOAUTHFLOW_H

#include "qtdatasync_global.h"

#include <optional>

#include <QtNetworkAuth/QOAuth2AuthorizationCodeFlow>
#include <QtNetworkAuth/QOAuthHttpServerReplyHandler>

namespace QtDataSync {

class Q_DATASYNC_EXPORT GoogleOAuthHandler : public QOAuthHttpServerReplyHandler
{
	Q_OBJECT

public:
	GoogleOAuthHandler(quint16 port, QObject *parent);

	void networkReplyFinished(QNetworkReply *reply) override;

Q_SIGNALS:
	void apiError(const QString &error, const QString &errorDescription, const QUrl &uri = {});
};

class Q_DATASYNC_EXPORT GoogleOAuthFlow : public QOAuth2AuthorizationCodeFlow
{
	Q_OBJECT

public:
	GoogleOAuthFlow(quint16 port, QNetworkAccessManager *nam, QObject *parent);

	QString idToken() const;
	QUrl requestUrl() const;

public Q_SLOTS:
	void setIdToken(QString idToken);

private Q_SLOTS:
	void handleIdToken(const QVariantMap &values);

private:
	QString _idToken;

	QString _codeVerifier;

	QString createChallenge();
	void urlUnescape(QVariantMap *parameters, const QString &key) const;
};

}

#endif // QTDATASYNC_GOOGLEOAUTHFLOW_H
